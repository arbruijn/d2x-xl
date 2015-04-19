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

#include "ipx.h"
#include "loadgame.h"
#include "piggy.h"

// What version of the multiplayer protocol is this?

#define MULTI_PROTO_VERSION			4

// Protocol versions:
//   1 Descent Shareware
//   2 Descent Registered/Commercial
//   3 Descent II Shareware
//   4 Descent II Commercial

// Save multiplayer games?

#define MULTI_SAVE

// How many simultaneous network players do we support?

#define APPLETALK_GAME					1
#define IPX_GAME							2
#define UDP_GAME							3

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
#define MULTI_DESTROY_REACTOR			10
#define MULTI_ROBOT_CLAIM				11
#define MULTI_END_SYNC					12
#define MULTI_CLOAK						13
#define MULTI_ENDLEVEL_START			14
#define MULTI_DOOR_OPEN					15
#define MULTI_CREATE_EXPLOSION		16
#define MULTI_CONTROLCEN_FIRE			17
#define MULTI_PLAYER_DROP				18
#define MULTI_CREATE_POWERUP			19
//unused - #define MULTI_CONSISTENCY				20
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
//unused - #define MULTI_DROP_ORB					62
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
#define MULTI_SYNC_KILLS				79
#define MULTI_COUNTDOWN					80
#define MULTI_PLAYER_WEAPONS			81
#define MULTI_SYNC_MONSTERBALL		82
#define MULTI_DROP_POWERUP				83
#define MULTI_CREATE_WEAPON			84
#define MULTI_AMMO						85
#define MULTI_FUSION_CHARGE			86
#define MULTI_PLAYER_THRUST			87
#define MULTI_CONFIRM_MESSAGE			88
#define MULTI_MAX_TYPE					88

#define MAX_NET_CREATE_OBJECTS		40

#define MULTI_MAX_MSG_LEN				160

// Exported functions

int32_t GetLocalObjNum (int32_t remote_obj, int32_t owner);
int32_t GetRemoteObjNum (int32_t local_obj, int8_t& owner);
void SetObjNumMapping (int32_t local, int32_t remote, int32_t owner);
void SetLocalObjNumMapping (int32_t nObject);
void ResetNetworkObjects ();

void MultiInitObjects (void);
void MultiShowPlayerList (void);
void MultiDoFrame (void);
void MultiCapObjects (void);

void MultiSendWeaponStates (void);
void MultiSendPlayerThrust (void);
void MultiSendFlags (uint8_t);
void MultiSendWeapons (int32_t bForce);
void MultiSendAmmo (void);
void MultiSendMonsterball (int32_t bForce, int32_t bCreate);
void MultiSendFire (void);
void MultiSendDestroyReactor (int32_t nObject, int32_t nPlayer);
void MultiSendCountdown (void);
void MultiSendEndLevelStart (int32_t);
void MultiSendPlayerExplode (uint8_t nType);
void MultiSendMessage (void);
void MultiSendPosition (int32_t nObject);
void MultiSendReappear (void);
void MultiSendKill (int32_t nObject);
void MultiSendRemoveObject (int32_t nObject);
void MultiSendQuit (int32_t nReason);
void MultiSendDoorOpen (int32_t nSegment, int32_t nSide, uint16_t flags);
void MultiSendCreateExplosion (int32_t nPlayer);
void MultiSendCtrlcenFire (CFixVector *to_target, int32_t nGun, int32_t nObject);
void MultiSendInvul (void);
void MultiSendDeInvul (void);
void MultiSendCloak (void);
void MultiSendDeCloak (void);
void MultiSendCreateWeapon (int32_t nObject);
void MultiSendCreatePowerup (int32_t powerupType, int32_t nSegment, int32_t nObject, const CFixVector *vPos);
void MultiSendDropPowerup (int32_t powerupType, int32_t nSegment, int32_t nObject, const CFixVector *vPos, const CFixVector *vVel);
void MultiSendPlaySound (int32_t nSound, fix volume);
void MultiSendAudioTaunt (int32_t taunt_num);
void MultiSendScore (void);
void MultiSendTrigger (int32_t nTrigger, int32_t nObject);
void MultiSendObjTrigger (int32_t CTrigger);
void MultiSendHostageDoorStatus (int32_t nWall);
void MultiSendNetPlayerStatsRequest (uint8_t nPlayer);
void MultiSendDropWeapon (int32_t nObject);
void MultiSendDropMarker (int32_t nPlayer, CFixVector position, char messagenum, char text[]);
void MultiSendGuidedInfo (CObject *miss, char);
void MultiSendReturnFlagHome (int16_t nObject);
void MultiSendCaptureBonus (uint8_t nPlayer);
void MultiSendShield (void);
void MultiSendCheating (void);
void MultiSendFusionCharge (void);

void MultiEndLevelScore (void);
void MultiPrepLevel (void);
int32_t MultiEndLevel (int32_t *secret);
int32_t MultiMenuPoll (void);
void MultiLeaveGame (void);
int MultiProcessData (uint8_t *data, int32_t len);
void MultiProcessBigData (uint8_t *buf, int32_t len);
void MultiDoDeath (int32_t nObject);
void MultiSendMsgDialog (void);
int32_t MultiDeleteExtraObjects (void);
void MultiTurnGhostToPlayer (int32_t nObject);
void MultiTurnPlayerToGhost (int32_t nObject);
void MultiDefineMacro (int32_t key);
void MultiSendMacro (int32_t key);
int32_t MultiGetKillList (int32_t *plist);
void MultiNewGame (void);
void MultiSortKillList (void);
int32_t MultiChooseMission (int32_t *anarchy_only);
void MultiResetStuff (void);
void MultiSendConquerRoom (uint8_t owner, uint8_t prevOwner, uint8_t group);
void MultiSendConquerWarning (void);
void MultiSendStopConquerWarning (void);
void MultiSendData (uint8_t* buf, int32_t len, int32_t repeat);
void ChoseTeam (int32_t nPlayer, bool bForce = false);
void AutoBalanceTeams (void);

int16_t GetTeam (int32_t nPlayer);
bool SameTeam (int32_t nPlayer1, int32_t nPlayer2);

// Exported variables


extern int32_t Network_active;
extern int32_t NetlifeKills,NetlifeKilled;

extern int32_t bUseMacros;

extern int32_t message_length[MULTI_MAX_TYPE+1];

extern CShortArray scoreMatrix;
//extern int16_t scoreMatrix[MAX_PLAYERS][MAX_PLAYERS];


extern void MultiMsgInputSub (int32_t key);
extern void MultiSendMsgStart (char nMsg);
extern void MultiSendMsgQuit (void);
extern void MultiSendTyping (void);

extern int32_t MultiPowerupIs4Pack (int32_t);
extern void MultiSendOrbBonus (uint8_t nPlayer);
extern void MultiSendGotOrb (uint8_t nPlayer);
extern void MultiAddLifetimeKills (void);

extern void MultiSendTeleport (uint8_t nPlayer, int16_t nSegment, uint8_t nSide);

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

typedef struct tNetPlayerInfo {
	char				callsign [CALLSIGN_LEN+1];
	CNetworkInfo	network;
	uint8_t			versionMajor;
	uint8_t			versionMinor;
	uint8_t			computerType;
	int8_t			connected;
	uint16_t			socket;
	uint8_t			rank;

	int32_t IsConnected (void) { return *callsign ? connected : 0; }
} __pack__ tNetPlayerInfo;


typedef struct tAllNetPlayersInfo {
	char		nType;
	int32_t  nSecurity;
	struct	tNetPlayerInfo players [MAX_PLAYERS_D2X + 4];
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
	public:
		int32_t     locations [MAX_PLAYERS_D2];			// 32 bytes
		int16_t		kills [MAX_PLAYERS_D2][MAX_PLAYERS_D2];	// 128 bytes
		uint16_t		nSegmentCheckSum;						// 2 bytes
		int16_t		teamKills [2];							// 4 bytes
		int16_t		killed [MAX_PLAYERS_D2];				// 16 bytes
		int16_t		playerKills [MAX_PLAYERS_D2];		// 16 bytes
		int32_t     nScoreGoal;								// 4 bytes
		fix			xPlayTimeAllowed;						// 4 bytes
		fix			xLevelTime;								// 4 bytes
		int32_t     controlInvulTime;						// 4 bytes
		int32_t     monitorVector;							// 4 bytes
		int32_t		playerScore [MAX_PLAYERS_D2];		// 32 bytes
		uint8_t		playerFlags [MAX_PLAYERS_D2];		// 8 bytes
		int16_t		nMinPPS;							// 2 bytes
		uint8_t		bShortPackets;							// 1 bytes
// 279 bytes
// 355 bytes total
		uint8_t		auxData[NETGAME_AUX_SIZE];  // Storage for protocol-specific data (e.g., multicast session and port)
} __pack__ tNetGameInfoD2;

typedef struct tNetGameInfoD2X {
	public:
		int32_t		locations [MAX_PLAYERS_D2X];		// 32 bytes
		int16_t		kills [MAX_PLAYERS_D2X][MAX_PLAYERS_D2X];	// 128 bytes
		uint16_t		nSegmentCheckSum;						// 2 bytes
		int16_t		teamKills [2];							// 4 bytes
		int16_t		killed [MAX_PLAYERS_D2X];			// 16 bytes
		int16_t		playerKills [MAX_PLAYERS_D2X];		// 16 bytes
		int32_t     nScoreGoal;								// 4 bytes
		fix			xPlayTimeAllowed;						// 4 bytes
		fix			xLevelTime;								// 4 bytes
		int32_t     controlInvulTime;						// 4 bytes
		int32_t     monitorVector;							// 4 bytes
		int32_t     playerScore [MAX_PLAYERS_D2X];		// 32 bytes
		uint8_t		playerFlags [MAX_PLAYERS_D2X];		// 8 bytes
		int16_t		nMinPPS;							// 2 bytes
		uint8_t		bShortPackets;							// 1 bytes
		uint8_t		auxData [NETGAME_AUX_SIZE];  // Storage for protocol-specific data (e.g., multicast session and port)
} __pack__ tNetGameInfoD2X;

typedef struct tNetGameInfoLite {
	public:
		uint8_t		nType;
		int32_t		nSecurity;
		char			szGameName [NETGAME_NAME_LEN+1];
		char			szMissionTitle [MISSION_NAME_LEN+1];
		char			szMissionName [9];
		int32_t		nLevel;
		uint8_t		gameMode;
		uint8_t		bRefusePlayers;
		uint8_t		difficulty;
		uint8_t		gameStatus;
		uint8_t		nNumPlayers;
		uint8_t		nMaxPlayers;
		uint8_t		nConnected;
		uint8_t		gameFlags;
		uint8_t		protocolVersion;
		uint8_t		versionMajor;
		uint8_t		versionMinor;
		uint8_t		teamVector;

   public:
		inline int32_t GetLevel (void) { return nLevel & 0xFFFF; }
		void SetLevel (int32_t n);
		uint16_t GetTeamVector (void);
		void SetTeamVector (uint16_t n);
		inline void AddTeamPlayer (int32_t n) { SetTeamVector (GetTeamVector () | (1 << n)); }
		inline void RemoveTeamPlayer (int32_t n) { SetTeamVector (GetTeamVector () & ~(1 << n)); }
} __pack__ tNetGameInfoLite;

typedef struct tNetGameInfo : tNetGameInfoLite {
// 72 bytes
// change the order of the bit fields for the mac compiler.
// doing so will mean I don't have to do screwy things to
// send this as network information

#if ! (defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
	int16_t DoMegas:1;		//first word
	int16_t DoSmarts:1;
	int16_t DoFusions:1;
	int16_t DoHelix:1;
	int16_t DoPhoenix:1;
	int16_t DoAfterburner:1;
	int16_t DoInvulnerability:1;
	int16_t DoCloak:1;
	int16_t DoGauss:1;
	int16_t DoVulcan:1;
	int16_t DoPlasma:1;
	int16_t DoOmega:1;
	int16_t DoSuperLaser:1;
	int16_t DoProximity:1;
	int16_t DoSpread:1;
	int16_t DoSmartMine:1;
	int16_t DoFlash:1;		//second word
	int16_t DoGuided:1;
	int16_t DoEarthShaker:1;
	int16_t DoMercury:1;
	int16_t bAllowMarkerView:1;
	int16_t bIndestructibleLights:1;
	int16_t DoAmmoRack:1;
	int16_t DoConverter:1;
	int16_t DoHeadlight:1;
	int16_t DoHoming:1;
	int16_t DoLaserUpgrade:1;
	int16_t DoQuadLasers:1;
	int16_t bShowAllNames:1;
	int16_t BrightPlayers:1;
	int16_t invul:1;
	int16_t FriendlyFireOff:1;
#else
	int16_t DoSmartMine:1;
	int16_t DoSpread:1;
	int16_t DoProximity:1;
	int16_t DoSuperLaser:1;
	int16_t DoOmega:1;
	int16_t DoPlasma:1;
	int16_t DoVulcan:1;
	int16_t DoGauss:1;
	int16_t DoCloak:1;
	int16_t DoInvulnerability:1;
	int16_t DoAfterburner:1;
	int16_t DoPhoenix:1;
	int16_t DoHelix:1;
	int16_t DoFusions:1;
	int16_t DoSmarts:1;
	int16_t DoMegas:1;
	int16_t FriendlyFireOff:1;
	int16_t invul:1;
	int16_t BrightPlayers:1;
	int16_t bShowAllNames:1;
	int16_t DoQuadLasers:1;
	int16_t DoLaserUpgrade:1;
	int16_t DoHoming:1;
	int16_t DoHeadlight:1;
	int16_t DoConverter:1;
	int16_t DoAmmoRack:1;
	int16_t bIndestructibleLights:1;
	int16_t bAllowMarkerView:1;
	int16_t DoMercury:1;
	int16_t DoEarthShaker:1;
	int16_t DoGuided:1;
	int16_t DoFlash:1;
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
		uint8_t			m_server [10];

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

		inline bool IsUdpGame (void) { return gameStates.multi.nGameType == UDP_GAME; }

		inline int32_t* Locations (int32_t i = 0) { return IsUdpGame () ? m_info.versionSpecific.d2x.locations + i : m_info.versionSpecific.d2.locations + i; }
		inline int16_t* Kills (int32_t i = 0, int32_t j = 0) { return IsUdpGame () ? m_info.versionSpecific.d2x.kills [i] + j : m_info.versionSpecific.d2.kills [i] + j; }
		inline int16_t* TeamKills (int32_t i = 0) { return IsUdpGame () ? m_info.versionSpecific.d2x.teamKills + i : m_info.versionSpecific.d2.teamKills + i; }
		inline int16_t* Killed (int32_t i = 0) { return IsUdpGame () ? m_info.versionSpecific.d2x.killed  + i : m_info.versionSpecific.d2.killed  + i; }
		inline int16_t* PlayerKills (int32_t i = 0) { return IsUdpGame () ? m_info.versionSpecific.d2x.playerKills  + i : m_info.versionSpecific.d2.playerKills  + i; }
		inline int32_t* PlayerScore (int32_t i = 0) { return IsUdpGame () ? m_info.versionSpecific.d2x.playerScore  + i : m_info.versionSpecific.d2.playerScore  + i; }
		inline uint8_t* PlayerFlags (int32_t i = 0) { return IsUdpGame () ? m_info.versionSpecific.d2x.playerFlags  + i : m_info.versionSpecific.d2.playerFlags  + i; }
		inline uint8_t* AuxData (void) { return IsUdpGame () ? m_info.versionSpecific.d2x.auxData : m_info.versionSpecific.d2.auxData; }

		inline uint16_t GetSegmentCheckSum (void) { return IsUdpGame () ? m_info.versionSpecific.d2x.nSegmentCheckSum : m_info.versionSpecific.d2.nSegmentCheckSum; }
		inline int32_t GetScoreGoal (void) { return IsUdpGame () ? m_info.versionSpecific.d2x.nScoreGoal : m_info.versionSpecific.d2.nScoreGoal; }
		inline fix GetPlayTimeAllowed (void) { return IsUdpGame () ? m_info.versionSpecific.d2x.xPlayTimeAllowed : m_info.versionSpecific.d2.xPlayTimeAllowed; }
		inline fix GetLevelTime (void) { return IsUdpGame () ? m_info.versionSpecific.d2x.xLevelTime : m_info.versionSpecific.d2.xLevelTime; }
		inline int32_t GetControlInvulTime (void) { return IsUdpGame () ? m_info.versionSpecific.d2x.controlInvulTime : m_info.versionSpecific.d2.controlInvulTime; }
		inline int32_t GetMonitorVector (void) { return IsUdpGame () ? m_info.versionSpecific.d2x.monitorVector : m_info.versionSpecific.d2.monitorVector; }
		inline int16_t GetMinPPS (void) { return IsUdpGame () ? m_info.versionSpecific.d2x.nMinPPS : m_info.versionSpecific.d2.nMinPPS; }
		inline uint8_t GetShortPackets (void) { return IsUdpGame () ? m_info.versionSpecific.d2x.bShortPackets : m_info.versionSpecific.d2.bShortPackets; }

		inline void SetSegmentCheckSum (uint16_t n) { if (IsUdpGame ()) m_info.versionSpecific.d2x.nSegmentCheckSum = n; else m_info.versionSpecific.d2.nSegmentCheckSum = n; }
		inline void SetScoreGoal (int32_t n) { if (IsUdpGame ()) m_info.versionSpecific.d2x.nScoreGoal = n; else m_info.versionSpecific.d2.nScoreGoal = n; }
		inline void SetPlayTimeAllowed (fix n) { if (IsUdpGame ()) m_info.versionSpecific.d2x.xPlayTimeAllowed = n; else m_info.versionSpecific.d2.xPlayTimeAllowed = n; }
		inline void SetLevelTime (fix n) { if (IsUdpGame ()) m_info.versionSpecific.d2x.xLevelTime = n; else m_info.versionSpecific.d2.xLevelTime = n; }
		inline void SetControlInvulTime (int32_t n) { if (IsUdpGame ()) m_info.versionSpecific.d2x.controlInvulTime = n; else m_info.versionSpecific.d2.controlInvulTime = n; }
		inline void SetMonitorVector (int32_t n) { if (IsUdpGame ()) m_info.versionSpecific.d2x.monitorVector = n; else m_info.versionSpecific.d2.monitorVector = n; }
		inline void SetMinPPS (int16_t n) { if (IsUdpGame ()) m_info.versionSpecific.d2x.nMinPPS = n; else m_info.versionSpecific.d2.nMinPPS = n; }
		inline void SetShortPackets (uint8_t n) { if (IsUdpGame ()) m_info.versionSpecific.d2x.bShortPackets = 0; else m_info.versionSpecific.d2.bShortPackets = n; } // always long packets for UDP/IP
};

#define MAX_ROBOTS_CONTROLLED 5

typedef struct tMultiRobotData {
	int32_t	controlled [MAX_ROBOTS_CONTROLLED];
	int32_t	agitation [MAX_ROBOTS_CONTROLLED];
	fix		controlledTime [MAX_ROBOTS_CONTROLLED];
	fix		lastSendTime [MAX_ROBOTS_CONTROLLED];
	fix		lastMsgTime [MAX_ROBOTS_CONTROLLED];
	int32_t	sendPending [MAX_ROBOTS_CONTROLLED];
	int32_t	fired [MAX_ROBOTS_CONTROLLED];
	int8_t	fireBuf [MAX_ROBOTS_CONTROLLED][18+3];
} __pack__ tMultiRobotData;

extern CNetGameInfo netGameInfo;
extern CAllNetPlayersInfo netPlayers [2];

int32_t IAmGameHost (void);
void ChangePlayerNumTo (int32_t nPlayer);
int32_t SetLocalPlayer (CAllNetPlayersInfo* playerInfoP, int32_t nPlayers, int32_t nDefault = -1);

//how to encode missiles & flares in weapon packets
#define MISSILE_ADJUST  100
#define FLARE_ADJUST    127

int32_t PingPlayer (int32_t n);
int32_t MultiProtectGame (void);
void SwitchTeam (int32_t nPlayer, int32_t bForce);
void SetTeam (int32_t nPlayer, int32_t team);
void MultiSendModemPing ();
int32_t MultiFindGoalTexture (int16_t t);
void MultiSetFlagPos (void);
void ResetPlayerPaths (void);
void UpdatePlayerPaths (void);
void MultiSyncKills (void);
void MultiAdjustPowerups (void);
void RemapLocalPlayerObject (int32_t nLocalObj, int32_t nRemoteObj);
void MultiOnlyPlayerMsg (int32_t bMsgBox);

void MultiSendConfirmMessage (int32_t nId);
void MultiSendStolenItems (void);
void MultiSendScoreGoalCounts (void);
void MultiSendPowerupUpdate (void);
void MultiSendDoorOpenSpecific (int32_t nPlayer, int32_t nSegment, int32_t nSide, uint16_t flags);
void MultiSendWallStatusSpecific (int32_t nPlayer, int32_t wallnum, uint8_t nType, uint16_t flags, uint8_t state);
void MultiSendLightSpecific (int32_t nPlayer, int32_t nSegment, uint8_t val);
void MultiSendTriggerSpecific (uint8_t nPlayer, uint8_t trig);
void MultiSetObjectTextures (CObject *objP);
void MultiSendSeismic (fix start, fix end);
void MultiSendDropBlobs (uint8_t nPlayer);
void InitDefaultShipProps (void);
void SetupPowerupFilter (tNetGameInfo* infoP = NULL);
void MultiDestroyPlayerShip (int32_t nPlayer, int32_t bExplode = 1, int32_t nRemoteCreated = 0, int16_t* objList = NULL);

bool MultiTrackMessage (uint8_t* buf);
bool MultiProcessMessage (uint8_t* buf);

#if DBG
#	define MULTIROBOT(nObject)	MultiRobot (nObject, __FILE__, __LINE__)
CObject* MultiRobot (int32_t nObject, const char* pszFile = "", const int32_t nLine = 0);
#else
#	define MULTIROBOT(nObject)	MultiRobot (nObject)
CObject* MultiRobot (int32_t nObject);
#endif


// Reserve space for the message id in the message. This macro will place the message id right after the one byte message type in the message buffer.
// bufP must point to the byte in the message buffer following the player id. The variable _msgId is created and points to the first byte of the reserved space in the message buffer.
#define ADD_MSG_ID				uint8_t* _msgIdP = gameData.multigame.msg.buf + bufP; \
										if (gameStates.multi.nGameType == UDP_GAME) { \
											PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, 0); \
											bufP += 4; \
											}

// Put the actual message id in the message buffer and add the message to the list of important messages.
#define SET_MSG_ID				MultiTrackMessage (_msgIdP);

// Check whether the message with the message id contained in it has already been processed. If yes, exit the processing function.
// If no, add the sender's player id and the message id to the list of important messages the receipt of which needs to be confirmed and send a receipt confirmation.
// That list entry is subsequently being used to determine whether a message has already been processed and has been resent by its originator because it hasn't received a 
// receipt confirmation for it from all game participants
#define CHECK_MSG_ID				if (!MultiProcessMessage (buf + bufP)) \
											return; \
										bufP += 4;

// Same as above, just returns the value specified by _r
#define CHECK_MSG_ID_RVAL(_r)	if (!MultiProcessMessage (buf + bufP)) \
											return (_r); \
										bufP += 4;

#endif /* _MULTI_H */
