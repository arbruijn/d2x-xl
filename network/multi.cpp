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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "descent.h"
#include "u_mem.h"
#include "strutil.h"
#include "network.h"
#include "network_lib.h"
#include "objsmoke.h"
#include "cockpit.h"
#include "collide.h"
#include "error.h"
#include "fireball.h"
#include "objeffects.h"
#include "loadobjects.h"
#include "endlevel.h"
#include "playerprofile.h"
#include "timer.h"
#include "newdemo.h"
#include "text.h"
#include "headlight.h"
#include "highscores.h"
#include "multibot.h"
#include "segmath.h"
#include "physics.h"
#include "savegame.h"
#include "byteswap.h"
#include "rendermine.h"
#include "multimsg.h"
#include "entropy.h"
#include "monsterball.h"
#include "dropobject.h"
#include "marker.h"
#include "console.h"

typedef void tMultiHandler (uint8_t *);
typedef tMultiHandler *pMultiHandler;
typedef struct tMultiHandlerInfo {
	pMultiHandler	fpMultiHandler;
	char				noEndLevelSeq;
} tMultiHandlerInfo;

void MultiResetPlayerObject (CObject *objP);
void MultiSetObjectTextures (CObject *objP);
void MultiAddLifetimeKilled (void);
void MultiAddLifetimeKills (void);
void MultiSendPlayByPlay (int32_t num, int32_t spnum, int32_t dpnum);
void MultiSendHeartBeat (void);
void MultiCapObjects (void);
void MultiSaveGame (char slot, uint32_t id, char *desc);
void MultiRestoreGame (char slot, uint32_t id);
void MultiSetRobotAI (void);
void MultiSendPowerupUpdate (void);
void FreeHoardData (void);
void InitHoardData (void);
void MultiApplyGoalTextures (void);
void MultiBadRestore (void);
void MultiDoCaptureBonus (uint8_t* buf);
void MultiDoOrbBonus (uint8_t* buf);
void MultiSendDropFlag (int32_t nObject, int32_t seed);
void MultiSendRanking (void);
void MultiDoPlayByPlay (uint8_t* buf);
void MultiDoConquerRoom (uint8_t* buf);
void MultiDoConquerWarning (uint8_t* buf);
void MultiAdjustPowerupCap (void);

//
// Local macros and prototypes
//

// LOCALIZE ME!!

void DropPlayerEggs (CObject* player); // from collide.c

//
// Global variaszPlayers
//

extern void SetFunctionMode (int32_t);

CNetGameInfo netGameInfo;

extern CAllNetPlayersInfo netPlayers [2];

tBitmapIndex mpTextureIndex [MAX_PLAYERS][N_PLAYER_SHIP_TEXTURES];

typedef struct tNetPlayerStats {
	uint8_t		messageType;
	uint8_t		nLocalPlayer;          // Who am i?
	uint32_t		flags;                 // Powerup flags, see below...
	fix			energy;                // Amount of energy remaining.
	fix			shield;                // shield remaining (protection)
	uint8_t		lives;                 // Lives remaining, 0 = game over.
	uint8_t		laserLevel;				// Current level of the laser.
	uint8_t		primaryWeaponFlags;		// bit set indicates the player has this weapon.
	uint8_t		secondaryWeaponFlags;  // bit set indicates the player has this weapon.
	uint16_t		primaryAmmo [MAX_PRIMARY_WEAPONS];     // How much ammo of each nType.
	uint16_t		secondaryAmmo [MAX_SECONDARY_WEAPONS]; // How much ammo of each nType.
	int32_t		lastScore;             // Score at beginning of current level.
	int32_t		score;                 // Current score.
	fix			cloakTime;             // Time cloaked
	fix			invulnerableTime;      // Time invulnerable
	fix			homingObjectDist;      // Distance of nearest homing CObject.
	int16_t		nScoreGoalCount;
	int16_t		netKilledTotal;        // Number of times nKilled total
	int16_t		netKillsTotal;         // Number of net kills total
	int16_t		numKillsLevel;         // Number of kills this level
	int16_t		numKillsTotal;         // Number of kills total
	int16_t		numRobotsLevel;        // Number of initial robots this level
	int16_t		numRobotsTotal;        // Number of robots total
	uint16_t		nHostagesRescued;		// Total number of hostages rescued.
	uint16_t		nHostagesTotal;        // Total number of hostages.
	uint8_t		nHostagesOnBoard;      // Number of hostages on ship.
	uint8_t		unused [16];
} tNetPlayerStats;

static int32_t multiMessageLengths [MULTI_MAX_TYPE+1][2] = {
	{24, 62}, // MULTI_POSITION
	{3, -1},  // MULTI_REAPPEAR
	{8, -1}, //9 + MAX_FIRED_OBJECTS * sizeof (int16_t)},  // MULTI_FIRE
	{5, 9},  // MULTI_KILL
	{4, 8},  // MULTI_REMOVE_OBJECT
	{106, 142}, // MULTI_PLAYER_EXPLODE
	{37, -1}, // MULTI_MESSAGE (MAX_MESSAGE_LENGTH = 40)
	{2, -1},  // MULTI_QUIT
	{4, -1},  // MULTI_PLAY_SOUND
	{41, -1}, // MULTI_BEGIN_SYNC
	{4, 8},  // MULTI_DESTROY_REACTOR
	{5, 9},  // MULTI_ROBOT_CLAIM
	{4, -1},  // MULTI_END_SYNC
	{2, 6},  // MULTI_CLOAK
	{3, 7},  // MULTI_ENDLEVEL_START
	{5, 9},  // MULTI_DOOR_OPEN
	{2, -1},  // MULTI_CREATE_EXPLOSION
	{16, -1}, // MULTI_CONTROLCEN_FIRE
	{106, 142}, // MULTI_PLAYER_DROP
	{19, 23}, // MULTI_CREATE_POWERUP
	{9, -1},  // MULTI_MISSILE_TRACK
	{2, 6},  // MULTI_DE-CLOAK
	{2, -1},  // MULTI_MENU_CHOICE
	{28, -1}, // MULTI_ROBOT_POSITION  (shortpos_length (23) + 5 = 28)
	{9, 13},  // MULTI_ROBOT_EXPLODE
	{5, 9},  // MULTI_ROBOT_RELEASE
	{18, -1}, // MULTI_ROBOT_FIRE
	{6, -1},  // MULTI_SCORE
	{6, 10},  // MULTI_CREATE_ROBOT
	{3, 9},  // MULTI_TRIGGER
	{10, -1}, // MULTI_BOSS_ACTIONS
	{27, 59}, // MULTI_CREATE_ROBOT_POWERUPS
	{7, 11},  // MULTI_HOSTAGE_DOOR
	{2+24, 30}, // MULTI_SAVE_GAME      (uint8_t slot, -1}, uint32_t id, -1}, char name [20])
	{2+4, 10},  // MULTI_RESTORE_GAME   (uint8_t slot, -1}, uint32_t id)
	{1+1, -1},  // MULTI_REQ_PLAYER
	{sizeof (tNetPlayerStats), -1}, // MULTI_SEND_PLAYER
	{55, -1}, // MULTI_MARKER
	{12, 16}, // MULTI_DROP_WEAPON
	{3+sizeof (tShortPos), 7 + sizeof (tLongPos)}, // MULTI_GUIDED
	{11, 15}, // MULTI_STOLEN_ITEMS
	{6, 10},  // MULTI_WALL_STATUS
	{5, -1},  // MULTI_HEARTBEAT
	{9, 21},  // MULTI_SCOREGOALS
	{9, 13},  // MULTI_SEISMIC
	{18, 22}, // MULTI_LIGHT
	{2, 6},  // MULTI_START_TRIGGER
	{6, 10},  // MULTI_FLAGS
	{2, -1},  // MULTI_DROP_BLOB
	{/*2 **/ MAX_POWERUP_TYPES * sizeof (uint16_t) + 1, MAX_POWERUP_TYPES * sizeof (uint16_t) + 5}, // MULTI_POWERUP_UPDATE
	{sizeof (CActiveDoor) + 3, -1}, // MULTI_ACTIVE_DOOR
	{4, -1},  // MULTI_SOUND_FUNCTION
	{2, 6},  // MULTI_CAPTURE_BONUS
	{2, 6},  // MULTI_GOT_FLAG
	{12, 16}, // MULTI_DROP_FLAG
	{142, 146}, // MULTI_ROBOT_CONTROLS
	{2, 6},  // MULTI_FINISH_GAME
	{3, -1},  // MULTI_RANK
	{1, -1},  // MULTI_MODEM_PING
	{1, -1},  // MULTI_MODEM_PING_RETURN
	{3, 7},  // MULTI_ORB_BONUS
	{2, 6},  // MULTI_GOT_ORB
	{12, -1}, // MULTI_DROP_ORB
	{4, 8},  // MULTI_PLAY_BY_PLAY
	{3, 7},	 // MULTI_RETURN_FLAG
	{4, 8},	 // MULTI_CONQUER_ROOM
	{3, -1},  // MULTI_CONQUER_WARNING
	{3, -1},  // MULTI_STOP_CONQUER_WARNING
	{5, 9},  // MULTI_TELEPORT
	{4, 8},  // MULTI_SET_TEAM
	{3, -1},  // MULTI_MESSAGE_START
	{3, -1},  // MULTI_MESSAGE_QUIT
	{3, 7},	 // MULTI_OBJECT_TRIGGER
	{6, -1},	 // MULTI_PLAYER_SHIELDS
	{2, 6},	 // MULTI_INVUL
	{2, 6},	 // MULTI_DEINVUL
	{37, -1}, // MULTI_WEAPONS
	{40, -1}, // MULTI_MONSTERBALL
	{2, -1},  // MULTI_CHEATING
	{16, -1}, // MULTI_SYNC_KILLS
	{5, -1},	 // MULTI_COUNTDOWN
	{29, -1}, // MULTI_PLAYER_WEAPONS
	{99, -1}, // MULTI_SYNC_MONSTERBALL
	{31, 35}, // MULTI_DROP_POWERUP
	{31, 35}, // MULTI_CREATE_WEAPON
	{6, -1},  // MULTI_AMMO
	{6, -1},  // MULTI_FUSION_CHARGE
	{7, -1},  // MULTI_PLAYER_THRUST
	{6, -1}   // MULTI_CONFIRM_MESSAGE
};

void ExtractNetPlayerStats (tNetPlayerStats *ps, CPlayerInfo * pd);
void UseNetPlayerStats (CPlayerInfo * ps, tNetPlayerStats *pd);

//-----------------------------------------------------------------------------

int32_t MultiMsgLen (uint8_t nMsg)
{
#if 0
return multiMessageLengths [nMsg][0];
#else
	int32_t l = multiMessageLengths [nMsg][gameStates.multi.nGameType == UDP_GAME];

return (l > 0) ? l : multiMessageLengths [nMsg][0];
#endif
}

//-----------------------------------------------------------------------------

bool MultiProcessMessage (uint8_t* buf)
{
if (gameStates.multi.nGameType != UDP_GAME)
	return false;
int32_t nId = GET_INTEL_INT (buf);
if (!nId)
	return true;
MultiSendConfirmMessage (nId); // (re-) confirm message
if (importantMessages [1].Find (nId))
	return false;
importantMessages [1].Add (nId);
return true; // message is new and needs to be processed
}

//-----------------------------------------------------------------------------

bool MultiTrackMessage (uint8_t* buf)
{
if (gameStates.multi.nGameType != UDP_GAME)
	return false;
int32_t nId = importantMessages [0].Add (buf);
if (nId < 0)
	return false;
PUT_INTEL_INT (buf, nId);
return true;
}

//-----------------------------------------------------------------------------

void tNetGameInfoLite::SetLevel (int32_t n) 
{
nLevel = ((gameStates.multi.nGameType == UDP_GAME) && !IsCoopGame) ? (nLevel & 0xFFFF0000) | (n & 0xFFFF) : n; 
}

//-----------------------------------------------------------------------------

uint16_t tNetGameInfoLite::GetTeamVector (void) 
{ 
return ((gameStates.multi.nGameType == UDP_GAME) && !IsCoopGame) ? uint16_t (nLevel >> 16) : uint16_t (teamVector); 
}

//-----------------------------------------------------------------------------

void tNetGameInfoLite::SetTeamVector (uint16_t n) 
{ 
if ((gameStates.multi.nGameType == UDP_GAME) && !IsCoopGame) 
	nLevel = (nLevel & 0xFFFF) | (int32_t (n) << 16); 
else 
	teamVector = uint8_t (n); 
}

//-----------------------------------------------------------------------------
// check protection
// return 0 if player cheats and cheat cannot be neutralized
// currently doesn't do anything besides setting player ship physics to default values.

int32_t MultiProtectGame (void)
{
if (IsMultiGame && !IsCoopGame) {
	SetDefaultShipProps ();
	SetDefaultWeaponProps ();
	}
return 1;
}

//-----------------------------------------------------------------------------
//
//  Functions that replace what used to be macros
//

// Map a remote object number from nOwner to a local object number

int32_t GetLocalObjNum (int32_t nRemoteObj, int32_t nOwner)
{
	int32_t nObject;

if ((nOwner >= N_PLAYERS) || (nOwner < -1)) {
	Int3 (); // Illegal!
	return nRemoteObj;
	}
if (nOwner == -1)
	return nRemoteObj;
if ((nRemoteObj < 0) || (nRemoteObj >= LEVEL_OBJECTS))
	return -1;
nObject = gameData.multigame.remoteToLocal [nOwner][nRemoteObj];
if (nObject < 0)
	return -1;
return nObject;
}

//-----------------------------------------------------------------------------
// Map a local CObject number to a remote + nOwner

int32_t GetRemoteObjNum (int32_t nLocalObj, int8_t& nObjOwner)
{
if ((nLocalObj < 0) || (nLocalObj > gameData.objData.nLastObject [0])) {
	nObjOwner = -1;
	return -1;
	}
nObjOwner = gameData.multigame.nObjOwner [nLocalObj];
if (nObjOwner == -1)
	return nLocalObj;
if ((nObjOwner >= N_PLAYERS) || (nObjOwner < -1)) {
	Int3 (); // Illegal!
	nObjOwner = -1;
	return nLocalObj;
	}
return gameData.multigame.localToRemote [nLocalObj];
}

//-----------------------------------------------------------------------------

void RemapLocalPlayerObject (int32_t nLocalObj, int32_t nRemoteObj)
{
if (nLocalObj != nRemoteObj)
	for (int32_t i = 0; i < gameData.multiplayer.nPlayerPositions; i++)
		if (PLAYER (i).nObject == nRemoteObj) {
			PLAYER (i).SetObject (nLocalObj);
			if (i == N_LOCALPLAYER)
				gameData.objData.consoleP = OBJECT (nLocalObj);
			break;
			}
}

//-----------------------------------------------------------------------------
// Add a mapping from a network remote object number to a local one

void SetObjNumMapping (int32_t nLocalObj, int32_t nRemoteObj, int32_t nObjOwner)
{
if ((nLocalObj > -1) && (nLocalObj < LEVEL_OBJECTS) && 
	 (nRemoteObj > -1) && (nRemoteObj < LEVEL_OBJECTS)/* && 
	 (nOwner > -1) && (nOwner != N_LOCALPLAYER)*/) {
	gameData.multigame.nObjOwner [nLocalObj] = nObjOwner;
	if ((nObjOwner > -1) && (nObjOwner < N_PLAYERS))
		gameData.multigame.remoteToLocal [nObjOwner][nRemoteObj] = nLocalObj;
	gameData.multigame.localToRemote [nLocalObj] = nRemoteObj;
	}
}

//-----------------------------------------------------------------------------
// Add a mapping for our locally created OBJECTS

void SetLocalObjNumMapping (int32_t nLocalObj)
{
#if 1
SetObjNumMapping (nLocalObj, nLocalObj, N_LOCALPLAYER);
#else
if ((nLocalObj > -1) && (nLocalObj < LEVEL_OBJECTS)) {
	gameData.multigame.nObjOwner [nLocalObj] = N_LOCALPLAYER;
	gameData.multigame.remoteToLocal [N_LOCALPLAYER][nLocalObj] = nLocalObj;
	gameData.multigame.localToRemote [nLocalObj] = nLocalObj;
	}
#endif
}

//-----------------------------------------------------------------------------

void ResetNetworkObjects (void)
{
gameData.multigame.localToRemote.Clear (0xff);
for (int32_t i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	gameData.multigame.remoteToLocal [i].Clear (0xff);
gameData.multigame.nObjOwner.Clear (0xff);
}

//
// Part 1 : functions whose main purpose in life is to divert the flow
//          of execution to either network or serial specific code based
//          on the curretn gameData.app.nGameMode value.
//

// -----------------------------------------------------------------------------

void ResetPlayerPaths (void)
{
	CPlayerData	*playerP = gameData.multiplayer.players;
	int32_t		i;

for (i = 0; i < MAX_PLAYERS; i++, playerP++) {
	gameData.render.thrusters [i].path.Reset (10, 1);
	PLAYER (i).m_flightPath.Reset (-1, -1);
	}
}

// -----------------------------------------------------------------------------

void UpdatePlayerPaths (void)
{
for (int32_t i = 0; i < N_PLAYERS; i++) {
	CObject* objP = OBJECT (PLAYER (i).nObject);
	if (objP) {
		gameData.render.thrusters [i].path.Update (objP);
		PLAYER (i).m_flightPath.Update (objP);
		}
	}
}

// -----------------------------------------------------------------------------

static inline int32_t BonusScore (void)
{
if (IsEntropyGame)
	return 3;
else if (IsHoardGame)
	return 5;
else if (gameData.app.GameMode (GM_CAPTURE))
	return 5;
else if (gameData.app.GameMode (GM_MONSTERBALL))
	return extraGameInfo [1].monsterball.nBonus;
else
	return 1;
}

// -----------------------------------------------------------------------------

static inline int32_t ScoreGoal (bool bNone = false)
{
return bNone ? 0 : netGameInfo.GetScoreGoal () ? netGameInfo.GetScoreGoal () * BonusScore () : 0x7fffffff;
}

// -----------------------------------------------------------------------------

static void CheckScoreGoal (int32_t nPlayer, bool bForce = false)
{
if (PLAYER (nPlayer).nScoreGoalCount >= ScoreGoal (bForce)) {
	if (nPlayer != N_LOCALPLAYER) 
		HUDInitMessage (TXT_REACH_SCOREGOAL2, PLAYER (nPlayer).callsign);
	else {
		HUDInitMessage (TXT_REACH_SCOREGOAL);
		LOCALPLAYER.SetShield (MAX_SHIELD);
		}
	if (!gameData.reactor.bDestroyed) {
		HUDInitMessage (TXT_CTRLCEN_DEAD);
		NetDestroyReactor (ObjFindFirstOfType (OBJ_REACTOR));
		}	
	gameData.reactor.bDestroyed = -1;
	}
}

// -----------------------------------------------------------------------------

void MultiSetFlagPos (void)
{
	CPlayerData	*playerP = gameData.multiplayer.players;
#if 1//!DBG
for (int32_t i = 0; i < N_PLAYERS; i++, playerP++)
	if (playerP->flags & PLAYER_FLAGS_FLAG)
		gameData.pig.flags [!GetTeam (i)].path.Update (OBJECT (playerP->nObject));
#else
SetPathPoint (&gameData.pig.flags [0].path, OBJECT (playerP->nObject));
#endif
}

//-----------------------------------------------------------------------------
// Show a score list to end of net players
// Save connect state and change to new connect state

void MultiEndLevelScore (void)
{
	int32_t oldConnect = 0;
	int32_t i;

if (IsNetworkGame) {
	oldConnect = LOCALPLAYER.IsConnected () ? LOCALPLAYER.connected : 0;
	if (LOCALPLAYER.connected != CONNECT_DIED_IN_MINE)
		CONNECT (N_LOCALPLAYER, CONNECT_END_MENU);
	networkData.nStatus = NETSTAT_ENDLEVEL;
	}
// Do the actual screen we wish to show
SetFunctionMode (FMODE_MENU);
scoreTable.Display ();
SetFunctionMode (FMODE_GAME);
// Restore connect state
if (IsNetworkGame)
	CONNECT (N_LOCALPLAYER, oldConnect);
if (IsCoopGame) {
	for (i = 0; i < gameData.multiplayer.nMaxPlayers; i++) // Reset keys
		PLAYER (i).flags &= ~(PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY);
	}
for (i = 0; i < gameData.multiplayer.nMaxPlayers; i++)
	PLAYER (i).flags &= ~(PLAYER_FLAGS_FLAG);  // Clear capture flag

for (i = 0; i < MAX_PLAYERS; i++)
	PLAYER (i).nScoreGoalCount = 0;
gameData.multiplayer.maxPowerupsAllowed.Clear (0);
gameData.multiplayer.powerupsInMine.Clear (0);
}

//-----------------------------------------------------------------------------

int16_t GetTeam (int32_t nPlayer)
{
return (netGameInfo.m_info.GetTeamVector () & (1 << nPlayer)) ? 1 : 0;
}

//-----------------------------------------------------------------------------

bool SameTeam (int32_t nPlayer1, int32_t nPlayer2)
{
if (IsCoopGame)
	return true;
if (!IsTeamGame)
	return false;
return GetTeam (nPlayer1) == GetTeam (nPlayer2);
}

//-----------------------------------------------------------------------------

void MultiSendWeaponStates (void)
{
	CWeaponState	*wsP = gameData.multiplayer.weaponStates + N_LOCALPLAYER;

gameData.multigame.msg.buf [0] = uint8_t (MULTI_PLAYER_WEAPONS);
gameData.multigame.msg.buf [1] = uint8_t (N_LOCALPLAYER);
gameData.multigame.msg.buf [2] = wsP->nPrimary;
gameData.multigame.msg.buf [3] = wsP->nSecondary;
gameData.multigame.msg.buf [4] = wsP->nMissiles;
gameData.multigame.msg.buf [5] = wsP->nLaserLevel;
gameData.multigame.msg.buf [6] = wsP->bQuadLasers;
PUT_INTEL_INT (gameData.multigame.msg.buf + 7, wsP->firing [0].nDuration);
PUT_INTEL_INT (gameData.multigame.msg.buf + 11, wsP->firing [1].nDuration);
gameData.multigame.msg.buf [15] = (uint8_t) wsP->firing [0].bSpeedUp;
gameData.multigame.msg.buf [16] = wsP->bTripleFusion;
gameData.multigame.msg.buf [17] = wsP->nMslLaunchPos;
PUT_INTEL_INT (gameData.multigame.msg.buf + 18, wsP->xMslFireTime);
gameData.multigame.msg.buf [22] = wsP->nBuiltinMissiles;
gameData.multigame.msg.buf [23] = wsP->nThrusters [0];
gameData.multigame.msg.buf [24] = wsP->nThrusters [1];
gameData.multigame.msg.buf [25] = wsP->nThrusters [2];
gameData.multigame.msg.buf [26] = wsP->nThrusters [3];
gameData.multigame.msg.buf [27] = wsP->nThrusters [4];
gameData.multigame.msg.buf [28] = char (gameOpts->gameplay.nShip [0]);
MultiSendData (gameData.multigame.msg.buf, 29, 0);
}

//-----------------------------------------------------------------------------

void MultiSendPlayerThrust (void)
{
	CWeaponState* wsP = gameData.multiplayer.weaponStates + N_LOCALPLAYER;

gameData.multigame.msg.buf [0] = uint8_t (MULTI_PLAYER_THRUST);
gameData.multigame.msg.buf [1] = uint8_t (N_LOCALPLAYER);
memcpy (gameData.multigame.msg.buf + 2, wsP->nThrusters, sizeof (wsP->nThrusters));
MultiSendData (gameData.multigame.msg.buf, 2 + sizeof (wsP->nThrusters), 0);
}

//-----------------------------------------------------------------------------

void MultiDoWeaponStates (uint8_t* buf)
{
	CWeaponState*	wsP = gameData.multiplayer.weaponStates + int32_t (buf [1]);
	tFiringData	*	fP;
	int32_t			i;

wsP->nPrimary = buf [2];
wsP->nSecondary = buf [3];
wsP->nMissiles = buf [4];
wsP->nLaserLevel = buf [5];
wsP->bQuadLasers = buf [6];
wsP->firing [0].nDuration = GET_INTEL_INT (buf + 7);
wsP->firing [1].nDuration = GET_INTEL_INT (buf + 11);
wsP->firing [0].bSpeedUp = buf [15];
wsP->bTripleFusion = buf [16];
wsP->nMslLaunchPos = buf [17];
wsP->xMslFireTime = GET_INTEL_INT (gameData.multigame.msg.buf + 18);
wsP->nBuiltinMissiles = buf [22];
wsP->nThrusters [0] = buf [23];
wsP->nThrusters [1] = buf [24];
wsP->nThrusters [2] = buf [25];
wsP->nThrusters [3] = buf [26];
wsP->nThrusters [4] = buf [27];
if (2 < (wsP->nShip = buf [28]))
 wsP->nShip = 1;
for (i = 0, fP = wsP->firing; i < 2; i++, fP++) {
	if (fP->nDuration) {
		if (fP->nStart <= 0) {
			fP->nStart = gameStates.app.nSDLTicks [0] - fP->nDuration;
			if (fP->bSpeedUp &&(fP->nDuration < GATLING_DELAY)) {
				fP->bSound = 1;
				}
			else {
				fP->nStart -= GATLING_DELAY + 1;
				if (fP->nStart < 0)
					fP->nStart = 0;
				fP->bSound = 0;
				}
			fP->nStop = 0;
			}
		}
	else {
		if (wsP->firing [i].nStart) {
			wsP->firing [i].nStart = 0;
			wsP->firing [i].nStop = gameStates.app.nSDLTicks [0];
			}
		}
	}
}

//-----------------------------------------------------------------------------

void MultiDoPlayerThrust (uint8_t* buf)
{
	CWeaponState	*wsP = gameData.multiplayer.weaponStates + int32_t (buf [1]);

memcpy (wsP->nThrusters, buf + 2, sizeof (wsP->nThrusters));
}

//-----------------------------------------------------------------------------

void MultiSendSetTeam (int32_t nPlayer)
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = uint8_t (MULTI_SET_TEAM);
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = uint8_t (nPlayer);
gameData.multigame.msg.buf [bufP++] = uint8_t (GetTeam (nPlayer));
gameData.multigame.msg.buf [bufP++] = 0;
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 0);
}

//-----------------------------------------------------------------------------

static const char *szTeamColors [2] = {TXT_BLUE, TXT_RED};

void SetTeam (int32_t nPlayer, int32_t team)
{
for (int32_t i = 0; i < N_PLAYERS; i++)
	if (PLAYER (i).IsConnected ())
		MultiSetObjectTextures (OBJECT (PLAYER (i).nObject));

if (team >= 0) {
	if (team)
		netGameInfo.m_info.AddTeamPlayer (nPlayer);
	else
		netGameInfo.m_info.RemoveTeamPlayer (nPlayer);
	}
else {
	team = !GetTeam (nPlayer);
	if (team)
		netGameInfo.m_info.AddTeamPlayer (nPlayer);
	else
		netGameInfo.m_info.RemoveTeamPlayer (nPlayer);
	NetworkSendNetGameUpdate ();
	}
sprintf (gameData.multigame.msg.szMsg, TXT_TEAMCHANGE3, PLAYER (nPlayer).callsign);
if (nPlayer == N_LOCALPLAYER) {
	HUDInitMessage (TXT_TEAMJOIN, szTeamColors [team]);
	}
else
	HUDInitMessage (TXT_TEAMJOIN2, PLAYER (nPlayer).callsign, szTeamColors [team]);
}

//-----------------------------------------------------------------------------

void MultiDoSetTeam (uint8_t* buf)
{
	int32_t bufP = 1;

CHECK_MSG_ID

SetTeam (int32_t (buf [bufP + 1]), int32_t (buf [bufP + 2]));
}

//-----------------------------------------------------------------------------

void StartPlayerDeathSequence (CObject *player);

void SwitchTeam (int32_t nPlayer, int32_t bForce)
{
if (gameStates.app.bHaveExtraGameInfo [1] &&(!extraGameInfo [1].bAutoBalanceTeams || bForce)) {
	int32_t t = !GetTeam (nPlayer);
	SetTeam (nPlayer, t);
	MultiSendSetTeam (nPlayer);
	if (!bForce) {
		PLAYER (nPlayer).SetShield (-1);
		StartPlayerDeathSequence (OBJECT (PLAYER (nPlayer).nObject));
		}
	}
}

//-----------------------------------------------------------------------------

void ChoseTeam (int32_t nPlayer, bool bForce)
{
if (gameStates.app.bHaveExtraGameInfo && (bForce || extraGameInfo [1].bAutoBalanceTeams) && IsTeamGame && IAmGameHost ()) {
	int32_t	i, t, teamScore [2] = {0, 0}, teamSize [2] = {0, 0};

	for (i = 0; i < N_PLAYERS; i++) {
		t = GetTeam (i);
		teamSize [t]++;
		teamScore [t] += PLAYER (i).score;
		}
	// put player on red team if red team smaller or weaker than blue team and on blue team otherwise
	t = (teamSize [1] < teamSize [0]) || ((teamSize [1] == teamSize [0]) && (teamScore [1] < teamScore [0]));
	SetTeam (nPlayer, t);
	MultiSendSetTeam (nPlayer);
	}
}

//-----------------------------------------------------------------------------

void AutoBalanceTeams (void)
{
if (gameStates.app.bHaveExtraGameInfo && extraGameInfo [1].bAutoBalanceTeams && IsTeamGame && IAmGameHost ()) {

		static int32_t nTimeout = 0;

	if (gameStates.app.nSDLTicks [0] - nTimeout < 1000)
		return;
	nTimeout = gameStates.app.nSDLTicks [0];

		int32_t	h, i, t, teamSize [2], teamScore [2];

	teamSize [0] =
	teamSize [1] =
	teamScore [0] =
	teamScore [1] = 0;
	for (i = 0; i < N_PLAYERS; i++) {
		t = GetTeam (i);
		teamSize [t]++;
		teamScore [t] = PLAYER (i).score;
		}
	h = teamSize [0] - teamSize [1];
	// don't change teams if 1 player or less difference
	if ((h  >= -1) &&(h <= 1))
		return;
	// don't change teams if smaller team is better
	if ((h > 1) && (teamScore [1] > teamScore [0]))
		return;
	if ((h < -1) && (teamScore [0] > teamScore [1]))
		return;
	// set id of the team to be reduced
	t = (h > 1) ? 0 : 1;
	if (h < 0)
		h = -h;
	for (i = 0; i < N_PLAYERS; i++) {
		if (GetTeam (i) != t)
			continue;
		if ((gameData.app.GameMode (GM_CAPTURE)) && (PLAYER (i).flags & PLAYER_FLAGS_FLAG))
			continue;
		SwitchTeam (i, 1);
		h -= 2; // one team grows and the other shrinks ...
		if (h <= 0)
			return;
		}
	}
}

//-----------------------------------------------------------------------------

extern void GameDisableCheats ();

void MultiNewGame (void)
{
	int32_t i;

memset (gameData.multigame.score.matrix, 0, MAX_NUM_NET_PLAYERS*MAX_NUM_NET_PLAYERS*2); // Clear kill matrix
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++) {
	gameData.multigame.score.nSorted [i] = i;
	PLAYER (i).netKilledTotal = 0;
	PLAYER (i).netKillsTotal = 0;
	PLAYER (i).flags = 0;
	PLAYER (i).nScoreGoalCount = 0;
	}
for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++) {
	gameData.multigame.robots.controlled [i] = -1;
	gameData.multigame.robots.agitation [i] = 0;
	gameData.multigame.robots.fired [i] = 0;
	}
gameData.multigame.score.nTeam [0] = gameData.multigame.score.nTeam [1] = 0;
gameStates.app.bEndLevelSequence = 0;
gameStates.app.bPlayerIsDead = 0;
gameData.multigame.nWhoKilledCtrlcen = -1;  // -1 = noone
//do we draw the kill list on the HUD?
gameData.multigame.score.bShowList = 1;
gameData.multigame.bShowReticleName = 1;
gameData.multigame.score.xShowListTimer = 0;
gameData.multigame.bIsGuided = 0;
gameData.multigame.create.nCount = 0;       // pointer into previous array
gameData.multigame.weapon.bFired = 0;  // How many times we shot
gameData.multigame.msg.bSending = 0;
gameData.multigame.msg.bDefining = 0;
gameData.multigame.msg.nIndex = 0;
gameData.multigame.msg.nReceiver = -1;
gameData.multigame.bGotoSecret = 0;
gameData.multigame.menu.bInvoked = 0;
gameData.multigame.menu.bLeave = 0;
gameData.multigame.bQuitGame = 0;
GameDisableCheats ();
LOCALPLAYER.m_bExploded = 0;
gameData.objData.deadPlayerCamera = 0;
}

//-----------------------------------------------------------------------------

void MultiMakePlayerGhost (int32_t nPlayer)
{
#if 0
if ((nPlayer == N_LOCALPLAYER) || (nPlayer >= MAX_NUM_NET_PLAYERS) || (nPlayer < 0)) {
	Int3 (); // Non-terminal, see Rob
	return;
	}
#endif

CObject *objP = OBJECT (PLAYER (nPlayer).nObject);
if (objP && (objP->Type () == OBJ_PLAYER)) {
	objP->SetType (OBJ_GHOST);
	objP->info.renderType = RT_NONE;
	objP->info.movementType = MT_NONE;
	MultiResetPlayerObject (objP);
	}
if (gameData.app.GameMode (GM_MULTI_ROBOTS))
	MultiStripRobots (nPlayer);
}

//-----------------------------------------------------------------------------

void MultiMakeGhostPlayer (int32_t nPlayer)
{
#if 0
if ((nPlayer == N_LOCALPLAYER) || (nPlayer >= MAX_NUM_NET_PLAYERS)) {
	Int3 (); // Non-terminal, see rob
	return;
	}
#endif

CObject *objP = OBJECT (PLAYER (nPlayer).nObject);
if (objP && (objP->Type () == OBJ_GHOST)) {
	objP->SetType (OBJ_PLAYER);
	objP->info.movementType = MT_PHYSICS;
	MultiResetPlayerObject (objP);
	}
}

//-----------------------------------------------------------------------------
// Returns the number of active net players and their sorted order of kills

int32_t MultiGetKillList (int32_t *listP)
{
for (int32_t i = 0; i < N_PLAYERS; i++)
	listP [i] = gameData.multigame.score.nSorted [i];
return  N_PLAYERS;
}

//-----------------------------------------------------------------------------

static void QSortScoreList (int32_t* score, int32_t left, int32_t right)
{
	int32_t	l = left,
			r = right,
			m = score [(l + r) / 2];

do {
	while (score [l] > m)
		l++;
	while (score [r] < m)
		r--;
	if (l <= r) {
		if (l < r)
			Swap (score [l], score [r]);
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	QSortScoreList (score, l, right);
if (left < r)
	QSortScoreList (score, left, r);
};

//-----------------------------------------------------------------------------
// Sort the kills list each time a new kill is added

void MultiSortKillList (void)
{

	int32_t score [MAX_PLAYERS];
	int32_t i;

for (i = 0; i < MAX_NUM_NET_PLAYERS; i++) {
	if (IsCoopGame)
		score [i] = PLAYER (i).score;
	else if (gameData.multigame.score.bShowList != 2) 
		score [i] = PLAYER (i).netKillsTotal;
	else {
		if (PLAYER (i).netKilledTotal + PLAYER (i).netKillsTotal)
			score [i] = int32_t (double (PLAYER (i).netKillsTotal) / 
								  double (PLAYER (i).netKilledTotal + PLAYER (i).netKillsTotal) / 100.0);
		else
			score [i] = -1;  // always draw the ones without any ratio last
		}
	}
QSortScoreList (score, 0, N_PLAYERS - 1);
}

//-----------------------------------------------------------------------------

void MultiComputeKill (int32_t nKiller, int32_t nKilled, int32_t nKillerPlayer = -1)
{
	// Figure out the results of a network kills and add it to the
	// appropriate player's tally.

	int32_t			nKilledPlayer, killedType, t0, t1;
	int32_t			killerType, nKillerId;
	char				szKilled [CALLSIGN_LEN * 2 + 4];
	char				szKiller [CALLSIGN_LEN * 2 + 4];
	CPlayerData*	killerP, * killedP;
	CObject*			objP;

gameData.score.nKillsChanged = 1;
gameStates.multi.bSuicide = 0;

// Both CObject numbers are localized already!
if ((nKilled < 0) || (nKilled > gameData.objData.nLastObject [0]) ||
	 (nKiller < 0) || (nKiller > gameData.objData.nLastObject [0])) {
	Int3 (); // See Rob, illegal value passed to computeKill;
	return;
	}
objP = OBJECT (nKilled);
killedType = objP->info.nType;
nKilledPlayer = objP->info.nId;
if ((nKillerPlayer >= 0) && (nKillerPlayer < N_PLAYERS)) {
	nKiller = PLAYER (nKillerPlayer).nObject;
	objP = OBJECT (nKiller);
	}
else {
	objP = OBJECT (nKiller);
	nKillerPlayer = objP->info.nId;
	}
killerType = objP->info.nType;
nKillerId = objP->info.nId;
if ((killedType != OBJ_PLAYER) && (killedType != OBJ_GHOST)) {
	Int3 (); // computeKill passed non-player object!
	return;
	}
killedP = gameData.multiplayer.players + nKilledPlayer;
gameData.multigame.score.pFlags [nKilledPlayer] = 1;
if (IsTeamGame)
	sprintf (szKilled, "%s (%s)", killedP->callsign, netGameInfo.m_info.szTeamName [GetTeam (nKilledPlayer)]);
else
	sprintf (szKilled, "%s", killedP->callsign);
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordMultiDeath (nKilledPlayer);
audio.PlaySound (SOUND_HUD_KILL, SOUNDCLASS_GENERIC, I2X (3));
if (gameData.reactor.bDestroyed)
	killedP->Connect (CONNECT_DIED_IN_MINE);
if (killerType == OBJ_REACTOR) {
	killedP->netKilledTotal++;
	killedP->netKillsTotal--;
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordMultiKill (nKilledPlayer, -1);
	if (nKilledPlayer == N_LOCALPLAYER) {
		HUDInitMessage ("%s %s.", TXT_YOU_WERE, TXT_KILLED_BY_NONPLAY);
		MultiAddLifetimeKilled ();
		}
	else
		HUDInitMessage ("%s %s %s.", szKilled, TXT_WAS, TXT_KILLED_BY_NONPLAY);
	return;
	}
else if ((killerType != OBJ_PLAYER) && (killerType != OBJ_GHOST)) {
	if ((nKillerId == SMALLMINE_ID) && (killerType != OBJ_ROBOT)) {
		if (nKilledPlayer == N_LOCALPLAYER)
			HUDInitMessage (TXT_MINEKILL);
		else
			HUDInitMessage (TXT_MINEKILL2, szKilled);
		}
	else {
		if (nKilledPlayer == N_LOCALPLAYER) {
			HUDInitMessage ("%s %s.", TXT_YOU_WERE, TXT_KILLED_BY_ROBOT);
			MultiAddLifetimeKilled ();
			}
		else
			HUDInitMessage ("%s %s %s.", szKilled, TXT_WAS, TXT_KILLED_BY_ROBOT);
		}
	killedP->netKilledTotal++;
	return;
	}
killerP = gameData.multiplayer.players + nKillerPlayer;
if (IsTeamGame)
	sprintf (szKiller, "%s (%s)", killerP->callsign, netGameInfo.m_info.szTeamName [GetTeam (nKillerPlayer)]);
else
	sprintf (szKiller, "%s", killerP->callsign);
// Beyond this point, it was definitely a player-player kill situation
#if DBG
if ((nKillerPlayer < 0) || (nKillerPlayer  >= N_PLAYERS))
	Int3 (); // See rob, tracking down bug with kill HUD messages
if ((nKilledPlayer < 0) || (nKilledPlayer  >= N_PLAYERS))
	Int3 (); // See rob, tracking down bug with kill HUD messages
#endif
t0 = GetTeam (nKilledPlayer);
t1 = GetTeam (nKillerPlayer);
if (nKillerPlayer == nKilledPlayer) {
	if (!(gameData.app.nGameMode &(GM_HOARD | GM_ENTROPY))) {
		if (IsTeamGame)
			gameData.multigame.score.nTeam [GetTeam (nKilledPlayer)]--;
		PLAYER (nKilledPlayer).netKilledTotal++;
		PLAYER (nKilledPlayer).netKillsTotal--;
		if (gameData.demo.nState == ND_STATE_RECORDING)
			NDRecordMultiKill (nKilledPlayer, -1);
		}
	gameData.multigame.score.matrix [nKilledPlayer][nKilledPlayer]++; // # of suicides
	if (nKillerPlayer == N_LOCALPLAYER) {
		HUDInitMessage ("%s %s %s!", TXT_YOU, TXT_KILLED, TXT_YOURSELF);
		gameStates.multi.bSuicide = 1;
		MultiAddLifetimeKilled ();
		}
	else
		HUDInitMessage ("%s %s", szKilled, TXT_SUICIDE);
	}
else {
	if (IsHoardGame) {
		if (IsTeamGame) {
			if ((nKilledPlayer == N_LOCALPLAYER) && (t0 == t1))
				gameStates.multi.bSuicide = 1;
			}
		}
	else if (IsEntropyGame) {
		if (t0 == t1) {
			if (nKilledPlayer == N_LOCALPLAYER)
				gameStates.multi.bSuicide = 1;
			}
		else {
			killerP->secondaryAmmo [SMARTMINE_INDEX] += extraGameInfo [1].entropy.nBumpVirusCapacity;
			if (extraGameInfo [1].entropy.nMaxVirusCapacity)
				if (PLAYER (nKillerPlayer).secondaryAmmo [SMARTMINE_INDEX] > extraGameInfo [1].entropy.nMaxVirusCapacity)
					PLAYER (nKillerPlayer).secondaryAmmo [SMARTMINE_INDEX] = extraGameInfo [1].entropy.nMaxVirusCapacity;
			}
		}
	else if (IsTeamGame) {
		if (t0 == t1) {
			gameData.multigame.score.nTeam [t0]--;
			killerP->netKillsTotal--;
			killerP->nScoreGoalCount--;
			}
		else {
			gameData.multigame.score.nTeam [t1]++;
			killerP->netKillsTotal++;
			killerP->nScoreGoalCount++;
			}
		}
	else {
		killerP->netKillsTotal++;
		killerP->nScoreGoalCount++;
		}
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordMultiKill (nKillerPlayer, 1);
	gameData.multigame.score.matrix [nKillerPlayer][nKilledPlayer]++;
	killedP->netKilledTotal++;
	if (nKillerPlayer == N_LOCALPLAYER) {
		HUDInitMessage ("%s %s %s!", TXT_YOU, TXT_KILLED, szKilled);
		MultiAddLifetimeKills ();
		if (IsCoopGame && (LOCALPLAYER.score  >= 1000))
			cockpit->AddPointsToScore (-1000);
		}
	else if (nKilledPlayer == N_LOCALPLAYER) {
		HUDInitMessage ("%s %s %s!", szKiller, TXT_KILLED, TXT_YOU);
		MultiAddLifetimeKilled ();
		if (IsHoardGame) {
			if (LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX] > 3)
				MultiSendPlayByPlay (1, nKillerPlayer, N_LOCALPLAYER);
			else if (LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX] > 0)
				MultiSendPlayByPlay (0, nKillerPlayer, N_LOCALPLAYER);
			}
		}
	else
		HUDInitMessage ("%s %s %s!", szKiller, TXT_KILLED, szKilled);
	}
CheckScoreGoal (nKillerPlayer);
MultiSortKillList ();
MultiShowPlayerList ();
killedP->flags &= (~(PLAYER_FLAGS_HEADLIGHT_ON));  // clear the nKilled guys flags/headlights
}

//-----------------------------------------------------------------------------

void MultiSyncMonsterball (void)
{
if ((gameData.app.GameMode (GM_MONSTERBALL)) && gameData.hoard.monsterballP && IAmGameHost ()) {
	static time_t	t0 = 0;

	if (gameStates.app.nSDLTicks [0] - t0 > 250) {
		t0 = gameStates.app.nSDLTicks [0];
		gameData.multigame.msg.buf [0] = MULTI_SYNC_MONSTERBALL;
		int32_t i = 1;
		PUT_INTEL_SHORT (gameData.multigame.msg.buf + i, gameData.hoard.monsterballP->info.nSegment);
		i += sizeof (int16_t);
		memcpy (gameData.multigame.msg.buf + i, &gameData.hoard.monsterballP->info.position.vPos, sizeof (gameData.hoard.monsterballP->info.position.vPos));
		i += sizeof (gameData.hoard.monsterballP->info.position.vPos);
		memcpy (gameData.multigame.msg.buf + i, &gameData.hoard.monsterballP->info.position.mOrient, sizeof (gameData.hoard.monsterballP->info.position.mOrient));
		i += sizeof (gameData.hoard.monsterballP->info.position.mOrient);
		memcpy (gameData.multigame.msg.buf + i, &gameData.hoard.monsterballP->mType.physInfo.velocity, sizeof (gameData.hoard.monsterballP->mType.physInfo.velocity));
		i += sizeof (gameData.hoard.monsterballP->mType.physInfo.velocity);
		memcpy (gameData.multigame.msg.buf + i, &gameData.hoard.monsterballP->mType.physInfo.thrust, sizeof (gameData.hoard.monsterballP->mType.physInfo.thrust));
		i += sizeof (gameData.hoard.monsterballP->mType.physInfo.thrust);
		memcpy (gameData.multigame.msg.buf + i, &gameData.hoard.monsterballP->mType.physInfo.rotVel, sizeof (gameData.hoard.monsterballP->mType.physInfo.rotVel));
		i += sizeof (gameData.hoard.monsterballP->mType.physInfo.rotVel);
		memcpy (gameData.multigame.msg.buf + i, &gameData.hoard.monsterballP->mType.physInfo.rotThrust, sizeof (gameData.hoard.monsterballP->mType.physInfo.rotThrust));
		i += sizeof (gameData.hoard.monsterballP->mType.physInfo.rotThrust);
		MultiSendData (gameData.multigame.msg.buf, i, 0);
		}
	}
}

//-----------------------------------------------------------------------------

void MultiDoSyncMonsterball (uint8_t* buf)
{
if ((gameData.app.GameMode (GM_MONSTERBALL)) && !IAmGameHost ()) {
	CFixVector	vPos;
	bool			bSync = false;
	int32_t		bufP = 1;

	int16_t nSegment = GET_INTEL_SHORT (buf + bufP);
	if (nSegment >= 0) {
		if (!FindMonsterball ()) {
			gameData.hoard.nMonsterballSeg = nSegment;
			if (!CreateMonsterball ())
				return;
			bSync = true;
			}
		bufP += sizeof (int16_t);
		memcpy (&vPos, buf + bufP, sizeof (gameData.hoard.monsterballP->info.position.vPos));
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
		INTEL_VECTOR (vPos);
#endif
		if (bSync || (CFixVector::Dist (vPos, gameData.hoard.monsterballP->info.position.vPos) > I2X (20))) {
			gameData.hoard.monsterballP->info.position.vPos = vPos;
			bufP += sizeof (gameData.hoard.monsterballP->info.position.vPos);
			memcpy (&gameData.hoard.monsterballP->info.position.mOrient, buf + bufP, sizeof (gameData.hoard.monsterballP->info.position.mOrient));
			bufP += sizeof (gameData.hoard.monsterballP->info.position.mOrient);
			memcpy (&gameData.hoard.monsterballP->mType.physInfo.velocity, buf + bufP, sizeof (gameData.hoard.monsterballP->mType.physInfo.velocity));
			bufP += sizeof (gameData.hoard.monsterballP->mType.physInfo.velocity);
			memcpy (&gameData.hoard.monsterballP->mType.physInfo.rotThrust, buf + bufP, sizeof (gameData.hoard.monsterballP->mType.physInfo.thrust));
			bufP += sizeof (gameData.hoard.monsterballP->mType.physInfo.thrust);
			memcpy (&gameData.hoard.monsterballP->mType.physInfo.rotVel, buf + bufP, sizeof (gameData.hoard.monsterballP->mType.physInfo.rotVel));
			bufP += sizeof (gameData.hoard.monsterballP->mType.physInfo.rotVel);
			memcpy (&gameData.hoard.monsterballP->mType.physInfo.rotThrust, buf + bufP, sizeof (gameData.hoard.monsterballP->mType.physInfo.rotThrust));
			bufP += sizeof (gameData.hoard.monsterballP->mType.physInfo.rotThrust);
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
			INTEL_MATRIX (gameData.hoard.monsterballP->info.position.mOrient);
			INTEL_VECTOR (gameData.hoard.monsterballP->mType.physInfo.velocity);
			INTEL_VECTOR (gameData.hoard.monsterballP->mType.physInfo.rotThrust);
			INTEL_VECTOR (gameData.hoard.monsterballP->mType.physInfo.rotVel);
			INTEL_VECTOR (gameData.hoard.monsterballP->mType.physInfo.rotThrust);
#endif
			gameData.hoard.monsterballP->RelinkToSeg (nSegment);
			}
		}
	}
}

//-----------------------------------------------------------------------------

void MultiDoFrame (void)
{
	static int32_t lasttime = 0;
	int32_t i;

if (!IsMultiGame) {
	Int3 ();
	return;
	}

if (IsNetworkGame && netGameInfo.GetPlayTimeAllowed () && (lasttime != X2I (gameStates.app.xThisLevelTime))) {
	for (i = 0; i < N_PLAYERS; i++)
		if (PLAYER (i).IsConnected ()) {
			if (i == N_LOCALPLAYER) {
				MultiSendHeartBeat ();
				lasttime = X2I (gameStates.app.xThisLevelTime);
				}
			break;
			}
		}
MultiSendMessage (); // Send any waiting messages
if (!gameData.multigame.menu.bInvoked)
	gameData.multigame.menu.bLeave = 0;
if (gameData.app.GameMode (GM_MULTI_ROBOTS)) {
	MultiCheckRobotTimeout ();
	}
NetworkDoFrame ();
if (gameData.multigame.bQuitGame && !(gameData.multigame.menu.bInvoked || gameStates.menus.nInMenu)) {
	gameData.multigame.bQuitGame = 0;
	longjmp (gameExitPoint, 0);
	}
MultiAdjustPowerupCap ();
MultiSyncMonsterball ();
}

//-----------------------------------------------------------------------------

void MultiSendData (uint8_t* buf, int32_t len, int32_t bUrgent)
{
if (IsNetworkGame && buf [0]) {
#if 1// DBG
	if (len != MultiMsgLen (buf [0])) {
		PrintLog (0, "Network message %d sent with wrong size %d!\n", buf [0], len);
		len = MultiMsgLen (buf [0]);
		}
#endif
	NetworkSendData (reinterpret_cast<uint8_t*> (buf), len, bUrgent);
	}
}

//-----------------------------------------------------------------------------

void MultiLeaveGame (void)
{
if (!IsMultiGame)
	return;
if (IsNetworkGame) {
	gameData.multigame.create.nCount = 0;
	AdjustMineSpawn ();
	MultiCapObjects ();
	gameStates.app.SRand ();
	fix shield = LOCALPLAYER.Shield ();
	LOCALPLAYER.SetShield (-1);
	gameStates.app.SRand (); // required to sync the player ship explosion on the clients
	DropPlayerEggs (gameData.objData.consoleP);
	LOCALPLAYER.SetShield (shield);
	MultiSendPlayerExplode (MULTI_PLAYER_DROP);
	}
MultiSendQuit (MULTI_QUIT);
#if 0
if (N_LOCALPLAYER != 0) {
	memcpy (&PLAYER (0), &LOCALPLAYER, sizeof (LOCALPLAYER));
	N_LOCALPLAYER = 0;
	}
#endif
ResetPlayerData (true, false, false, -1);
if (IsNetworkGame)
	NetworkLeaveGame ();
gameData.app.nGameMode |= GM_GAME_OVER;
if (gameStates.app.nFunctionMode != FMODE_EXIT)
	SetFunctionMode (FMODE_MENU);
}

//-----------------------------------------------------------------------------

void MultiShowPlayerList (void)
{
if (!IsMultiGame || IsCoopGame)
	return;
if (gameData.multigame.score.bShowList)
	return;
gameData.multigame.score.xShowListTimer = I2X (5); // 5 second timer
gameData.multigame.score.bShowList = 1;
}

//-----------------------------------------------------------------------------

int32_t MultiEndLevel (int32_t *secret)
{
return IsNetworkGame ? NetworkEndLevel (secret) : 0;
}

//
// Part 2 : functions that act on network/serial messages and change
//          the state of the game in some way.
//

//-----------------------------------------------------------------------------

int32_t MultiMenuPoll (void)
{
// Special polling function for in-game menus for multiplayer and serial
if (!(IsMultiGame && (gameStates.app.nFunctionMode == FMODE_GAME)))
	return 0;
if (gameData.multigame.menu.bLeave)
	return -1;

	int32_t bReactorWasAlive = gameData.reactor.bDestroyed;
	int32_t bPlayerWasDead = gameStates.app.bPlayerIsDead;
	fix xOldShield = (N_LOCALPLAYER < 0) ? -1 : LOCALPLAYER.Shield ();

if (!gameOpts->menus.nStyle && gameStates.app.bGameRunning) {
	gameData.multigame.menu.bInvoked++; // Track level of menu nesting
	GameFrame (0, -1, 40);
	gameData.multigame.menu.bInvoked--;
	G3_SLEEP (100);   // delay 100 milliseconds
	}
if (gameStates.app.bEndLevelSequence ||
	 (gameData.reactor.bDestroyed && !bReactorWasAlive) ||
	 (gameStates.app.bPlayerIsDead != bPlayerWasDead) ||
	 ((N_LOCALPLAYER >= 0) && (LOCALPLAYER.Shield () < xOldShield))) {
	gameData.multigame.menu.bLeave = 1;
	return -1;
	}
if (gameData.reactor.bDestroyed && (gameData.reactor.countdown.nSecsLeft < 10)) {
	gameData.multigame.menu.bLeave = 1;
	return -1;
	}
return 0;
}

//-----------------------------------------------------------------------------
// Do any miscellaneous stuff for a new network player after death

void MultiDoDeath (int32_t nObject)
{
if (!IsCoopGame)
	LOCALPLAYER.flags |= (PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_GOLD_KEY);
}

//-----------------------------------------------------------------------------

void MultiDoFire (uint8_t* buf)
{
	uint8_t nPlayer = int32_t (buf [1]);
	uint8_t weapon = uint8_t (buf [2]);
	int32_t flags = int32_t (buf [4]);

gameData.multigame.weapon.nTrack = GET_INTEL_SHORT (buf + 6);

#if 0
if (gameStates.multi.nGameType == UDP_GAME) {
	gameData.multigame.weapon.nFired [1] = buf [8];
	for (int32_t i = 0; i < gameData.multigame.weapon.nFired [1]; i++)
		gameData.multigame.weapon.nObjects [1][i] = GET_INTEL_SHORT (buf + 9 + i * sizeof (int16_t));
	}
#endif

if (PLAYEROBJECT (nPlayer)->info.nType == OBJ_GHOST)
	MultiMakeGhostPlayer (nPlayer);
if (weapon == FLARE_ADJUST)
	LaserPlayerFire (OBJECT (PLAYER (nPlayer).nObject), FLARE_ID, 6, 1, 0, -1);
else if (weapon >= MISSILE_ADJUST) {
	int32_t h = weapon - MISSILE_ADJUST;
	uint8_t weaponId = secondaryWeaponToWeaponInfo [h];
	int32_t weaponGun = secondaryWeaponToGunNum [h] + (flags & 1);
	CPlayerInfo *playerP = gameData.multiplayer.players + nPlayer;
	if (h == GUIDED_INDEX)
		gameData.multigame.bIsGuided = 1;
	if (playerP->secondaryAmmo [h] > 0)
		playerP->secondaryAmmo [h]--;
	LaserPlayerFire (OBJECT (PLAYER (nPlayer).nObject), weaponId, weaponGun, 1, 0, -1);
	}
else {
	fix xSaveCharge = gameData.FusionCharge ();
	if (weapon == FUSION_INDEX)
		gameData.SetFusionCharge (flags << 12, true);
	else if (weapon == LASER_ID) {
		if (flags & LASER_QUAD)
			PLAYER (nPlayer).flags |= PLAYER_FLAGS_QUAD_LASERS;
		else
			PLAYER (nPlayer).flags &= ~PLAYER_FLAGS_QUAD_LASERS;
		}
	FireWeapon (int16_t (PLAYER (nPlayer).nObject), weapon, int32_t (buf [3]), flags, int32_t (buf [5]));
	gameData.SetFusionCharge (xSaveCharge, true);
	}
gameData.multigame.weapon.nFired [0] =
gameData.multigame.weapon.nFired [1] = 0;
}

//-----------------------------------------------------------------------------
// This routine does only player positions, mode game only

void MultiDoPosition (uint8_t* buf)
{
if (IsNetworkGame) 
	return;

int32_t bufP = 0;
if (gameStates.multi.nGameType == UDP_GAME) {
	CObject* objP = OBJECT (PLAYER (buf [bufP++]).nObject);
	memcpy (&objP->Position (), buf + bufP, sizeof (CFixVector));
	bufP += sizeof (CFixVector);
	memcpy (&objP->Orientation (), buf + bufP, sizeof (CFixMatrix));
	bufP += sizeof (CFixMatrix);
	memcpy (&objP->Velocity (), buf + bufP, sizeof (CFixMatrix));
	bufP += sizeof (CFixMatrix);
#if (defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
	INTEL_VECTOR (objP->Position ());
	INTEL_MATRIX (objP->Orientation ());
	INTEL_VECTOR (objP->Velocity ());
#endif
	}
else {
	CObject *objP = OBJECT (PLAYER ((N_LOCALPLAYER + 1) % 2).nObject);
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
	ExtractShortPos (objP, reinterpret_cast<tShortPos*> (buf + 1), 0);
#else
	tShortPos sp;
	memcpy (reinterpret_cast<uint8_t*> (sp.orient), reinterpret_cast<uint8_t*> (buf + 1), 9);
	memcpy (reinterpret_cast<uint8_t*> (&sp.coord), reinterpret_cast<uint8_t*> (buf + 10), 14);
	ExtractShortPos (PLAYEROBJECT (nPlayer), &sp, 1);
	if (objP->info.movementType == MT_PHYSICS)
		objP->SetThrustFromVelocity ();
#endif
	}
}

//-----------------------------------------------------------------------------

void MultiDoReappear (uint8_t* buf)
{
CObject *objP = OBJECT (GET_INTEL_SHORT (buf + 1));
if (objP) {
	MultiMakeGhostPlayer (objP->info.nId);
	objP->CreateAppearanceEffect ();
	gameData.multigame.score.pFlags [objP->info.nId] = 0;
	}
#if DBG
else
	BRP;
#endif
}

//-----------------------------------------------------------------------------

void MultiDestroyPlayerShip (int32_t nPlayer, int32_t bExplode, int32_t nRemoteCreated, int16_t* objList)
{
CPlayerData* playerP = gameData.multiplayer.players + nPlayer;
CObject* objP = OBJECT (playerP->nObject);
fix shield = playerP->Shield ();
playerP->SetShield (-1, false);
gameData.multigame.create.nCount = 0; // this variable gets incremented everytime a powerup is created; use it to find out how many powerups actually get created
DropPlayerEggs (objP);
playerP->SetShield (shield);
// Create mapping from remote to local numbering system
int32_t i;
for (i = 0; (i < nRemoteCreated) && (i < gameData.multigame.create.nCount); i++) {
	int16_t nRemoteObj = GET_INTEL_SHORT (objList + i);
	if (nRemoteObj > 0)
		SetObjNumMapping ((int16_t) gameData.multigame.create.nObjNums [i], nRemoteObj, nPlayer);
	}
for (; i < gameData.multigame.create.nCount; i++)
	OBJECT (gameData.multigame.create.nObjNums [i])->Die ();
if (bExplode) {
	KillPlayerSmoke (nPlayer);
	objP->ExplodeSplashDamagePlayer ();
	objP->info.nFlags &= ~OF_SHOULD_BE_DEAD;              //don't really kill player
	MultiMakePlayerGhost (nPlayer);
	}
else
	objP->CreateAppearanceEffect ();
playerP->flags &= ~(PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE | PLAYER_FLAGS_FLAG);
playerP->cloakTime = 0;
playerP->m_bExploded = 1;
}

//-----------------------------------------------------------------------------
// Only call this for players, not robots.  nPlayer is player number, not
// Object number.

void MultiDoPlayerExplode (uint8_t* buf)
{
int32_t bufP = 1;

CHECK_MSG_ID

int32_t nPlayer = (int32_t) buf [bufP++];
#if !DBG
if ((nPlayer < 0) || (nPlayer >= N_PLAYERS))
	return;
#endif
// If we are in the process of sending OBJECTS to a new player, reset that process
NetworkResetObjSync (-1);
// Stuff the gameData.multiplayer.players structure to prepare for the explosion
gameData.multiplayer.weaponStates [nPlayer].nAmmoUsed = 0;
CPlayerData* playerP = gameData.multiplayer.players + nPlayer;
playerP->primaryWeaponFlags = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
playerP->secondaryWeaponFlags = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
playerP->ComputeLaserLevels (buf [bufP++]);
playerP->secondaryAmmo [HOMING_INDEX] = buf [bufP++];
playerP->secondaryAmmo [CONCUSSION_INDEX] = buf [bufP++];
playerP->secondaryAmmo [SMART_INDEX] = buf [bufP++];
playerP->secondaryAmmo [MEGA_INDEX] = buf [bufP++];
playerP->secondaryAmmo [PROXMINE_INDEX] = buf [bufP++];
playerP->secondaryAmmo [FLASHMSL_INDEX] = buf [bufP++];
playerP->secondaryAmmo [GUIDED_INDEX] = buf [bufP++];
playerP->secondaryAmmo [SMARTMINE_INDEX] = buf [bufP++];
playerP->secondaryAmmo [MERCURY_INDEX] = buf [bufP++];
playerP->secondaryAmmo [EARTHSHAKER_INDEX] = buf [bufP++];
playerP->primaryAmmo [VULCAN_INDEX] = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
playerP->primaryAmmo [GAUSS_INDEX] = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
playerP->flags = GET_INTEL_INT (buf + bufP);
bufP += 4;
if (gameStates.multi.nGameType == UDP_GAME) {
	playerP->SetLaserLevels (buf [bufP], buf [bufP + 1]);
	bufP += 2;
	gameStates.app.nRandSeed = (uint32_t) GET_INTEL_INT (buf + bufP);
	bufP += 4;
#if 1
	CObject* objP = PLAYEROBJECT (nPlayer);
	objP->Velocity ().Set (GET_INTEL_INT (buf + bufP), GET_INTEL_INT (buf + bufP + 4), GET_INTEL_INT (buf + bufP + 8));
	bufP += 12;
	objP->Position ().Set (GET_INTEL_INT (buf + bufP), GET_INTEL_INT (buf + bufP + 4), GET_INTEL_INT (buf + bufP + 8));
	bufP += 12;
	objP->SetSegment (GET_INTEL_SHORT (buf + bufP));
	bufP += 2;
#endif
	}
#if 0
MultiAdjustRemoteCap (nPlayer);
#endif
playerP->m_tWeaponInfo = 0; // keep the PSALM from kicking in before updated player weapon info is available
MultiDestroyPlayerShip (nPlayer, buf [0] == MULTI_PLAYER_EXPLODE, buf [bufP], (int16_t*) (buf + bufP + 1));
}

//-----------------------------------------------------------------------------

void MultiDoKill (uint8_t* buf)
{
int32_t bufP = 1;

CHECK_MSG_ID

int32_t nPlayer = int32_t (buf [bufP++]);
if ((nPlayer < 0) || (nPlayer >= N_PLAYERS)) {
	Int3 (); // Invalid player number nKilled
	return;
	}
int32_t nKilled = PLAYER (nPlayer).nObject;
PLAYER (nPlayer).SetShield (-1);
int32_t nKiller = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
if (nKiller < 0) 
	nPlayer = -1;
else
	nKiller = GetLocalObjNum (nKiller, int8_t (nPlayer = int32_t (buf [bufP])));
#if DBG
MultiComputeKill (nKiller, nKilled);
#else
MultiComputeKill (nKiller, nKilled, nPlayer);
#endif
}

//-----------------------------------------------------------------------------
// Changed by MK on 10/20/94 to send NULL as CObject to NetDestroyReactor if it got -1
// which means not a controlcen CObject, but contained in another CObject

void MultiDoDestroyReactor (uint8_t* buf)
{
	int32_t bufP = 1;

CHECK_MSG_ID

int16_t nObject = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
int32_t nPlayer = (int32_t) buf [bufP];
if (gameData.reactor.bDestroyed != 1) {
	if ((nPlayer < N_PLAYERS) && (nPlayer != N_LOCALPLAYER))
		HUDInitMessage ("%s %s", PLAYER (nPlayer).callsign, TXT_HAS_DEST_CONTROL);
	else if (nPlayer == N_LOCALPLAYER)
		HUDInitMessage (TXT_YOU_DEST_CONTROL);
	else
		HUDInitMessage (TXT_CONTROL_DESTROYED);
	if (nObject != -1)
		NetDestroyReactor (OBJECT (nObject));
	else
		NetDestroyReactor (NULL);
	}
}

//-----------------------------------------------------------------------------

void MultiDoCountdown (uint8_t* buf)
{
if (NetDestroyReactor (ObjFindFirstOfType (OBJ_REACTOR)))
	 InitCountdown (NULL, 1, GET_INTEL_INT (buf + 1));
}

//-----------------------------------------------------------------------------

void MultiDoEscape (uint8_t* buf)
{
	int32_t bufP = 1;

CHECK_MSG_ID

int32_t nPlayer = (int32_t) buf [bufP++];
uint8_t bSecret = buf [bufP];
int32_t nObject = PLAYER (nPlayer).nObject;
audio.PlaySound (SOUND_HUD_MESSAGE);
audio.DestroyObjectSound (nObject);
if (bSecret == 0) {
	HUDInitMessage ("%s %s", PLAYER (nPlayer).callsign, TXT_HAS_ESCAPED);
	if (IsNetworkGame)
		CONNECT (nPlayer, CONNECT_ESCAPE_TUNNEL);
	if (!gameData.multigame.bGotoSecret)
		gameData.multigame.bGotoSecret = 2;
	}
else if (bSecret == 1) {
	HUDInitMessage ("%s %s", PLAYER (nPlayer).callsign, TXT_HAS_FOUND_SECRET);
	if (IsNetworkGame)
		CONNECT (nPlayer, CONNECT_FOUND_SECRET);
	if (!gameData.multigame.bGotoSecret)
		gameData.multigame.bGotoSecret = 1;
	}
OBJECT (nObject)->CreateAppearanceEffect ();
MultiMakePlayerGhost (nPlayer);
}

//-----------------------------------------------------------------------------

void MultiDoRemoveObject (uint8_t* buf)
{
if ((gameStates.multi.nGameType == UDP_GAME) && (buf [8] == N_LOCALPLAYER))
	return;

int32_t bufP = 1;

CHECK_MSG_ID

int16_t nObject = GET_INTEL_SHORT (buf + bufP);
if (nObject < 1)
	return;
bufP += 2;
int8_t nObjOwner = buf [bufP++];
int16_t nLocalObj = GetLocalObjNum (nObject, nObjOwner); // translate to local nObject
if (nLocalObj < 0)
	return;
CObject* objP = OBJECT (nLocalObj);
if ((gameStates.multi.nGameType == UDP_GAME) && (objP->info.nType == OBJ_ROBOT)) {
	if (objP->cType.aiInfo.xDyingStartTime > 0)	// robot death sequence
		return;
	}
else if ((objP->info.nType != OBJ_POWERUP) && (objP->info.nType != OBJ_HOSTAGE))
	return;
if (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_EXPLODING | OF_DESTROYED))
	return;
NetworkResetObjSync (nLocalObj);
//if ((objP->info.nType == OBJ_POWERUP) && IsNetworkGame) 
//	RemovePowerupInMine (objP->info.nId);
if (objP->info.nType == OBJ_ROBOT)
	MultiDestroyRobot (objP);
else
	objP->Die (); // quick and painless
}

//-----------------------------------------------------------------------------

void MultiOnlyPlayerMsg (int32_t bMsgBox)
{
if (bMsgBox)
	TextBox (NULL, BG_STANDARD, 1, TXT_OK, TXT_ONLY_PLAYER);
else {
	char szMsg [100];

	audio.PlaySound (SOUND_HUD_MESSAGE);
	sprintf (szMsg, "%c%c%c%c%s", 1, 127 + 128, 63 + 128, 128, TXT_ONLY_PLAYER);
	HUDInitMessage (szMsg);
	}
}

//-----------------------------------------------------------------------------

void MultiDoQuit (uint8_t* buf)
{
if (IsNetworkGame) {
	audio.PlaySound (SOUND_HUD_MESSAGE);
	int32_t nPlayer = int32_t (buf [1]);
	HUDInitMessage ("%s %s", PLAYER (nPlayer).callsign, TXT_HAS_LEFT_THE_GAME);
	NetworkDisconnectPlayer (buf [1]);
	*PLAYER (nPlayer).callsign = '\0';
	ResetShipData (nPlayer, 0);
	if (gameData.multigame.menu.bInvoked || gameStates.menus.nInMenu)
		return;
	int32_t nPlayers = 0;
	for (int32_t i = 0; i < N_PLAYERS; i++)
		if (PLAYER (i).IsConnected ())
			nPlayers++;
	if (nPlayers == 1)
		MultiOnlyPlayerMsg (0);
	}
}

//-----------------------------------------------------------------------------

int8_t MultiDoPowerState (uint8_t* buf, uint32_t flag, bool bOn)
{
	int32_t bufP = 1;

CHECK_MSG_ID_RVAL(-1)

int32_t nPlayer = (int32_t) buf [bufP];
if (bOn) {
	PLAYER (nPlayer).flags |= flag;
	PLAYER (nPlayer).cloakTime = gameData.time.xGame;
	if (gameData.app.GameMode (GM_MULTI_ROBOTS))
		MultiStripRobots (nPlayer);
	}
else
	PLAYER (nPlayer).flags &= ~flag;
return nPlayer;
}

//-----------------------------------------------------------------------------

void MultiDoInvul (uint8_t* buf)
{
MultiDoPowerState (buf, PLAYER_FLAGS_INVULNERABLE, true);
}

//-----------------------------------------------------------------------------

void MultiDoDeInvul (uint8_t* buf)
{
MultiDoPowerState (buf, PLAYER_FLAGS_INVULNERABLE, false);
}

//-----------------------------------------------------------------------------

void MultiDoCloak (uint8_t* buf)
{
int8_t nPlayer = MultiDoPowerState (buf, PLAYER_FLAGS_CLOAKED, true);
if (nPlayer < 0)
	return;
AIDoCloakStuff ();
//if (gameData.app.GameMode (GM_MULTI_ROBOTS))
//	MultiStripRobots (nPlayer);
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordMultiCloak (nPlayer);
}

//-----------------------------------------------------------------------------

void MultiDoDeCloak (uint8_t* buf)
{
int8_t nPlayer = MultiDoPowerState (buf, PLAYER_FLAGS_CLOAKED, false);
if (nPlayer < 0)
	return;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordMultiDeCloak (nPlayer);
}

//-----------------------------------------------------------------------------

void MultiDoDoorOpen (uint8_t* buf)
{
	int32_t	bufP = 1;

CHECK_MSG_ID

int32_t nSegment = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
int8_t nSide = buf [bufP++];
uint8_t flag = buf [bufP++];

if ((nSegment < 0) || (nSegment > gameData.segData.nLastSegment) || (nSide < 0) || (nSide > 5)) {
	Int3 ();
	return;
	}
CSegment* segP = SEGMENT (nSegment);
CWall* wallP = segP->Wall (nSide);
if (!wallP) {  //Opening door on illegal CWall
	Int3 ();
	return;
	}
if (wallP->nType == WALL_BLASTABLE) {
	if (!(wallP->flags & WALL_BLASTED))
		segP->DestroyWall (nSide);
	return;
	}
else if (wallP->state != WALL_DOOR_OPENING) {
	segP->OpenDoor (nSide);
	wallP->flags = flag;
	}
else
	wallP->flags = flag;
}

//-----------------------------------------------------------------------------

void MultiDoCreateExplosion (uint8_t* buf)
{
int32_t nPlayer = (int32_t) buf [1];
CreateSmallFireballOnObject (PLAYEROBJECT (nPlayer), I2X (1), 1);
}

//-----------------------------------------------------------------------------

void MultiDoCtrlcenFire (uint8_t* buf)
{
	CFixVector	vTarget;
	char			nGun;
	int16_t		nObject;
	int32_t		i, count = 1;

memcpy (&vTarget, buf + count, 12);
count += 12;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)  // swap the vector to_target
vTarget.dir.coord.x = (fix)INTEL_INT ((int32_t) vTarget.dir.coord.x);
vTarget.dir.coord.y = (fix)INTEL_INT ((int32_t) vTarget.dir.coord.y);
vTarget.dir.coord.z = (fix)INTEL_INT ((int32_t) vTarget.dir.coord.z);
#endif
nGun = buf [count++];
nObject = GET_INTEL_SHORT (buf + count);
if ((nObject < 0) || (nObject > gameData.objData.nLastObject [0]))
	return;
if (0 <= (i = FindReactor (OBJECT (nObject))))
	CreateNewWeaponSimple (&vTarget, gameData.reactor.states [i].vGunPos + (int32_t) nGun, nObject, CONTROLCEN_WEAPON_NUM, 1);
}

//-----------------------------------------------------------------------------

void MultiDoCreateWeapon (uint8_t* buf)
{
	int16_t		nSegment;
	int16_t		nObject;
	int32_t		nLocalObj;
	int32_t		nPlayer;
	CFixVector	vPos, vVel;
	char			nId;
	int32_t		bufP = 1;

CHECK_MSG_ID

if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed)
	return;
nPlayer = int32_t (buf [bufP++]);
if (nPlayer == N_LOCALPLAYER)
	return;
nId = buf [bufP++];
nSegment = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
nObject = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
if ((nSegment < 0) || (nSegment > gameData.segData.nLastSegment)) {
	Int3 ();
	return;
	}
memcpy (&vPos, buf + bufP, sizeof (CFixVector));
bufP += sizeof (CFixVector);
memcpy (&vVel, buf + bufP, sizeof (CFixVector));
bufP += sizeof (CFixVector);
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
INTEL_VECTOR (vPos);
INTEL_VECTOR (vVel);
#endif
gameData.multigame.create.nCount = 0;
nLocalObj = CreateNewWeapon (&vVel, &vPos, nSegment, nPlayer, nId, 0);
if (nLocalObj < 0)
	return;
NetworkResetObjSync (nLocalObj);
CObject* objP = OBJECT (nLocalObj);
objP->info.position.vPos = vPos;
objP->mType.physInfo.velocity = vVel;
objP->RelinkToSeg (nSegment);
SetObjNumMapping (nLocalObj, nObject, nPlayer);
return;
}

//-----------------------------------------------------------------------------

int32_t MultiDoCreatePowerup (uint8_t* buf)
{
	int32_t		bufP = 1;

CHECK_MSG_ID_RVAL(-1)

if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed)
	return -1;
int32_t nPlayer = int32_t (buf [bufP++]);
if (nPlayer == N_LOCALPLAYER)
	return -1;
uint8_t powerupType = buf [bufP++];
int16_t nSegment = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
int16_t nObject = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
if ((nSegment < 0) || (nSegment > gameData.segData.nLastSegment)) {
	Int3 ();
	return -1;
	}
CFixVector	vPos;
memcpy (&vPos, buf + bufP, sizeof (CFixVector));
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
INTEL_VECTOR (vPos);
#endif
gameData.multigame.create.nCount = 0;
int32_t nLocalObj = PrepareObjectCreateEgg (OBJECT (PLAYER (nPlayer).nObject), 1, OBJ_POWERUP, powerupType, true);
if (nLocalObj < 0)
	return -1;
NetworkResetObjSync (nLocalObj);
CObject* objP = OBJECT (nLocalObj);
objP->info.position.vPos = vPos;
objP->mType.physInfo.velocity.SetZero ();
objP->RelinkToSeg (nSegment);
SetObjNumMapping (nLocalObj, nObject, nPlayer);
CreateExplosion (nSegment, vPos, I2X (5), ANIM_POWERUP_DISAPPEARANCE);
return nLocalObj;
}

//-----------------------------------------------------------------------------

void MultiDoDropPowerup (uint8_t* buf)
{
int32_t nObject = MultiDoCreatePowerup (buf);
if (nObject >= 0)
	OBJECT (nObject)->mType.physInfo.velocity = *reinterpret_cast<CFixVector*>(buf + 7 + sizeof (CFixVector));
}

//-----------------------------------------------------------------------------

void MultiDoPlaySound (uint8_t* buf)
{
	int32_t nPlayer = (int32_t) (buf [1]);
	int16_t nSound = (int32_t) (buf [2]);
	fix volume = (int32_t) (buf [3]) << 12;

if (!PLAYER (nPlayer).IsConnected ())
	return;
if ((PLAYER (nPlayer).nObject >= 0) &&
	 (PLAYER (nPlayer).nObject <= gameData.objData.nLastObject [0]))
	audio.CreateObjectSound (nSound, SOUNDCLASS_PLAYER, (int16_t) PLAYER (nPlayer).nObject, 0, volume);
}

//-----------------------------------------------------------------------------

void MultiDoScore (uint8_t* buf)
{
	int32_t nPlayer = (int32_t) (buf [1]);

if ((nPlayer < 0) || (nPlayer  >= N_PLAYERS)) {
	Int3 (); // Non-terminal, see rob
	return;
	}
if (gameData.demo.nState == ND_STATE_RECORDING) {
	int32_t score = GET_INTEL_INT (buf + 2);
	NDRecordMultiScore (nPlayer, score);
	}
PLAYER (nPlayer).score = GET_INTEL_INT (buf + 2);
MultiSortKillList ();
}

//-----------------------------------------------------------------------------

void MultiDoTrigger (uint8_t* buf)
{
	int32_t bufP = 1;
	
CHECK_MSG_ID

int32_t nPlayer = (int32_t) buf [bufP++];
if ((nPlayer < 0) || (nPlayer >= N_PLAYERS) || (nPlayer == N_LOCALPLAYER)) {
	Int3 (); // Got nTrigger from illegal nPlayer
	return;
	}
uint8_t nTrigger = buf [bufP++];
if ((nTrigger < 0) || (nTrigger  >= gameData.trigData.m_nTriggers [0])) {
	Int3 (); // Illegal nTrigger number in multiplayer
	return;
	}
int16_t nObject = (gameStates.multi.nGameType == UDP_GAME) ? GET_INTEL_SHORT (buf + bufP) : PLAYER (nPlayer).nObject;
CTrigger* trigP = GEOTRIGGER (nTrigger);
if (trigP)
	trigP->Operate (nObject, nPlayer, 0, 0);
}

//-----------------------------------------------------------------------------

void MultiDoShield (uint8_t* buf)
{
	int32_t	nPlayer = int32_t (buf [1]);
	int32_t	shield = GET_INTEL_INT (buf+2);

if ((nPlayer < 0) || (nPlayer  >= N_PLAYERS) || (nPlayer == N_LOCALPLAYER)) {
	Int3 (); 
	return;
	}
PLAYER (nPlayer).SetShield (shield, false);
}

//-----------------------------------------------------------------------------

void MultiDoObjTrigger (uint8_t* buf)
{
	int32_t bufP = 1;

CHECK_MSG_ID

int32_t nPlayer = (int32_t) buf [bufP++];
if ((nPlayer < 0) || (nPlayer  >= N_PLAYERS) || (nPlayer == N_LOCALPLAYER)) {
	Int3 (); // Got nTrigger from illegal nPlayer
	return;
	}

CTrigger* trigP = OBJTRIGGER ((int32_t) buf [bufP++]);
if (trigP)
	trigP->Operate (PLAYER (nPlayer).nObject, nPlayer, 0, 1);
}

//-----------------------------------------------------------------------------

void MultiDoDropMarker (uint8_t* buf)
{
	int32_t		nPlayer = int32_t (buf [1]);
	int32_t		nMsg = int32_t (buf [2]);
	int32_t		nMarker = nPlayer * 2 + nMsg;
	CFixVector	vPos;

if (nPlayer == N_LOCALPLAYER)  // my marker? don't set it down cuz it might screw up the orientation
	return;
vPos.v.coord.x = GET_INTEL_INT (buf + 3);
vPos.v.coord.y = GET_INTEL_INT (buf + 7);
vPos.v.coord.z = GET_INTEL_INT (buf + 11);
memcpy (markerManager.Message (2 * nPlayer + nMsg), buf + 15, 40);
markerManager.SetPosition (nMarker, vPos);
if ((markerManager.Objects (nMarker) != -1) &&
	 (OBJECT (markerManager.Objects (nMarker))->info.nType != OBJ_NONE) &&
	 (markerManager.Objects (nMarker) != 0))
	ReleaseObject (markerManager.Objects (nMarker));
markerManager.SetObject (nPlayer * 2 + nMsg, 
								 DropMarkerObject (vPos, LOCALOBJECT->info.nSegment, 
								 LOCALOBJECT->info.position.mOrient, 
								 (uint8_t) nMarker));
markerManager.SetOwner (nMarker, PLAYER (nPlayer).callsign);
}

//-----------------------------------------------------------------------------
// Update hit point status of a door

void MultiDoHostageDoorStatus (uint8_t* buf)
{
	int32_t	bufP = 1;

CHECK_MSG_ID

int16_t nWall = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
if ((!IS_WALL (nWall)) || (nWall > gameData.wallData.nWalls))
	return;
int16_t hps = GET_INTEL_INT (buf + bufP);
if (hps < 0)
	return;
CWall* wallP = WALL (nWall);
if (wallP->nType != WALL_BLASTABLE)
	return;
if (hps < wallP->hps)
	SEGMENT (wallP->nSegment)->DamageWall ((int16_t) wallP->nSide, wallP->hps - hps);
}

//-----------------------------------------------------------------------------

void MultiDoSaveGame (uint8_t* buf)
{
	int32_t bufP = 1;

CHECK_MSG_ID

	char desc [25];

char slot = buf [bufP++];
uint32_t id = GET_INTEL_INT (buf + bufP);
bufP += 4;
memcpy (desc, buf + bufP, 20);
MultiSaveGame (slot, id, desc);
}

//-----------------------------------------------------------------------------

void MultiDoRestoreGame (uint8_t* buf)
{
	int32_t bufP = 1;

CHECK_MSG_ID

char slot = buf [bufP++];
uint32_t id = GET_INTEL_INT (buf + bufP);
MultiRestoreGame (slot, id);
}

//-----------------------------------------------------------------------------

void MultiDoReqPlayer (uint8_t* buf)
{
	uint8_t nPlayer = *reinterpret_cast<uint8_t*> (buf+1);

if ((nPlayer == N_LOCALPLAYER) || (nPlayer == 255)) {
		tNetPlayerStats ps;

	ExtractNetPlayerStats (&ps, gameData.multiplayer.players + N_LOCALPLAYER);
	ps.nLocalPlayer = N_LOCALPLAYER;
	ps.messageType = MULTI_SEND_PLAYER;            // SET
	MultiSendData (reinterpret_cast<uint8_t*> (&ps), sizeof (tNetPlayerStats), 0);
	}
}

//-----------------------------------------------------------------------------
// Got a player packet from someone!!!

void MultiDoSendPlayer (uint8_t* buf)
{
tNetPlayerStats *p = reinterpret_cast<tNetPlayerStats*> (buf);
UseNetPlayerStats (gameData.multiplayer.players  + p->nLocalPlayer, p);
}

//-----------------------------------------------------------------------------
// A generic, emergency function to solve proszPlayerms that crop up
// when a player exits quick-out from the game because of a
// serial connection loss.  Fixes several weird bugs!

void MultiResetStuff (void)
{
DeadPlayerEnd ();
LOCALPLAYER.homingObjectDist = -I2X (1); // Turn off homing sound.
gameData.objData.deadPlayerCamera = 0;
gameStates.app.bEndLevelSequence = 0;
ResetRearView ();
}

//-----------------------------------------------------------------------------

void MultiResetPlayerObject (CObject *objP)
{
	int32_t i;

//Init physics for a non-console player
if (objP > OBJECT (gameData.objData.nLastObject [0]))
	return;
if ((objP->info.nType != OBJ_PLAYER) && (objP->info.nType != OBJ_GHOST))
	return;
objP->mType.physInfo.velocity.SetZero ();
objP->mType.physInfo.thrust.SetZero ();
objP->mType.physInfo.rotVel.SetZero ();
objP->mType.physInfo.rotThrust.SetZero ();
objP->mType.physInfo.brakes = objP->mType.physInfo.turnRoll = 0;
objP->mType.physInfo.mass = gameData.pig.ship.player->mass;
objP->mType.physInfo.drag = gameData.pig.ship.player->drag;
//      objP->mType.physInfo.flags &= ~ (PF_TURNROLL | PF_LEVELLING | PF_WIGGLE | PF_USES_THRUST);
objP->mType.physInfo.flags &= ~(PF_TURNROLL | PF_LEVELLING | PF_WIGGLE);
//Init render info
objP->info.renderType = (objP->info.nType == OBJ_GHOST) ? RT_NONE : RT_POLYOBJ;
objP->rType.polyObjInfo.nModel = gameData.pig.ship.player->nModel;               //what model is this?
objP->rType.polyObjInfo.nSubObjFlags = 0;         //zero the flags
for (i = 0; i < MAX_SUBMODELS; i++)
	objP->rType.polyObjInfo.animAngles [i].SetZero ();
//reset textures for this, if not player 0
MultiSetObjectTextures (objP);
// Clear misc
objP->info.nFlags = 0;
}

//-----------------------------------------------------------------------------

void MultiSetObjectTextures (CObject *objP)
{
if (!objP)
	return;

	int32_t			id, i, j;
	CPolyModel*		modelP = gameData.models.polyModels [0] + objP->ModelId ();
	tBitmapIndex*	bmiP;

id = (IsTeamGame ? GetTeam (objP->info.nId) : objP->info.nId % MAX_PLAYER_COLORS);
if (!id)
	objP->rType.polyObjInfo.nAltTextures = 0;
else 
	{
	bmiP = mpTextureIndex [--id];
	for (i = 0, j = modelP->FirstTexture (); i < N_PLAYER_SHIP_TEXTURES; i++, j++)
		bmiP [i] = gameData.pig.tex.objBmIndex [gameData.pig.tex.objBmIndexP [j]];
	bmiP [4] = gameData.pig.tex.objBmIndex [gameData.pig.tex.objBmIndexP [gameData.pig.tex.nFirstMultiBitmap + 2 * id]];
	bmiP [5] = gameData.pig.tex.objBmIndex [gameData.pig.tex.objBmIndexP [gameData.pig.tex.nFirstMultiBitmap + 2 * id + 1]];
	objP->rType.polyObjInfo.nAltTextures = id + 1;
	}
}

//-----------------------------------------------------------------------------

#ifdef NETPROFILING
extern int32_t TTRecv [];
extern FILE *RecieveLogFile;
#endif

//-----------------------------------------------------------------------------
// Takes a bunch of messages, check them for validity,
// and pass them to MultiProcessData.

void MultiProcessBigData (uint8_t *buf, int32_t bufLen)
{
for (int32_t bufPos = 0, msgLen = 0; bufPos < bufLen; bufPos += msgLen) {
	uint8_t nType = buf [bufPos];
	if (nType > MULTI_MAX_TYPE)
		return;
	msgLen = MultiMsgLen (nType);
	if (bufPos + msgLen > bufLen) 
		return;
	if (!MultiProcessData (buf + bufPos, msgLen))
		return;
	}
}

//-----------------------------------------------------------------------------

int MultiCheckBigData (uint8_t *buf, int32_t bufLen)
{
	static uint8_t nDbgType = MULTI_RESTORE_GAME;

for (int32_t bufPos = 0, msgLen = 0; bufPos < bufLen; bufPos += msgLen) {
	uint8_t nType = buf [bufPos];
#if DBG
	if (nType == nDbgType)
		BRP;
#endif
	if (nType > MULTI_MAX_TYPE)
		return 0;
	msgLen = MultiMsgLen (nType);
	if (bufPos + msgLen > bufLen) 
		return 0;
	}
return 1;
}

//-----------------------------------------------------------------------------

int MultiCheckPlayerData (uint8_t* pd)
{
if (pd [0] != PID_PLAYER_DATA)
	return -1;

int32_t dataSize;
uint8_t* data;

if (netGameInfo.GetShortPackets ()) {
	dataSize = reinterpret_cast<tFrameInfoShort*> (pd)->data.dataSize;
	data = reinterpret_cast<tFrameInfoShort*> (pd)->data.msgData;
	}
else {
	dataSize = reinterpret_cast<tFrameInfoLong*> (pd)->data.dataSize;
	data = reinterpret_cast<tFrameInfoLong*> (pd)->data.msgData;
	}
return dataSize ? MultiCheckBigData (data, dataSize) : 1;
}

//-----------------------------------------------------------------------------
//
// Part 2 : Functions that send communication messages to inform the other
//          players of something we did.
//

//-----------------------------------------------------------------------------

void MultiSendFire (void)
{
if (!gameData.multigame.weapon.bFired)
	return;
int32_t bufP = 0;
gameData.multigame.msg.buf [bufP++] = uint8_t (MULTI_FIRE);
gameData.multigame.msg.buf [bufP++] = uint8_t (N_LOCALPLAYER);
gameData.multigame.msg.buf [bufP++] = uint8_t (gameData.multigame.weapon.nGun);
gameData.multigame.msg.buf [bufP++] = uint8_t (gameData.multigame.weapon.nLevel);
gameData.multigame.msg.buf [bufP++] = uint8_t (gameData.multigame.weapon.nFlags);
gameData.multigame.msg.buf [bufP++] = uint8_t (gameData.multigame.weapon.bFired);
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, gameData.multigame.weapon.nTrack);
bufP += 2;
MultiSendData (gameData.multigame.msg.buf, bufP, gameData.multigame.weapon.bFired);
gameData.multigame.weapon.nFired [0] = 0;
}

//-----------------------------------------------------------------------------

void MultiSendDestroyReactor (int32_t nObject, int32_t nPlayer)
{
if (nPlayer == N_LOCALPLAYER)
	HUDInitMessage (TXT_YOU_DEST_CONTROL);
else if ((nPlayer > 0) && (nPlayer < N_PLAYERS))
	HUDInitMessage ("%s %s", PLAYER (nPlayer).callsign, TXT_HAS_DEST_CONTROL);
else
	HUDInitMessage (TXT_CONTROL_DESTROYED);

int32_t bufP = 0;
gameData.multigame.msg.buf [bufP++] = MULTI_DESTROY_REACTOR;
ADD_MSG_ID
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, nObject);
bufP += 2;
gameData.multigame.msg.buf [bufP++] = nPlayer;
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 2);
}

//-----------------------------------------------------------------------------

void MultiSendCountdown (void)
{
gameData.multigame.msg.buf [0] = MULTI_COUNTDOWN;
PUT_INTEL_INT (gameData.multigame.msg.buf+1, gameData.reactor.countdown.nTimer);
MultiSendData (gameData.multigame.msg.buf, 5, 2);
}

//-----------------------------------------------------------------------------

void MultiSendDropMarker (int32_t nPlayer, CFixVector position, char nMessage, char text [])
{
if (nPlayer < N_PLAYERS) {
	gameData.multigame.msg.buf [0] = MULTI_MARKER;
	gameData.multigame.msg.buf [1] = (char) nPlayer;
	gameData.multigame.msg.buf [2] = nMessage;
	PUT_INTEL_INT (gameData.multigame.msg.buf + 3, position.v.coord.x);
	PUT_INTEL_INT (gameData.multigame.msg.buf + 7, position.v.coord.y);
	PUT_INTEL_INT (gameData.multigame.msg.buf + 11, position.v.coord.z);
	memcpy (gameData.multigame.msg.buf + 15, text, 40);
	MultiSendData (gameData.multigame.msg.buf, 55, 1);
	}
}

//-----------------------------------------------------------------------------

void MultiSendEndLevelStart (int32_t secret)
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_ENDLEVEL_START;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = N_LOCALPLAYER;
gameData.multigame.msg.buf [bufP++] = (uint8_t) secret;
if ((secret) && !gameData.multigame.bGotoSecret)
	gameData.multigame.bGotoSecret = 1;
else if (!gameData.multigame.bGotoSecret)
	gameData.multigame.bGotoSecret = 2;
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
if (IsNetworkGame) {
	CONNECT (N_LOCALPLAYER, CONNECT_ESCAPE_TUNNEL);
	NetworkSendEndLevelPacket ();
	}
}

//-----------------------------------------------------------------------------

void MultiSendPlayerExplode (uint8_t nType)
{
	int32_t bufP = 0;
	int32_t i;

MultiSendPosition (LOCALPLAYER.nObject);
NetworkResetObjSync (-1);
gameData.multigame.msg.buf [bufP++] = nType;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = N_LOCALPLAYER;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, LOCALPLAYER.primaryWeaponFlags);
bufP += 2;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, LOCALPLAYER.secondaryWeaponFlags);
bufP += 2;
gameData.multigame.msg.buf [bufP++] = uint8_t (LOCALPLAYER.laserLevel);
gameData.multigame.msg.buf [bufP++] = uint8_t (gameData.multiplayer.SecondaryAmmo (N_LOCALPLAYER, HOMING_INDEX));
gameData.multigame.msg.buf [bufP++] = uint8_t (gameData.multiplayer.SecondaryAmmo (N_LOCALPLAYER, CONCUSSION_INDEX, 0));
gameData.multigame.msg.buf [bufP++] = uint8_t (gameData.multiplayer.SecondaryAmmo (N_LOCALPLAYER, SMART_INDEX));
gameData.multigame.msg.buf [bufP++] = uint8_t (gameData.multiplayer.SecondaryAmmo (N_LOCALPLAYER, MEGA_INDEX));
gameData.multigame.msg.buf [bufP++] = uint8_t (gameData.multiplayer.SecondaryAmmo (N_LOCALPLAYER, PROXMINE_INDEX));
gameData.multigame.msg.buf [bufP++] = uint8_t (gameData.multiplayer.SecondaryAmmo (N_LOCALPLAYER, FLASHMSL_INDEX));
gameData.multigame.msg.buf [bufP++] = uint8_t (gameData.multiplayer.SecondaryAmmo (N_LOCALPLAYER, GUIDED_INDEX));
gameData.multigame.msg.buf [bufP++] = uint8_t (gameData.multiplayer.SecondaryAmmo (N_LOCALPLAYER, SMARTMINE_INDEX));
gameData.multigame.msg.buf [bufP++] = uint8_t (gameData.multiplayer.SecondaryAmmo (N_LOCALPLAYER, MERCURY_INDEX));
gameData.multigame.msg.buf [bufP++] = uint8_t (gameData.multiplayer.SecondaryAmmo (N_LOCALPLAYER, EARTHSHAKER_INDEX));
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, LOCALPLAYER.primaryAmmo [VULCAN_INDEX] + 
					  gameData.multiplayer.weaponStates [N_LOCALPLAYER].nAmmoUsed);
bufP += 2;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, LOCALPLAYER.primaryAmmo [GAUSS_INDEX]);
bufP += 2;
PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, LOCALPLAYER.flags);
bufP += 4;
#if 1
if (gameStates.multi.nGameType == UDP_GAME) {
	gameData.multigame.msg.buf [bufP++] = uint8_t (LOCALPLAYER.m_laserLevels [0]);
	gameData.multigame.msg.buf [bufP++] = uint8_t (LOCALPLAYER.m_laserLevels [1]);
	PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, gameStates.app.nRandSeed);
	bufP += 4;
	PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, LOCALOBJECT->Velocity ().v.coord.x);
	bufP += 4;
	PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, LOCALOBJECT->Velocity ().v.coord.y);
	bufP += 4;
	PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, LOCALOBJECT->Velocity ().v.coord.z);
	bufP += 4;
	PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, LOCALOBJECT->Position ().v.coord.x);
	bufP += 4;
	PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, LOCALOBJECT->Position ().v.coord.y);
	bufP += 4;
	PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, LOCALOBJECT->Position ().v.coord.z);
	bufP += 4;
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, LOCALOBJECT->Segment ());
	bufP += 2;
	}
#endif
gameData.multigame.msg.buf [bufP++] = gameData.multigame.create.nCount;
memset (gameData.multigame.msg.buf + bufP, -1, MAX_NET_CREATE_OBJECTS * sizeof (int16_t));
for (i = 0; i < gameData.multigame.create.nCount; i++) {
	if (gameData.multigame.create.nObjNums [i] <= 0)
		PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, -1);
	else {
		PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, gameData.multigame.create.nObjNums [i]);
		// We created these objs so our local number = the network number
		SetLocalObjNumMapping (int16_t (gameData.multigame.create.nObjNums [i]));
		}
	bufP += 2;
	}
gameData.multigame.create.nCount = 0;
#if DBG
if (bufP > MultiMsgLen (nType))
	Warning ("MultiSendPlayerExplode:\nMax. message length exceeded!"); // See Rob
#endif
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, MultiMsgLen (nType), 2);
if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED)
	MultiSendDeCloak ();
if (gameData.app.GameMode (GM_MULTI_ROBOTS))
	MultiStripRobots (N_LOCALPLAYER);
gameData.multiplayer.weaponStates [N_LOCALPLAYER].nAmmoUsed = 0;
}

//-----------------------------------------------------------------------------

//extern uint8_t secondaryWeaponToPowerup [2][];
//extern uint8_t primaryWeaponToPowerup [];

static int32_t nDeviceFlags [] = {PLAYER_FLAGS_FULLMAP, PLAYER_FLAGS_AMMO_RACK, PLAYER_FLAGS_CONVERTER, PLAYER_FLAGS_QUAD_LASERS, PLAYER_FLAGS_AFTERBURNER};
static int32_t nDevicePowerups [] = {POW_FULL_MAP, POW_AMMORACK, POW_CONVERTER, POW_QUADLASER, POW_AFTERBURNER};

// This function is called if a player ship is about to explode. It will make sure the ship doesn't drop more powerups of each kind than 
// the entire mine initially contained to prevent rampant powerups in netgames

void MultiCapObjects (void)
{
	int32_t	h, i, nType, nFlagType;

if (!IsNetworkGame)
	return;
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++) {
	nType = (i == SUPER_LASER_INDEX) ? POW_SUPERLASER : int32_t (primaryWeaponToPowerup [i]);
	if (!LOCALPLAYER.primaryWeaponFlags & (1 << i))
		continue;
	if (0 <= (h = MissingPowerups (nType)))
		continue;
	if (i == LASER_INDEX) {
		h = MAX_LASER_LEVEL - h;
		if (h < 0) 
			h = 0;
		if (LOCALPLAYER.LaserLevel (0) > h)
			LOCALPLAYER.SetStandardLaser (h);
		}
	else if (i == SUPER_LASER_INDEX) {
		h = MAX_SUPERLASER_LEVEL - MAX_LASER_LEVEL - h;
		if (LOCALPLAYER.LaserLevel (1) > h)
			LOCALPLAYER.SetSuperLaser (h);
		}
	else {
		if ((i == FUSION_INDEX) && (h < -1))
			gameData.multiplayer.weaponStates [N_LOCALPLAYER].bTripleFusion = 0;
		if (h < 0)
			LOCALPLAYER.primaryWeaponFlags &= (~(1 << i));
		}
	}
// Don't do the adjustment stuff for Hoard mode
if (!(gameData.app.GameMode (GM_HOARD | GM_ENTROPY)))
	LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX] /= 4;
if (IsEntropyGame)
	LOCALPLAYER.secondaryAmmo [SMARTMINE_INDEX] = 0;
else
	LOCALPLAYER.secondaryAmmo [SMARTMINE_INDEX] /= 4;

for (i = 0; i < MAX_SECONDARY_WEAPONS; i++) {
	if (IsHoardGame) {
		if (i == PROXMINE_INDEX)
			continue;
		}
	else if (IsEntropyGame) {
		if ((i == PROXMINE_INDEX) || (i == SMARTMINE_INDEX))
			continue;
		}
	nType = int32_t (secondaryWeaponToPowerup [0][i]);
	if (0 <= (h = MissingPowerups (nType)))
		continue;
	h += LOCALPLAYER.secondaryAmmo [i];
	if (LOCALPLAYER.secondaryAmmo [i] < h)
		LOCALPLAYER.secondaryAmmo [i] = h;
	}

if (!gameData.app.GameMode (GM_HOARD | GM_ENTROPY))
	LOCALPLAYER.secondaryAmmo [2] *= 4;
LOCALPLAYER.secondaryAmmo [7] *= 4;

for (i = 0; i < int32_t (sizeofa (nDeviceFlags)); i++) {
	if (LOCALPLAYER.flags & nDeviceFlags [i]) {
		nType = nDevicePowerups [i];
		if (0 > MissingPowerups (nType))
			LOCALPLAYER.flags &= (~PLAYER_FLAGS_CLOAKED);
		}
	}

if (PlayerHasHeadlight (-1) && (0 > MissingPowerups (POW_HEADLIGHT)))
	LOCALPLAYER.flags &= (~PLAYER_FLAGS_HEADLIGHT);

if (gameData.app.GameMode (GM_CAPTURE)) {
	if (LOCALPLAYER.flags & PLAYER_FLAGS_FLAG) {
		if (GetTeam (N_LOCALPLAYER) == TEAM_RED)
			nFlagType = POW_BLUEFLAG;
		else
			nFlagType = POW_REDFLAG;
		if (MissingPowerups (nFlagType) < 0)
			LOCALPLAYER.flags &= (~PLAYER_FLAGS_FLAG);
		}
	}
}

//-----------------------------------------------------------------------------
// adds players inventory to multi cap

void MultiAdjustCapForPlayer (int32_t nPlayer)
{
	int32_t	i, nType;

if (!IsNetworkGame)
	return;
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++) {
	nType = int32_t (primaryWeaponToPowerup [i]);
	if (IsBuiltinWeapon (i))	// weapon is standard loadout
		continue;
	if (i == 0) {// laser
		if (IsBuiltinWeapon (SUPER_LASER_INDEX))
			continue;
		if (!IsBuiltinWeapon (LASER_INDEX))	// lasers or superlasers are standard loadout
			AddAllowedPowerup (POW_LASER, PLAYER (nPlayer).LaserLevel (0));
		if (PLAYER (nPlayer).laserLevel > MAX_LASER_LEVEL)
			AddAllowedPowerup (POW_SUPERLASER, PLAYER (nPlayer).LaserLevel (1));
		}
	else if (i != 5) { // super laser
		if (IsBuiltinWeapon (i))
			continue;
		if (PLAYER (nPlayer).primaryWeaponFlags & (1 << i)) {
		    AddAllowedPowerup (nType);
			if ((nType == POW_FUSION) && (gameData.multiplayer.weaponStates [nPlayer].bTripleFusion > 0))
				AddAllowedPowerup (primaryWeaponToPowerup [FUSION_INDEX]);
			}
		}
	}
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++) {
	nType = int32_t (secondaryWeaponToPowerup [0][i]);
	AddAllowedPowerup (nType, i ? gameData.multiplayer.SecondaryAmmo (nPlayer, i) : gameData.multiplayer.SecondaryAmmo (nPlayer, i, 0));
	}

for (i = 0; i < int32_t (sizeofa (nDeviceFlags)); i++)
	if (gameData.multiplayer.Flag (nPlayer, nDeviceFlags [i]) && !(extraGameInfo [IsMultiGame].loadout.nDevice & nDeviceFlags [i]))
		AddAllowedPowerup (nDevicePowerups [i]);
if (PlayerHasHeadlight (nPlayer) && !(extraGameInfo [IsMultiGame].loadout.nDevice & PLAYER_FLAGS_HEADLIGHT))
	AddAllowedPowerup (POW_HEADLIGHT);
}

//-----------------------------------------------------------------------------

void MultiAdjustPowerupCap (void)
{
//if (IAmGameHost ()) 
	{
	for (int32_t i = 0; i < N_PLAYERS; i++) {
		if ((i != N_LOCALPLAYER) && PLAYER (i).IsConnected () && gameData.multiplayer.bAdjustPowerupCap [i]) {
			MultiAdjustCapForPlayer (i);
			gameData.multiplayer.bAdjustPowerupCap [i] = false;
			}
		}
	}
}

//-----------------------------------------------------------------------------

void MultiSendReappear (void)
{
gameData.multigame.msg.buf [0] = MULTI_REAPPEAR;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + 1, LOCALPLAYER.nObject);
MultiSendData (gameData.multigame.msg.buf, 3, 2);
gameData.multigame.score.pFlags [N_LOCALPLAYER] = 0;
}

//-----------------------------------------------------------------------------
// this message was initially only intended for modem games and is now used to better align shots
// in UDP/IP (-> D2X-XL specific) games.

void MultiSendPosition (int32_t nObject)
{
if (IsNetworkGame) 
	return;

CObject* objP = OBJECT (nObject);
if (!objP || (objP->Type () != OBJ_PLAYER))
	return;

int32_t bufP = 0;
gameData.multigame.msg.buf [bufP++] = MULTI_POSITION;
if (gameStates.multi.nGameType == UDP_GAME) {
	gameData.multigame.msg.buf [bufP++] = objP->Id ();
	memcpy (gameData.multigame.msg.buf + bufP, &objP->info.position, sizeof (tObjTransformation));
#if (defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
	tObjTransformation* p = reinterpret_cast<tObjTransformation*> (gameData.multigame.msg.buf + bufP);
	INTEL_VECTOR (t.vPos);
	INTEL_MATRIX (t.mOrientation);
#endif
	bufP += sizeof (tObjTransformation);
	memcpy (gameData.multigame.msg.buf + bufP, &objP->Velocity (), sizeof (CFixVector));
#if (defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
	INTEL_VECTOR (*reinterpret_cast<CFixVector*>(gameData.multigame.msg.buf + bufP));
#endif
	bufP += sizeof (CFixVector);
	}
else {
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
	CreateShortPos (reinterpret_cast<tShortPos*> (gameData.multigame.msg.buf + bufP), objP, 0);
	bufP += sizeof (tShortPos);
#else
	tShortPos sp;
	CreateShortPos (&sp, objP, 1);
	memcpy (gameData.multigame.msg.buf + bufP, reinterpret_cast<uint8_t*> (sp.orient), 9);
	bufP += 9;
	memcpy (gameData.multigame.msg.buf + bufP, reinterpret_cast<uint8_t*> (&sp.coord), 14);
	bufP += 14;
#endif
	}
MultiSendData (gameData.multigame.msg.buf, bufP, 0);
}

//-----------------------------------------------------------------------------
// I died, tell the world.

void MultiSendKill (int32_t nObject)
{
	int32_t	nKillerObj;
	int32_t	bufP = 0;

nKillerObj = LOCALPLAYER.nKillerObj;
MultiComputeKill (nKillerObj, nObject);
gameData.multigame.msg.buf [bufP++] = MULTI_KILL;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = N_LOCALPLAYER;
if (nKillerObj > -1) {
	// do it with variable player since INTEL_SHORT won't work on return val from function.
	int16_t nRemoteObj = (int16_t) GetRemoteObjNum (nKillerObj, reinterpret_cast<int8_t&> (gameData.multigame.msg.buf [bufP + 2]));
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, nRemoteObj);
	bufP += 3;
	}
else {
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, -1);
	bufP += 2;
	gameData.multigame.msg.buf [bufP++] = (char) -1;
	}
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
if (gameData.app.GameMode (GM_MULTI_ROBOTS))
	MultiStripRobots (N_LOCALPLAYER);
}

//-----------------------------------------------------------------------------

void MultiDoSyncKills (uint8_t* buf)
{
	int32_t	nPlayer = int32_t (buf [1]);
	uint8_t*	bufP = buf + 2;

PLAYER (nPlayer).score = GET_INTEL_INT (bufP);
bufP += 4;
PLAYER (nPlayer).nScoreGoalCount = GET_INTEL_SHORT (bufP);
bufP += 2;
PLAYER (nPlayer).netKilledTotal = GET_INTEL_SHORT (bufP);
bufP += 2;
PLAYER (nPlayer).netKillsTotal = GET_INTEL_SHORT (bufP);
bufP += 2;
PLAYER (nPlayer).numKillsLevel = GET_INTEL_SHORT (bufP);
bufP += 2;
PLAYER (nPlayer).numKillsTotal = GET_INTEL_SHORT (bufP);
}

//-----------------------------------------------------------------------------

void MultiSendSyncKills (void)
{
	uint8_t* bufP = gameData.multigame.msg.buf + 2;

gameData.multigame.msg.buf [0] = MULTI_SYNC_KILLS;
gameData.multigame.msg.buf [1] = N_LOCALPLAYER;
PUT_INTEL_INT (bufP, LOCALPLAYER.score);
bufP += 4;
PUT_INTEL_SHORT (bufP, LOCALPLAYER.nScoreGoalCount);
bufP += 2;
PUT_INTEL_SHORT (bufP, LOCALPLAYER.netKilledTotal);
bufP += 2;
PUT_INTEL_SHORT (bufP, LOCALPLAYER.netKillsTotal);
bufP += 2;
PUT_INTEL_SHORT (bufP, LOCALPLAYER.numKillsLevel);
bufP += 2;
PUT_INTEL_SHORT (bufP, LOCALPLAYER.numKillsTotal);
MultiSendData (gameData.multigame.msg.buf, 16, 0);
}

//-----------------------------------------------------------------------------

void MultiSyncKills (void)
{
	static	time_t	t0;

if (IsMultiGame && (gameStates.multi.nGameType == UDP_GAME) && (gameStates.app.nSDLTicks [0] - t0 > 1000)) {
	t0 = gameStates.app.nSDLTicks [0];
	MultiSendSyncKills ();
	}
}

//-----------------------------------------------------------------------------
// Tell the other guy to remove an object from his list

void MultiSendRemoveObject (int32_t nObject)
{
if ((nObject < 0) || (nObject > gameData.objData.nLastObject [0]))
	return;

	int8_t	nObjOwner;
	int16_t	nRemoteObj;
	int32_t	bufP = 0;

gameData.multigame.msg.buf [bufP++] = uint8_t (MULTI_REMOVE_OBJECT);
ADD_MSG_ID
nRemoteObj = GetRemoteObjNum (int16_t (nObject), nObjOwner);
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, nRemoteObj); // Map to network objnums
bufP += 2;
gameData.multigame.msg.buf [bufP++] = nObjOwner;
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 0);
NetworkResetObjSync (nObject);
}

//-----------------------------------------------------------------------------
// I am quitting the game, tell the other guy the bad news.

void MultiSendQuit (int32_t nReason)
{
gameData.multigame.msg.buf [0] = (uint8_t) nReason;
gameData.multigame.msg.buf [1] = N_LOCALPLAYER;
MultiSendData (gameData.multigame.msg.buf, 2, 1);
}

//-----------------------------------------------------------------------------

void MultiSendPowerState (uint8_t id, bool bStripRobots)
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = id;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = (uint8_t) N_LOCALPLAYER;
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
if (bStripRobots && gameData.app.GameMode (GM_MULTI_ROBOTS))
	MultiStripRobots (N_LOCALPLAYER);
}

//-----------------------------------------------------------------------------
// Broadcast a change in our pflags (made to support cloaking)

void MultiSendInvul (void)
{
MultiSendPowerState (MULTI_INVUL, true);
}

//-----------------------------------------------------------------------------
// Broadcast a change in our pflags (made to support cloaking)

void MultiSendDeInvul (void)
{
MultiSendPowerState (MULTI_DEINVUL, false);
}

//-----------------------------------------------------------------------------
// Broadcast a change in our pflags (made to support cloaking)

void MultiSendCloak (void)
{
MultiSendPowerState (MULTI_CLOAK, true);
}

//-----------------------------------------------------------------------------
// Broadcast a change in our pflags (made to support cloaking)

void MultiSendDeCloak (void)
{
MultiSendPowerState (MULTI_DECLOAK, false);
}

//-----------------------------------------------------------------------------
// When we open a door make sure everyone else opens that door

void MultiSendDoorOpen (int32_t nSegment, int32_t nSide, uint16_t flags)
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_DOOR_OPEN;
ADD_MSG_ID
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, nSegment);
bufP += 2;
gameData.multigame.msg.buf [bufP++] = (int8_t) nSide;
gameData.multigame.msg.buf [bufP++] = uint8_t (flags);
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 2);
}

//-----------------------------------------------------------------------------
// For sending doors only to a specific person (usually when they're joining)

extern void NetworkSendMineSyncPacket (char *, int16_t, int32_t);

void MultiSendDoorOpenSpecific (int32_t nPlayer, int32_t nSegment, int32_t nSide, uint16_t flags)
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_DOOR_OPEN;
ADD_MSG_ID
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, nSegment);
bufP += 2;
gameData.multigame.msg.buf [bufP++] = (int8_t) nSide;
gameData.multigame.msg.buf [bufP++] = (uint8_t) flags;
NetworkSendMineSyncPacket (gameData.multigame.msg.buf, 5, nPlayer);
}

//
// Part 3 : Functions that change or prepare the game for multiplayer use.
//          Not including functions needed to synchronize or start the
//          particular nType of multiplayer game.  Includes preparing the
//                      mines, player structures, etc.

//-----------------------------------------------------------------------------

void MultiSendCreateExplosion (int32_t nPlayer)
{
// Send all data needed to create a remote explosion
gameData.multigame.msg.buf [0] = MULTI_CREATE_EXPLOSION;
gameData.multigame.msg.buf [1] = (int8_t)nPlayer;
MultiSendData (gameData.multigame.msg.buf, 2, 0);
}

//-----------------------------------------------------------------------------

void MultiSendCtrlcenFire (CFixVector *vGoal, int32_t nBestGun, int32_t nObject)
{
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	CFixVector vSwapped;
#endif
	int32_t count = 0;

gameData.multigame.msg.buf [count++] = MULTI_CONTROLCEN_FIRE;
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
memcpy (gameData.multigame.msg.buf + count, vGoal, 12);
count += 12;
#else
vSwapped.dir.coord.x = (fix) INTEL_INT ((int32_t) (*vGoal).dir.coord.x);
vSwapped.dir.coord.y = (fix) INTEL_INT ((int32_t) (*vGoal).dir.coord.y);
vSwapped.dir.coord.z = (fix) INTEL_INT ((int32_t) (*vGoal).dir.coord.z);
memcpy (gameData.multigame.msg.buf + count, &vSwapped, 12);
count += 12;
#endif
gameData.multigame.msg.buf [count++] = (char)nBestGun;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + count, nObject);
count += 2;
MultiSendData (gameData.multigame.msg.buf, count, 0);
}

//-----------------------------------------------------------------------------

void MultiSendCreateWeapon (int32_t nObject)
{
	// Create a powerup on a remote machine, used for remote
	// placement of used powerups like missiles and cloaking
	// powerups.
CObject* objP = OBJECT (nObject);
if (!objP)
	return;

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	CFixVector vSwapped;
#endif
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_CREATE_WEAPON;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = N_LOCALPLAYER;
gameData.multigame.msg.buf [bufP++] = objP->info.nId;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, objP->info.nSegment);
bufP += 2;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, nObject);
bufP += 2;
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
memcpy (gameData.multigame.msg.buf + bufP, &objP->info.position.vPos, sizeof (CFixVector));
bufP += sizeof (CFixVector);
memcpy (gameData.multigame.msg.buf + bufP, &objP->mType.physInfo.velocity, sizeof (CFixVector));
bufP += sizeof (CFixVector);
#else
vSwapped.dir.coord.x = (fix)INTEL_INT (int32_t (objP->info.position.vPos.dir.coord.x));
vSwapped.dir.coord.y = (fix)INTEL_INT (int32_t (objP->info.position.vPos.dir.coord.y));
vSwapped.dir.coord.z = (fix)INTEL_INT (int32_t (objP->info.position.vPos.dir.coord.z));
memcpy (gameData.multigame.msg.buf + bufP, &vSwapped, 12);
bufP += 12;
vSwapped.dir.coord.x = (fix)INTEL_INT (int32_t (objP->mType.physInfo.velocity.dir.coord.x));
vSwapped.dir.coord.y = (fix)INTEL_INT (int32_t (objP->mType.physInfo.velocity.dir.coord.y));
vSwapped.dir.coord.z = (fix)INTEL_INT (int32_t (objP->mType.physInfo.velocity.dir.coord.z));
memcpy (gameData.multigame.msg.buf + bufP, &vSwapped, 12);
bufP += 12;
#endif
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
NetworkResetObjSync (nObject);
SetLocalObjNumMapping (nObject);
}

//-----------------------------------------------------------------------------

void MultiSendCreatePowerup (int32_t powerupType, int32_t nSegment, int32_t nObject, const CFixVector *vPos)
{
	// Create a powerup on a remote machine, used for remote
	// placement of used powerups like missiles and cloaking
	// powerups.

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	CFixVector vSwapped;
#endif
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_CREATE_POWERUP;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = N_LOCALPLAYER;
gameData.multigame.msg.buf [bufP++] = powerupType;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, nSegment);
bufP += 2;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, nObject);
bufP += 2;
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
memcpy (gameData.multigame.msg.buf + bufP, vPos, sizeof (CFixVector));
bufP += sizeof (CFixVector);
#else
vSwapped.dir.coord.x = (fix)INTEL_INT ((int32_t) (*vPos).dir.coord.x);
vSwapped.dir.coord.y = (fix)INTEL_INT ((int32_t) (*vPos).dir.coord.y);
vSwapped.dir.coord.z = (fix)INTEL_INT ((int32_t) (*vPos).dir.coord.z);
memcpy (gameData.multigame.msg.buf + bufP, &vSwapped, 12);
bufP += 12;
#endif
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
NetworkResetObjSync (nObject);
SetLocalObjNumMapping (nObject);
}

//-----------------------------------------------------------------------------

void MultiSendDropPowerup (int32_t powerupType, int32_t nSegment, int32_t nObject, const CFixVector *vPos, const CFixVector *vVel)
{
	// Create a powerup on a remote machine, used for remote
	// placement of used powerups like missiles and cloaking
	// powerups.

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	CFixVector vSwapped;
#endif
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_DROP_POWERUP;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = N_LOCALPLAYER;
gameData.multigame.msg.buf [bufP++] = powerupType;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, nSegment);
bufP += 2;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, nObject);
bufP += 2;
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
memcpy (gameData.multigame.msg.buf + bufP, vPos, sizeof (CFixVector));
bufP += sizeof (CFixVector);
memcpy (gameData.multigame.msg.buf + bufP, vVel, sizeof (CFixVector));
bufP += sizeof (CFixVector);
#else
vSwapped.dir.coord.x = (fix)INTEL_INT ((int32_t) (*vPos).dir.coord.x);
vSwapped.dir.coord.y = (fix)INTEL_INT ((int32_t) (*vPos).dir.coord.y);
vSwapped.dir.coord.z = (fix)INTEL_INT ((int32_t) (*vPos).dir.coord.z);
memcpy (gameData.multigame.msg.buf + bufP, &vSwapped, 12);
bufP += 12;
vSwapped.dir.coord.x = (fix)INTEL_INT ((int32_t) (*vVel).dir.coord.x);
vSwapped.dir.coord.y = (fix)INTEL_INT ((int32_t) (*vVel).dir.coord.y);
vSwapped.dir.coord.z = (fix)INTEL_INT ((int32_t) (*vVel).dir.coord.z);
memcpy (gameData.multigame.msg.buf + bufP, &vSwapped, 12);
bufP += 12;
#endif
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
NetworkResetObjSync (nObject);
SetLocalObjNumMapping (nObject);
}

//-----------------------------------------------------------------------------

void MultiSendPlaySound (int32_t nSound, fix volume)
{
gameData.multigame.msg.buf [0] = MULTI_PLAY_SOUND;
gameData.multigame.msg.buf [1] = N_LOCALPLAYER;
gameData.multigame.msg.buf [2] = char (nSound);
gameData.multigame.msg.buf [3] = char (volume >> 12);
MultiSendData (gameData.multigame.msg.buf, 4, 0);
}

//-----------------------------------------------------------------------------

void MultiSendAudioTaunt (int32_t taunt_num)
{
	return; // Taken out, awaiting sounds..

#if 0
	int32_t audio_taunts [4] = {
		SOUND_CONTROL_CENTER_WARNING_SIREN,
		SOUND_HOSTAGE_RESCUED,
		SOUND_REFUEL_STATION_GIVING_FUEL,
		SOUND_BAD_SELECTION
		};

	audio.PlaySound (audio_taunts [taunt_num]);
	MultiSendPlaySound (audio_taunts [taunt_num], I2X (1));
#endif
}

//-----------------------------------------------------------------------------

void MultiSendScore (void)
{
	// Send my current score to all other players so it will remain
	// synced.
	int32_t bufP = 0;

if (IsCoopGame) {
	MultiSortKillList ();
	gameData.multigame.msg.buf [bufP++] = MULTI_SCORE;
	gameData.multigame.msg.buf [bufP++] = N_LOCALPLAYER;
	PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, LOCALPLAYER.score);
	bufP += 4;
	MultiSendData (gameData.multigame.msg.buf, bufP, 0);
	}
}

//-----------------------------------------------------------------------------

void MultiSendSaveGame (uint8_t slot, uint32_t id, char * desc)
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_SAVE_GAME;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = slot;
PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, id);
bufP += 4;             // Save id
memcpy (&gameData.multigame.msg.buf [bufP], desc, 20);
bufP += 20;
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 2);
}

//-----------------------------------------------------------------------------

void MultiSendRestoreGame (uint8_t slot, uint32_t id)
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_RESTORE_GAME;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = slot;
PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, id);
bufP += 4;             // Save id
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 2);
}

//-----------------------------------------------------------------------------

void MultiSendNetPlayerStatsRequest (uint8_t nPlayer)
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_REQ_PLAYER;
gameData.multigame.msg.buf [bufP++] = nPlayer;
MultiSendData (gameData.multigame.msg.buf, bufP, 0);
}

//-----------------------------------------------------------------------------

void MultiSendTrigger (int32_t nTrigger, int32_t nObject)
{
	// Send an event to trigger something in the mine

	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_TRIGGER;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = N_LOCALPLAYER;
gameData.multigame.msg.buf [bufP++] = (uint8_t) nTrigger;
if (gameStates.multi.nGameType == UDP_GAME) {
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, nObject);
	bufP += 2;
	}
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
}

//-----------------------------------------------------------------------------

void MultiSendObjTrigger (int32_t nTrigger)
{
	// Send an even to CTrigger something in the mine

int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_OBJ_TRIGGER;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = N_LOCALPLAYER;
gameData.multigame.msg.buf [bufP++] = (uint8_t) nTrigger;
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
}

//-----------------------------------------------------------------------------

void MultiSendHostageDoorStatus (int32_t nWall)
{
	// Tell the other player what the hit point status of a hostage door
	// should be
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_HOSTAGE_DOOR;
ADD_MSG_ID
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, nWall);
bufP += 2;
PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, WALL (nWall)->hps);
bufP += 4;
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 0);
}

//-----------------------------------------------------------------------------

void MultiPrepLevel (void)
{
	// Do any special stuff to the level required for serial games
	// before we begin playing in it.

	// N_LOCALPLAYER MUST be set before calling this procedure.

	// This function must be called before checksuming the Object array,
	// since the resulting checksum with depend on the value of N_LOCALPLAYER
	// at the time this is called.

	int32_t	i, nObject;
	int32_t	cloakCount, invulCount;
	CObject*	objP;

gameData.score.nHighscore = 0;
gameData.score.nChampion = -1;
gameStates.render.bDropAfterburnerBlob = 0;
networkData.nConsistencyErrorCount = 0;
memset (gameData.multigame.score.pFlags, 0, MAX_NUM_NET_PLAYERS * sizeof (gameData.multigame.score.pFlags [0]));
for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++) {
	objP = OBJECT (PLAYER (i).nObject);
	if (objP) {
		if (i != N_LOCALPLAYER)
			objP->info.controlType = CT_REMOTE;
		objP->info.movementType = MT_PHYSICS;
		MultiResetPlayerObject (objP);
		networkData.nLastPacketTime [i] = 0;
		}
	}
for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++) {
	gameData.multigame.robots.controlled [i] = -1;
	gameData.multigame.robots.agitation [i] = 0;
	gameData.multigame.robots.fired [i] = 0;
	}
gameData.objData.viewerP = gameData.objData.consoleP = OBJECT (LOCALPLAYER.nObject);
if (!IsCoopGame)
	MultiDeleteExtraObjects (); // Removes monsters from level
if (gameData.app.GameMode (GM_MULTI_ROBOTS))
	MultiSetRobotAI (); // Set all Robot AI to types we can cope with
if (IsNetworkGame) {
	MultiAdjustCapForPlayer (N_LOCALPLAYER);
	MultiSendPowerupUpdate ();
	}
invulCount = 0;
cloakCount = 0;
SetupPowerupFilter ();
FORALL_STATIC_OBJS (objP) {
	if (objP->info.nType == OBJ_HOSTAGE) {
		if (!IsCoopGame) {
			nObject = CreatePowerup (POW_SHIELD_BOOST, -1, objP->info.nSegment, objP->info.position.vPos, 1);
			ReleaseObject (objP->Index ());
			if (nObject != -1) {
				CObject	*objP = OBJECT (nObject);
				if (objP) {
					objP->rType.animationInfo.nClipIndex = gameData.objData.pwrUp.info [POW_SHIELD_BOOST].nClipIndex;
					objP->rType.animationInfo.xFrameTime = gameData.effects.animations [0][objP->rType.animationInfo.nClipIndex].xFrameTime;
					objP->rType.animationInfo.nCurFrame = 0;
					objP->mType.physInfo.drag = 512;     //1024;
					objP->mType.physInfo.mass = I2X (1);
					objP->mType.physInfo.velocity.SetZero ();
					}
				}
			}
		}
	else if (objP->info.nType == OBJ_POWERUP) {
		switch (objP->info.nId) {
			case POW_EXTRA_LIFE:
				if (netGameInfo.m_info.DoInvulnerability)
					objP->Bash (POW_INVUL);
				else
					objP->BashToShield (true);
				break;

			case POW_INVUL:
				if ((invulCount < 3) && netGameInfo.m_info.DoInvulnerability)
					invulCount++;
				else
					objP->BashToShield (true);
				break;

			case POW_CLOAK:
				if ((cloakCount < 3) && netGameInfo.m_info.DoCloak)
					cloakCount++;
				else
					objP->BashToShield (true);
				break;
#if 1
			default:
				if (!powerupFilter [objP->info.nId])
					objP->BashToShield (true);
#else
			case POW_KEY_BLUE:
			case POW_KEY_RED:
			case POW_KEY_GOLD:
				if (!IsCoopGame)
				break;
			case POW_AFTERBURNER:
				objP->BashToShield (!netGameInfo.m_info.DoAfterburner);
				break;
			case POW_FUSION:
				objP->BashToShield (!netGameInfo.m_info.DoFusions);
				break;
			case POW_PHOENIX:
				objP->BashToShield (!netGameInfo.m_info.DoPhoenix);
				break;
			case POW_HELIX:
				objP->BashToShield (!netGameInfo.m_info.DoHelix);
				break;
			case POW_MEGAMSL:
				objP->BashToShield (!netGameInfo.m_info.DoMegas);
				break;
			case POW_SMARTMSL:
				objP->BashToShield (!netGameInfo.m_info.DoSmarts);
				break;
			case POW_GAUSS:
				objP->BashToShield (!netGameInfo.m_info.DoGauss);
				break;
			case POW_VULCAN:
				objP->BashToShield (!netGameInfo.m_info.DoVulcan);
				break;
			case POW_PLASMA:
				objP->BashToShield (!netGameInfo.m_info.DoPlasma);
				break;
			case POW_OMEGA:
				objP->BashToShield (!netGameInfo.m_info.DoOmega);
				break;
			case POW_SUPERLASER:
				objP->BashToShield (!netGameInfo.m_info.DoSuperLaser);
				break;
			case POW_PROXMINE:
				objP->BashToShield (!netGameInfo.m_info.DoProximity || (gameData.app.nGameMode &(GM_HOARD | GM_ENTROPY)));
				break;
			case POW_SMARTMINE:
				objP->BashToShield (!netGameInfo.m_info.DoSmartMine || IsEntropyGame);
				break;
			case POW_VULCAN_AMMO:
				objP->BashToShield (!(netGameInfo.m_info.DoVulcan || netGameInfo.m_info.DoGauss));
				break;
			case POW_SPREADFIRE:
				objP->BashToShield (!netGameInfo.m_info.DoSpread);
				break;
			case POW_FLASHMSL_1:
				objP->BashToShield (!netGameInfo.m_info.DoFlash);
				break;
			case POW_FLASHMSL_4:
				objP->BashToShield (!netGameInfo.m_info.DoFlash);
				break;
			case POW_GUIDEDMSL_1:
				objP->BashToShield (!netGameInfo.m_info.DoGuided);
				break;
			case POW_GUIDEDMSL_4:
				objP->BashToShield (!netGameInfo.m_info.DoGuided);
				break;
			case POW_EARTHSHAKER:
				objP->BashToShield (!netGameInfo.m_info.DoEarthShaker);
				break;
			case POW_MERCURYMSL_1:
				objP->BashToShield (!netGameInfo.m_info.DoMercury);
				break;
			case POW_MERCURYMSL_4:
				objP->BashToShield (!netGameInfo.m_info.DoMercury);
				break;
			case POW_CONVERTER:
				objP->BashToShield (!netGameInfo.m_info.DoConverter);
				break;
			case POW_AMMORACK:
				objP->BashToShield (!netGameInfo.m_info.DoAmmoRack);
				break;
			case POW_HEADLIGHT:
				 objP->BashToShield (!netGameInfo.m_info.DoHeadlight || (EGI_FLAG (bDarkness, 0, 0, 0) && !EGI_FLAG (headlight.bAvailable, 0, 1, 0)));
				break;
			case POW_LASER:
				objP->BashToShield (!netGameInfo.m_info.DoLaserUpgrade);
				break;
			case POW_HOMINGMSL_1:
				objP->BashToShield (!netGameInfo.m_info.DoHoming);
				break;
			case POW_HOMINGMSL_4:
				objP->BashToShield (!netGameInfo.m_info.DoHoming);
				break;
			case POW_QUADLASER:
				objP->BashToShield (!netGameInfo.m_info.DoQuadLasers);
				break;
			case POW_BLUEFLAG:
				objP->BashToShield (!(gameData.app.GameMode (GM_CAPTURE)));
				break;
			case POW_REDFLAG:
				objP->BashToShield (!(gameData.app.GameMode (GM_CAPTURE)));
#endif
			}
		}
	}
#if !DBG
if (gameData.app.nGameMode &(GM_HOARD | GM_ENTROPY | GM_MONSTERBALL))
#endif
	{
	FreeHoardData ();
	InitHoardData ();
	}
if (gameData.app.GameMode (GM_CAPTURE | GM_HOARD | GM_ENTROPY | GM_MONSTERBALL))
	MultiApplyGoalTextures ();
ResetMonsterball (IAmGameHost () != 0);	//will simply delete all Monsterballs for non-Monsterball games
MultiSortKillList ();
MultiShowPlayerList ();
gameData.objData.consoleP->info.controlType = CT_FLYING;
ResetPlayerObject ();
}

//-----------------------------------------------------------------------------

int32_t MultiFindGoalTexture (int16_t t)
{
	int32_t i;

if (t == TMI_PRODUCER)
	return 333;
for (i = 0; i < gameData.pig.tex.nTextures [gameStates.app.bD1Data]; i++)
	if (gameData.pig.tex.tMapInfoP [i].flags & t)
		return i;
Int3 (); // Hey, there is no goal texture for this PIG!!!!
// Edit bitmaps.tbl and designate two textures to be RED and BLUE
// goal textures
return -1;
}


//-----------------------------------------------------------------------------
// override a segments texture with the owning team's textures.
// if nOldTexture  >= 0, only override textures equal to nOldTexture

void CSegment::OverrideTextures (int16_t nTexture, int16_t nOldTexture, int16_t nTexture2, int32_t bFullBright, int32_t bForce)
{
nTexture = (nTexture < 0) ? -nTexture : MultiFindGoalTexture (nTexture);
nOldTexture = (nOldTexture < 0) ? -nOldTexture : MultiFindGoalTexture (nOldTexture);
if (nTexture >- 1) {
	CSide* sideP = m_sides;
	for (int32_t j = 0; j < SEGMENT_SIDE_COUNT; j++, sideP++) {
		if (!sideP->FaceCount ())
			continue;
		if (bForce || (m_sides [j].m_nBaseTex == nOldTexture)) {
			sideP->m_nBaseTex = nTexture;
			if ((extraGameInfo [1].entropy.nOverrideTextures == 1) &&
				 (sideP->m_nOvlTex > 0) &&(nTexture2 > 0))
				sideP->m_nOvlTex = nTexture2;
			if ((extraGameInfo [1].entropy.nOverrideTextures == 1) && bFullBright)
				for (int32_t v = 0; v < 4; v++)
					sideP->m_uvls [v].l = I2X (100);		//max out
			}
		}
	}
if (bFullBright)
	m_xAvgSegLight = I2X (100);	//make static light bright
}

//-----------------------------------------------------------------------------

#define	TMI_RED_TEAM	TMI_GOAL_RED	//-313
#define	TMI_BLUE_TEAM	TMI_GOAL_BLUE	//-333

int32_t nBlueGoalSegment, nRedGoalSegment;

void CSegment::ChangeTexture (int32_t oldOwner)
{
	int32_t		bFullBright = (IsHoardGame != 0) || (IsEntropyGame && extraGameInfo [1].entropy.bBrightenRooms);
	static	int16_t texOverrides [3] = {-313, TMI_BLUE_TEAM, TMI_RED_TEAM};

//if (oldOwner < 0)
//	oldOwner = nOwner;
if (IsEntropyGame &&(extraGameInfo [1].entropy.nOverrideTextures == 2))
	return;
switch (m_function) {
	case SEGMENT_FUNC_GOAL_BLUE:
		nBlueGoalSegment = Index ();
		OverrideTextures ((int16_t) (IsHoardGame ? TMI_GOAL_HOARD : TMI_GOAL_BLUE), -1, -1, bFullBright, 1);
		break;

	case SEGMENT_FUNC_GOAL_RED:
		nRedGoalSegment = Index ();
		// Make both textures the same if Hoard mode
		OverrideTextures ((int16_t) (IsHoardGame ? TMI_GOAL_HOARD : TMI_GOAL_RED), -1, -1, bFullBright, 1);
		break;

	case SEGMENT_FUNC_ROBOTMAKER:
		if (IsEntropyGame &&(m_owner >= 0))
			OverrideTextures (texOverrides [(int32_t) m_owner],
									 (int16_t) ((oldOwner < 0) ? -1 : texOverrides [oldOwner]), 316, bFullBright, oldOwner < 0);
		break;

	case SEGMENT_FUNC_REPAIRCENTER:
		if (IsEntropyGame &&(m_owner >= 0))
			OverrideTextures (texOverrides [(int32_t) m_owner],
									 (int16_t) ((oldOwner < 0) ? -1 : texOverrides [oldOwner]), 315, bFullBright, oldOwner < 0);
		break;

	case SEGMENT_FUNC_FUELCENTER:
		if (IsEntropyGame &&(m_owner >= 0))
			OverrideTextures (texOverrides [(int32_t) m_owner],
								   (int16_t) ((oldOwner < 0) ? -1 : texOverrides [oldOwner]), 314, bFullBright, oldOwner < 0);
		break;

	default:
		if (IsEntropyGame &&(m_owner >= 0))
			OverrideTextures (texOverrides [(int32_t) m_owner],
									 (int16_t) ((oldOwner < 0) ? -1 : texOverrides [oldOwner]), -1, bFullBright, oldOwner < 0);
	}
}

//-----------------------------------------------------------------------------

void MultiApplyGoalTextures ()
{
	int32_t		i;

if (!IsEntropyGame || (extraGameInfo [1].entropy.nOverrideTextures == 1))
	for (i = 0; i <= gameData.segData.nLastSegment; i++)
		SEGMENT (i)->ChangeTexture (-1);
}

//-----------------------------------------------------------------------------

void MultiSetRobotAI (void)
{
	// Go through the objects array looking for robots and setting
	// them to certain supported types of NET AI behavior.

	//      int32_t i;
	//
	//      for (i = 0; i <= gameData.objData.nLastObject [0]; i++)
	// {
	//              CObject* objP = OBJECT (i);
	//              if (objP->IsRobot ()) {
	//                      objP->aiInfo.REMOTE_OWNER = -1;
	//                      if (objP->aiInfo.behavior == AIB_STATION)
	//                              objP->aiInfo.behavior = AIB_NORMAL;
	//              }
	//      }
}

//-----------------------------------------------------------------------------

int32_t MultiDeleteExtraObjects (void)
{
	int32_t 	nType, nnp = 0;
	CObject *objP;

// Go through the CObject list and remove any objects not used in
// 'Anarchy!' games.
// This function also prints the total number of available multiplayer
// positions in this level, even though this should always be 8 or more!

FORALL_OBJS (objP) {
	nType = objP->info.nType;
	if ((nType == OBJ_PLAYER) || (nType == OBJ_GHOST) || (nType == OBJ_CAMBOT) || (nType == OBJ_EFFECT))
		nnp++;
	else if ((nType == OBJ_ROBOT) && gameData.app.GameMode (GM_MULTI_ROBOTS))
		;
	else if ((nType != OBJ_NONE) &&(nType != OBJ_PLAYER) &&(nType != OBJ_POWERUP) &&
				(nType != OBJ_MONSTERBALL) &&(nType != OBJ_EXPLOSION) &&(nType != OBJ_REACTOR) &&
				(nType != OBJ_HOSTAGE) &&((nType != OBJ_WEAPON) || (objP->info.nId != SMALLMINE_ID))) {
		// Before deleting CObject, if it's a robot, drop it's special powerup, if any
		if (nType == OBJ_ROBOT)
			if (objP->info.contains.nCount && (objP->info.contains.nType == OBJ_POWERUP))
				objP->CreateEgg ();
		objP->Die ();
		}
	}
return nnp;
}

//-----------------------------------------------------------------------------

void ChangePlayerNumTo (int32_t nLocalPlayer)
{
if ((N_LOCALPLAYER > -1) && (N_LOCALPLAYER != nLocalPlayer))
	memcpy (PLAYER (nLocalPlayer).callsign, LOCALPLAYER.callsign, CALLSIGN_LEN + 1);
N_LOCALPLAYER = nLocalPlayer;
}

//-----------------------------------------------------------------------------

int32_t MultiAllPlayersAlive (void)
{
	int32_t i;

for (i = 0; i < N_PLAYERS; i++)
	if (gameData.multigame.score.pFlags [i] && PLAYER (i).IsConnected ())
		return 0;
return 1;
}

//-----------------------------------------------------------------------------

void MultiInitiateSaveGame (int32_t bSecret)
{
	uint32_t	gameId;
	int32_t	i;
	char	slot;
	union {
		uint8_t		b [4];
		uint32_t		i;
	} uintCast;

if ((gameStates.app.bEndLevelSequence) || (gameData.reactor.bDestroyed))
	return;
if (!MultiAllPlayersAlive ()) {
	HUDInitMessage (TXT_SAVE_DEADPLRS);
	return;
	}
if (bSecret < 0) {
	slot = -missionManager.nCurrentLevel;
	strcpy (saveGameManager.Description (), "[LEVEL STATE]");
	}
else if (0 > (slot = saveGameManager.GetSaveFile (1) - 1))
	return;
// Make a unique game id (two separate operations because of data type mix)
gameId = TimerGetFixedSeconds ();
gameId ^= N_PLAYERS << 4;
for (i = 0; i < N_PLAYERS; i++) {
	memcpy (uintCast.b, &PLAYER (i).callsign [0], sizeof (uintCast.b));
	gameId ^= uintCast.i;
	}
if (gameId == 0)
	gameId = 1; // 0 is invalid
if (slot >= 0)
	MultiSendSaveGame (slot, gameId, saveGameManager.Description ());
MultiDoFrame ();
MultiSaveGame (slot, gameId, saveGameManager.Description ());
}

//-----------------------------------------------------------------------------

void MultiInitiateRestoreGame (int32_t bSecret)
{
	char slot;

#if !DBG
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed)
	return;
#endif
if (!MultiAllPlayersAlive ()) {
	HUDInitMessage (TXT_LOAD_DEADPLRS);
	return;
	}
if (bSecret < 0) {
	slot = -missionManager.NextLevel ();
	missionManager.LevelStateName (saveGameManager.Filename (), -slot);
	}
else if (0 > (slot = saveGameManager.GetLoadFile (1) - 1))
	return;
gameData.app.nStateGameId = saveGameManager.GetGameId (saveGameManager.Filename (), bSecret);
if (!gameData.app.nStateGameId)
	return;
//StartTime (0);
if (slot >= 0)
	MultiSendRestoreGame (slot, gameData.app.nStateGameId);
MultiDoFrame ();
MultiRestoreGame (slot, gameData.app.nStateGameId);
}

//-----------------------------------------------------------------------------

void MultiSaveGame (char slot, uint32_t id, char *description)
{
	char szFile [FILENAME_LEN];

if ((gameStates.app.bEndLevelSequence) || (gameData.reactor.bDestroyed))
	return;
if (slot < 0) 
	missionManager.LevelStateName (szFile);
else {
	HUDInitMessage (TXT_SAVEGAME_NO, slot, description);
	sprintf (szFile, "%s.mg%d", LOCALPLAYER.callsign, slot);
	}
StopTime ();
gameData.app.nStateGameId = id;
saveGameManager.SaveState ((slot < 0) ? -1 : 0, szFile, description);
}

//-----------------------------------------------------------------------------

void MultiRestoreGame (char slot, uint32_t id)
{
	char			szFile [FILENAME_LEN];
	//CPlayerInfo	savedPlayer = LOCALPLAYER;
	int32_t			i;

#if !DBG
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed)
	return;
#endif
if (slot < 0)
	missionManager.LevelStateName (szFile, -slot);
else
	sprintf (szFile, "%s.mg%d", LOCALPLAYER.callsign, slot);
gameData.app.bGamePaused = 1;
for (i = 0; i < N_PLAYERS; i++)
	MultiStripRobots (i);
if ((uint32_t) saveGameManager.GetGameId (szFile, (slot < 0) ? -1 : 0) != id) {
	MultiBadRestore ();
	gameData.app.bGamePaused = 0;
	return;
	}
int32_t bGameRunning = gameStates.app.bGameRunning;
saveGameManager.LoadState (1, (slot < 0) ? -1 : 0, szFile);
gameStates.app.bGameRunning = bGameRunning;
ogl.RebuildContext (1);
gameData.app.bGamePaused = 0;
}

//-----------------------------------------------------------------------------

void ExtractNetPlayerStats (tNetPlayerStats *ps, CPlayerInfo * pd)
{
	int32_t i;

ps->flags = INTEL_INT (pd->flags);                                   // Powerup flags, see below...
ps->energy = (fix)INTEL_INT (pd->energy);                            // Amount of energy remaining.
ps->shield = (fix)INTEL_INT (pd->shield);                          // shield remaining (protection)
ps->lives = pd->lives;                                              // Lives remaining, 0 = game over.
ps->laserLevel = pd->laserLevel;                                  // Current level of the laser.
ps->primaryWeaponFlags = (uint8_t) pd->primaryWeaponFlags;                  // bit set indicates the player has this weapon.
ps->secondaryWeaponFlags = (uint8_t) pd->secondaryWeaponFlags;              // bit set indicates the player has this weapon.
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	ps->primaryAmmo [i] = INTEL_SHORT (pd->primaryAmmo [i]);
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
	ps->secondaryAmmo [i] = INTEL_SHORT (pd->secondaryAmmo [i]);
//memcpy (ps->primaryAmmo, pd->primaryAmmo, MAX_PRIMARY_WEAPONS*sizeof (int16_t));        // How much ammo of each nType.
//memcpy (ps->secondaryAmmo, pd->secondaryAmmo, MAX_SECONDARY_WEAPONS*sizeof (int16_t)); // How much ammo of each nType.
ps->lastScore = INTEL_INT (pd->lastScore);                           // Score at beginning of current level.
ps->score = INTEL_INT (pd->score);                                     // Current score.
ps->cloakTime = (fix)INTEL_INT (pd->cloakTime);                      // Time cloaked
ps->homingObjectDist = (fix)INTEL_INT (pd->homingObjectDist);      // Distance of nearest homing CObject.
ps->invulnerableTime = (fix)INTEL_INT (pd->invulnerableTime);        // Time invulnerable
ps->nScoreGoalCount = INTEL_SHORT (pd->nScoreGoalCount);
ps->netKilledTotal = INTEL_SHORT (pd->netKilledTotal);             // Number of times nKilled total
ps->netKillsTotal = INTEL_SHORT (pd->netKillsTotal);               // Number of net kills total
ps->numKillsLevel = INTEL_SHORT (pd->numKillsLevel);               // Number of kills this level
ps->numKillsTotal = INTEL_SHORT (pd->numKillsTotal);               // Number of kills total
ps->numRobotsLevel = INTEL_SHORT (pd->numRobotsLevel);             // Number of initial robots this level
ps->numRobotsTotal = INTEL_SHORT (pd->numRobotsTotal);             // Number of robots total
ps->nHostagesRescued = INTEL_SHORT (pd->hostages.nRescued); // Total number of hostages rescued.
ps->nHostagesTotal = INTEL_SHORT (pd->hostages.nTotal);                 // Total number of hostages.
ps->nHostagesOnBoard = pd->hostages.nOnBoard;                        // Number of hostages on ship.
}

//-----------------------------------------------------------------------------

void UseNetPlayerStats (CPlayerInfo * ps, tNetPlayerStats *pd)
{
	int32_t i;

ps->flags = INTEL_INT (pd->flags);                       // Powerup flags, see below...
ps->energy = (fix)INTEL_INT ((int32_t)pd->energy);           // Amount of energy remaining.
ps->shield = (fix)INTEL_INT ((int32_t)pd->shield);         // shield remaining (protection)
ps->lives = pd->lives;                                  // Lives remaining, 0 = game over.
ps->laserLevel = pd->laserLevel;                      // Current level of the laser.
ps->primaryWeaponFlags = pd->primaryWeaponFlags;      // bit set indicates the player has this weapon.
ps->secondaryWeaponFlags = pd->secondaryWeaponFlags;  // bit set indicates the player has this weapon.
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	ps->primaryAmmo [i] = INTEL_SHORT (pd->primaryAmmo [i]);
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
	ps->secondaryAmmo [i] = INTEL_SHORT (pd->secondaryAmmo [i]);
//memcpy (ps->primaryAmmo, pd->primaryAmmo, MAX_PRIMARY_WEAPONS*sizeof (int16_t));  // How much ammo of each nType.
//memcpy (ps->secondaryAmmo, pd->secondaryAmmo, MAX_SECONDARY_WEAPONS*sizeof (int16_t)); // How much ammo of each nType.
ps->lastScore = INTEL_INT (pd->lastScore);             // Score at beginning of current level.
ps->score = INTEL_INT (pd->score);                       // Current score.
ps->cloakTime = (fix)INTEL_INT ((int32_t)pd->cloakTime);   // Time cloaked
ps->homingObjectDist = (fix)INTEL_INT ((int32_t)pd->homingObjectDist); // Distance of nearest homing CObject.
ps->invulnerableTime = (fix)INTEL_INT ((int32_t)pd->invulnerableTime); // Time invulnerable
ps->nScoreGoalCount = INTEL_SHORT (pd->nScoreGoalCount);
ps->netKilledTotal = INTEL_SHORT (pd->netKilledTotal); // Number of times nKilled total
ps->netKillsTotal = INTEL_SHORT (pd->netKillsTotal); // Number of net kills total
ps->numKillsLevel = INTEL_SHORT (pd->numKillsLevel); // Number of kills this level
ps->numKillsTotal = INTEL_SHORT (pd->numKillsTotal); // Number of kills total
ps->numRobotsLevel = INTEL_SHORT (pd->numRobotsLevel); // Number of initial robots this level
ps->numRobotsTotal = INTEL_SHORT (pd->numRobotsTotal); // Number of robots total
ps->hostages.nRescued = INTEL_SHORT (pd->nHostagesRescued); // Total number of hostages rescued.
ps->hostages.nTotal = INTEL_SHORT (pd->nHostagesTotal);   // Total number of hostages.
ps->hostages.nOnBoard = pd->nHostagesOnBoard;            // Number of hostages on ship.
}

//-----------------------------------------------------------------------------

void MultiSendDropWeapon (int32_t nObject)
{
	CObject *objP;
	int32_t bufP = 0;
	int32_t ammoCount;

if (nObject < 0)
	return;
objP = OBJECT (nObject);
if (!objP)
	return;
ammoCount = objP->cType.powerupInfo.nCount;
if (objP->info.nId == POW_OMEGA && ammoCount == I2X (1))
	ammoCount = I2X (1) - 1; //make fit in int16_t

gameData.multigame.msg.buf [bufP++] = MULTI_DROP_WEAPON;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = (uint8_t)objP->info.nId;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, N_LOCALPLAYER);
bufP += 2;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, nObject);
bufP += 2;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, ammoCount);
bufP += 2;
PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, gameStates.app.nRandSeed);
SetLocalObjNumMapping (nObject);
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 2);
}

//-----------------------------------------------------------------------------

void MultiDoDropWeapon (uint8_t* buf)
{
	int32_t	bufP = 1;

CHECK_MSG_ID

uint8_t powerupId = (uint8_t) (buf [bufP++]);
int32_t nPlayer = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
int16_t nRemoteObj = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
int16_t ammo = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
int32_t seed = GET_INTEL_INT (buf + bufP);
bufP += 4;
CObject* objP = OBJECT (PLAYER (nPlayer).nObject);
if (objP) {
	int16_t nObject = SpitPowerup (objP, powerupId, seed);
	if	(nObject >= 0) {
		SetObjNumMapping (nObject, nRemoteObj, nPlayer);
		OBJECT (nObject)->cType.powerupInfo.nCount = ammo;
		}
	}
}

//-----------------------------------------------------------------------------

void MultiSendGuidedInfo (CObject *missileP, char done)
{
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tShortPos sp;
#endif
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_GUIDED;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = (uint8_t) N_LOCALPLAYER;
gameData.multigame.msg.buf [bufP++] = done;
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
if (gameStates.multi.nGameType < UDP_GAME) {
	CreateShortPos (reinterpret_cast<tShortPos*> (gameData.multigame.msg.buf + bufP), missileP, 0);
	bufP += sizeof (tShortPos);
	}
else {
	CreateLongPos (reinterpret_cast<tLongPos*> (gameData.multigame.msg.buf + bufP), missileP);
	bufP += sizeof (tLongPos);
	}
#else
CreateShortPos (&sp, miss, 1);
memcpy (&(gameData.multigame.msg.buf [bufP]), reinterpret_cast<uint8_t*> (sp.orient), 9);
bufP += 9;
memcpy (&(gameData.multigame.msg.buf [bufP]), reinterpret_cast<uint8_t*> (&sp.coord), 14);
bufP += 14;
#endif
if (done)
	SET_MSG_ID // only enforce message receipt confirmation if player has released the guided missile
MultiSendData (gameData.multigame.msg.buf, bufP, 0);
}

//-----------------------------------------------------------------------------

void MultiDoGuided (uint8_t* buf)
{
	int32_t	bufP = 1;

CHECK_MSG_ID

int32_t nPlayer = int32_t (buf [bufP++]);

static int32_t fun = 200;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tShortPos sp;
#endif
CObject *missileP = gameData.objData.GetGuidedMissile (nPlayer);

if (missileP == NULL) {
	if (++fun >= 50)
		fun = 0;
	return;
	}
else if (++fun >= 50)
	fun = 0;

if (buf [bufP++]) {
	ReleaseGuidedMissile (nPlayer);
	return;
	}
if ((missileP < OBJECTS.Buffer ()) ||
		(missileP - OBJECTS > gameData.objData.nLastObject [0])) {
	Int3 ();  // Get Jason immediately!
	return;
	}
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
if (gameStates.multi.nGameType < UDP_GAME) 
	ExtractShortPos (missileP, reinterpret_cast<tShortPos*> (buf + bufP), 0);
else
	ExtractLongPos (missileP, reinterpret_cast<tLongPos*> (buf + bufP));
#else
memcpy (reinterpret_cast<uint8_t*>(sp.orient), reinterpret_cast<uint8_t*>(buf + bufP), 9);
memcpy (reinterpret_cast<uint8_t*> (&(sp.coord)), reinterpret_cast<uint8_t*> (buf + bufP + 9), 14);
ExtractShortPos (missileP, &sp, 1);
#endif
bufP += sizeof (tShortPos);
UpdateObjectSeg (missileP);
}

//-----------------------------------------------------------------------------

void MultiSendStolenItems (void)
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_STOLEN_ITEMS;
ADD_MSG_ID
for (int32_t i = 0; i < MAX_STOLEN_ITEMS; i++)
	gameData.multigame.msg.buf [bufP++] = gameData.thief.stolenItems [i];
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
}

//-----------------------------------------------------------------------------

void MultiDoStolenItems (uint8_t* buf)
{
	int32_t bufP = 1;

CHECK_MSG_ID

for (int32_t i = 0; i < MAX_STOLEN_ITEMS; i++)
	gameData.thief.stolenItems [i] = buf [bufP++];
}

//-----------------------------------------------------------------------------

void MultiSendWallStatus (int32_t nWall, uint8_t nType, uint16_t flags, uint8_t state)
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_WALL_STATUS;
ADD_MSG_ID
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, nWall);
bufP += 2;
gameData.multigame.msg.buf [bufP++] = nType;
gameData.multigame.msg.buf [bufP++] = uint8_t (flags);
gameData.multigame.msg.buf [bufP++] = state;
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 1); // twice, just to be sure
if (gameStates.multi.nGameType < UDP_GAME)
	MultiSendData (gameData.multigame.msg.buf, bufP, 1);
}

//-----------------------------------------------------------------------------

void MultiSendWallStatusSpecific (int32_t nPlayer, int32_t nWall, uint8_t nType, uint16_t flags, uint8_t state)
{
	// Send wall states a specific rejoining player
	int16_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_WALL_STATUS;
ADD_MSG_ID
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, nWall);
bufP += 2;
gameData.multigame.msg.buf [bufP++] = nType;
gameData.multigame.msg.buf [bufP++] = uint8_t (flags);
gameData.multigame.msg.buf [bufP++] = state;
// This message is only sent during initial sync'ing when a new player enters a match and can be sent a lot
// do not set the message id here to avoid generating tons of message confirmations
NetworkSendMineSyncPacket (gameData.multigame.msg.buf, bufP, nPlayer); // twice, just to be sure
if (gameStates.multi.nGameType < UDP_GAME)
	NetworkSendMineSyncPacket (gameData.multigame.msg.buf, bufP, nPlayer);
}

//-----------------------------------------------------------------------------

void MultiDoWallStatus (uint8_t* buf)
{
	int32_t	bufP = 1;

CHECK_MSG_ID

int16_t nWall = GET_INTEL_SHORT (buf + bufP);
bufP += 2;

if (nWall >= gameData.wallData.nWalls)
	return;

CWall& wall = *WALL (nWall);

wall.nType = buf [bufP++];
wall.flags = buf [bufP++];
wall.state = buf [bufP];
if (wall.nType == WALL_OPEN)
	audio.DestroySegmentSound ((int16_t) wall.nSegment, (int16_t) wall.nSide, SOUND_FORCEFIELD_HUM);
}

//-----------------------------------------------------------------------------

void MultiSendJasonCheat (int32_t num)
{
return;
}

//-----------------------------------------------------------------------------

void MultiSendScoreGoalCounts ()
{
	int32_t h, i, bufP = 0;
	
gameData.multigame.msg.buf [bufP++] = MULTI_SCOREGOALS;
ADD_MSG_ID
h = (gameStates.multi.nGameType == UDP_GAME) ? MAX_PLAYERS_D2X : MAX_PLAYERS_D2;
for (i = 0; i < h; i++)
		gameData.multigame.msg.buf [bufP++] = (uint8_t) PLAYER (i).nScoreGoalCount;
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
}

//-----------------------------------------------------------------------------

void MultiDoScoreGoalCounts (uint8_t* buf)
{
	int32_t h, i, bufP = 1;

CHECK_MSG_ID

h = (gameStates.multi.nGameType == UDP_GAME) ? MAX_PLAYERS_D2X : MAX_PLAYERS_D2;
for (i = 0; i < h; i++)
	PLAYER (i).nScoreGoalCount = buf [bufP++];
}

//-----------------------------------------------------------------------------

void MultiSendHeartBeat ()
{
if (netGameInfo.GetPlayTimeAllowed ()) {
	gameData.multigame.msg.buf [0] = MULTI_HEARTBEAT;
	PUT_INTEL_INT (gameData.multigame.msg.buf+1, gameStates.app.xThisLevelTime);
	MultiSendData (gameData.multigame.msg.buf, 5, 0);
	}
}

//-----------------------------------------------------------------------------

void MultiDoHeartBeat (uint8_t* buf)
{
fix num = GET_INTEL_INT (buf + 1);
gameStates.app.xThisLevelTime = num;
}

//-----------------------------------------------------------------------------

void MultiCheckForEntropyWinner ()
{
#if 1//!DBG
	CSegment*	segP;
	int32_t			h, i, t;
	char			bGotRoom [2] = {0, 0};

	static int32_t		countDown;

if (!IsEntropyGame)
	return;
#if 1//!DBG
if (gameData.reactor.bDestroyed) {
	if (gameStates.app.nSDLTicks [0] - countDown  >= 5000)
		StopEndLevelSequence ();
	return;
	}
#endif
countDown = -1;
gameStates.entropy.bExitSequence = 0;
for (i = 0, segP = SEGMENTS.Buffer (); i <= gameData.segData.nLastSegment; i++, segP++)
	if ((t = (int32_t) segP->m_owner) > 0) {
		bGotRoom [--t] = 1;
		if (bGotRoom [!t])
			return;
	}
for (t = 0; t < 2; t++)
	if (bGotRoom [t])
		break;
if (t == 2)	// no team as at least one room -> this is probably not an entropy enaszPlayerd level
	return;
for (h = i = 0; i < N_PLAYERS; i++)
	if (GetTeam (i) != t) {
		if (PLAYER (i).secondaryAmmo [PROXMINE_INDEX]  >= extraGameInfo [1].entropy.nCaptureVirusThreshold)
			return;
		h += PLAYER (i).secondaryAmmo [PROXMINE_INDEX];
		}
if ((h  >= extraGameInfo [1].entropy.nCaptureVirusThreshold) && extraGameInfo [1].entropy.nVirusStability)
	return;
HUDInitMessage (TXT_WINNING_TEAM, t ? TXT_RED : TXT_BLUE);
for (i = 0, segP = SEGMENTS.Buffer (); i <= gameData.segData.nLastSegment; i++, segP++) {
	if (segP->m_owner != t + 1)
		segP->m_owner = t + 1;
	segP->ChangeTexture (-1);
	}
gameStates.entropy.bExitSequence = 1;
for (i = 0; i < N_PLAYERS; i++)
	if ((GetTeam (i) != t) && (PLAYER (i).Shield () >= 0))
		return;
countDown = gameStates.app.nSDLTicks [0];
#if 1//!DBG
gameData.reactor.bDestroyed = 1;
gameData.reactor.countdown.nTimer = -1;
#endif
#else
if (gameData.reactor.bDestroyed)
	StopEndLevelSequence ();
else {
	gameData.reactor.bDestroyed = 1;
	gameData.reactor.countdown.nTimer = -1;
	}
#endif
}

//-----------------------------------------------------------------------------

void MultiRemoveGhostShips (void)
{
	uint8_t	bHaveObject [MAX_PLAYERS];
	CObject*	objP;

memset (bHaveObject, 0, sizeof (bHaveObject));
networkThread.LockThread ();
FORALL_PLAYER_OBJS (objP) {
	if (objP->info.nType != OBJ_PLAYER)
		continue;
	if (bHaveObject [objP->info.nId] || (objP->info.nId >= N_PLAYERS))
		objP->Die ();
	else {
		if (!PLAYER (objP->info.nId).IsConnected ()) {
			objP->SetType (OBJ_GHOST);
			objP->info.renderType = RT_NONE;
			objP->info.movementType = MT_NONE;
			}
		bHaveObject [objP->info.nId] = 1;
		}
	}
networkThread.UnlockThread ();
}

//-----------------------------------------------------------------------------

void MultiCheckForScoreGoalWinner (bool bForce)
{
	int32_t h = 0, nPlayer = -1;

if (gameData.reactor.bDestroyed < 0)
	return;
for (int32_t i = 0; i < N_PLAYERS; i++)
	if (h < PLAYER (i).nScoreGoalCount) {
		h = PLAYER (i).nScoreGoalCount;
		nPlayer = i;
		}
CheckScoreGoal (nPlayer, bForce);
}

//-----------------------------------------------------------------------------

void MultiSendSeismic (fix start, fix end)
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_SEISMIC;
ADD_MSG_ID
PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, start);
bufP += 4;
PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, end);
bufP += 4;
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
}

//-----------------------------------------------------------------------------

void MultiDoSeismic (uint8_t* buf)
{
	int32_t bufP = 1;

CHECK_MSG_ID

gameStates.gameplay.seismic.nStartTime = GET_INTEL_INT (buf + bufP);
bufP += 4;
gameStates.gameplay.seismic.nEndTime = GET_INTEL_INT (buf + bufP);
audio.PlaySound (SOUND_SEISMIC_DISTURBANCE_START);
}

//-----------------------------------------------------------------------------

void MultiSendLight (int32_t nSegment, uint8_t val)
{
int32_t bufP = 0;
gameData.multigame.msg.buf [bufP++] = MULTI_LIGHT;
ADD_MSG_ID
PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, nSegment);
bufP += sizeof (int32_t);
gameData.multigame.msg.buf [bufP++] = val;
CSegment* segP = SEGMENT (nSegment);
for (int32_t i = 0; i < 6; i++, bufP += 2)
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, (segP->m_sides [i].FaceCount ()) ? segP->m_sides [i].m_nOvlTex : 0);
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
}

//-----------------------------------------------------------------------------

void MultiSendLightSpecific (int32_t nPlayer, int32_t nSegment, uint8_t val)
{
	int16_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_LIGHT;
ADD_MSG_ID
PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, nSegment);
bufP += sizeof (int32_t);
gameData.multigame.msg.buf [bufP++] = val;
CSegment* segP = SEGMENT (nSegment);
for (int32_t i = 0; i < 6; i++, bufP += 2)
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, (segP->m_sides [i].FaceCount ()) ? segP->m_sides [i].m_nOvlTex : 0);
// Multiple sync message for joining players - do not set message id to avoid confirmation message spamming
NetworkSendMineSyncPacket (gameData.multigame.msg.buf, bufP, nPlayer);
}

//-----------------------------------------------------------------------------

void MultiDoLight (uint8_t* buf)
{
	int32_t bufP = 1;

CHECK_MSG_ID
	
int16_t nSegment = GET_INTEL_INT (buf + bufP);
bufP += 4;
uint8_t sides = buf [bufP++];

CSegment* segP = SEGMENT (nSegment);
for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++, bufP += 2) {
	if (segP->m_sides [i].FaceCount () && (sides & (1 << i))) {
		SubtractLight (nSegment, i);
		segP->m_sides [i].m_nOvlTex = GET_INTEL_SHORT (buf + bufP);
		}
	}
}

//-----------------------------------------------------------------------------

void MultiDoFlags (uint8_t* buf)
{
	int32_t bufP = 1;

CHECK_MSG_ID

int32_t nPlayer = int32_t (buf [bufP++]);
uint32_t flags = GET_INTEL_INT (buf + bufP);
if (nPlayer != N_LOCALPLAYER)
	PLAYER (nPlayer).flags = flags;
}

//-----------------------------------------------------------------------------

void MultiSendFlags (uint8_t nPlayer)
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_FLAGS;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = nPlayer;
PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, PLAYER (int32_t (nPlayer)).flags);
bufP += 4;
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
}

//-----------------------------------------------------------------------------

void MultiDoWeapons (uint8_t* buf)
{
	int32_t	i, bufP = 1;
	int32_t	nPlayer = int32_t (buf [bufP++]);
	CPlayerData& player = PLAYER (nPlayer);

player.SetShield (GET_INTEL_INT (buf + bufP));
bufP += 4;
player.flags = int32_t (GET_INTEL_INT (buf + bufP));
bufP += 4;
player.primaryWeaponFlags = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++) {
	player.secondaryAmmo [i] = GET_INTEL_SHORT (buf + bufP);
	bufP += 2;
	}
player.SetLaserLevels (buf [bufP], buf [bufP + 1]);
bufP += 2;
gameData.multiplayer.weaponStates [nPlayer].nAmmoUsed = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
gameData.multiplayer.weaponStates [nPlayer].nBuiltinMissiles = buf [bufP];
player.m_tWeaponInfo = gameStates.app.nSDLTicks [0];
}

//-----------------------------------------------------------------------------

void MultiDoAmmo (uint8_t* buf)
{
	int32_t	nPlayer = int32_t (buf [1]);

PLAYER (int32_t (nPlayer)).primaryAmmo [VULCAN_INDEX] = uint16_t (GET_INTEL_SHORT (buf + 2));
gameData.multiplayer.weaponStates [int32_t (nPlayer)].nAmmoUsed = GET_INTEL_SHORT (buf + 4);
}

//-----------------------------------------------------------------------------

void MultiDoFusionCharge (uint8_t* buf)
{
gameData.multiplayer.weaponStates [int32_t (buf [1])].xFusionCharge = GET_INTEL_INT (buf + 2);
}

//-----------------------------------------------------------------------------

void MultiSendWeapons (int32_t bForce)
{
if (!IsMultiGame || (gameStates.multi.nGameType != UDP_GAME))
	return;

	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_WEAPONS;
gameData.multigame.msg.buf [bufP++] = N_LOCALPLAYER;
PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, LOCALPLAYER.Shield ());
bufP += 4;
PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, LOCALPLAYER.flags);
bufP += 4;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, LOCALPLAYER.primaryWeaponFlags);
bufP += 2;
for (int32_t i = 0; i < MAX_SECONDARY_WEAPONS; i++) {
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, LOCALPLAYER.secondaryAmmo [i]);
	bufP += 2;
	}
gameData.multigame.msg.buf [bufP++] = LOCALPLAYER.LaserLevel (0);
gameData.multigame.msg.buf [bufP++] = LOCALPLAYER.LaserLevel (1);
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, gameData.multiplayer.weaponStates [N_LOCALPLAYER].nAmmoUsed);
bufP += 2;
gameData.multigame.msg.buf [bufP++] = gameData.multiplayer.weaponStates [N_LOCALPLAYER].nBuiltinMissiles;
MultiSendData (gameData.multigame.msg.buf, bufP, bForce);
MultiSendShield ();
}

//-----------------------------------------------------------------------------

void MultiSendAmmo (void)
{
if (!IsMultiGame || (gameStates.multi.nGameType != UDP_GAME))
	return;

	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_AMMO;
gameData.multigame.msg.buf [bufP++] = N_LOCALPLAYER;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, int16_t (LOCALPLAYER.primaryAmmo [VULCAN_INDEX]));
bufP += 2;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, gameData.multiplayer.weaponStates [N_LOCALPLAYER].nAmmoUsed);
bufP += 2;
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
}

//-----------------------------------------------------------------------------

void MultiSendFusionCharge (void)
{
if (!IsMultiGame || (gameStates.multi.nGameType != UDP_GAME))
	return;

	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_FUSION_CHARGE;
gameData.multigame.msg.buf [bufP++] = N_LOCALPLAYER;
PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, int32_t (gameData.FusionCharge ()));
bufP += 4;
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
}

//-----------------------------------------------------------------------------

void MultiDoMonsterball (uint8_t* buf)
{
	int32_t			bCreate, bufP = 1;
	int16_t			nSegment;
	CFixVector	v;

bCreate = (int32_t) buf [bufP++];
nSegment = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
v.v.coord.x = GET_INTEL_INT (buf + bufP);
bufP += 4;
v.v.coord.y = GET_INTEL_INT (buf + bufP);
bufP += 4;
v.v.coord.z = GET_INTEL_INT (buf + bufP);
bufP += 4;
if (bCreate || !gameData.hoard.monsterballP) {
	gameData.hoard.nMonsterballSeg = FindSegByPos (v, nSegment, 1, 0);
	gameData.hoard.vMonsterballPos = v;
	CreateMonsterball ();
	gameData.hoard.monsterballP->info.position.vPos = v;
	}
v.v.coord.x = GET_INTEL_INT (buf + bufP);
bufP += 4;
v.v.coord.y = GET_INTEL_INT (buf + bufP);
bufP += 4;
v.v.coord.z = GET_INTEL_INT (buf + bufP);
bufP += 4;
gameData.hoard.monsterballP->ApplyForce (v);
v.v.coord.x = GET_INTEL_INT (buf + bufP);
bufP += 4;
v.v.coord.y = GET_INTEL_INT (buf + bufP);
bufP += 4;
v.v.coord.z = GET_INTEL_INT (buf + bufP);
bufP += 4;
gameData.hoard.monsterballP->ApplyRotForce (v);
}

//-----------------------------------------------------------------------------

void MultiSendMonsterball (int32_t bForce, int32_t bCreate)
{
	int32_t t = gameStates.app.nSDLTicks [0];

	static int32_t nTimeout = 0;

if (!(gameData.app.GameMode (GM_MONSTERBALL)))
	return;
if (!gameData.hoard.monsterballP)
	return;
if (bForce || (t - nTimeout > 1000)) {
		int32_t bufP = 0;

	nTimeout = t;
	gameData.multigame.msg.buf [bufP++] = MULTI_MONSTERBALL;
	gameData.multigame.msg.buf [bufP++] = (char) bCreate;
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, gameData.hoard.monsterballP->info.nSegment);
	bufP += 2;
	PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, gameData.hoard.monsterballP->info.position.vPos.v.coord.x);
	bufP += 4;
	PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, gameData.hoard.monsterballP->info.position.vPos.v.coord.y);
	bufP += 4;
	PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, gameData.hoard.monsterballP->info.position.vPos.v.coord.z);
	bufP += 4;
	PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, gameData.hoard.monsterballP->mType.physInfo.velocity.v.coord.x);
	bufP += 4;
	PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, gameData.hoard.monsterballP->mType.physInfo.velocity.v.coord.y);
	bufP += 4;
	PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, gameData.hoard.monsterballP->mType.physInfo.velocity.v.coord.z);
	bufP += 4;
	PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, gameData.hoard.monsterballP->mType.physInfo.rotVel.v.coord.x);
	bufP += 4;
	PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, gameData.hoard.monsterballP->mType.physInfo.rotVel.v.coord.y);
	bufP += 4;
	PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, gameData.hoard.monsterballP->mType.physInfo.rotVel.v.coord.z);
	bufP += 4;
	MultiSendData (gameData.multigame.msg.buf, bufP, 1);
	}
}

//-----------------------------------------------------------------------------

void MultiSendDropBlobs (uint8_t nPlayer)
{
gameData.multigame.msg.buf [0] = MULTI_DROP_BLOB;
gameData.multigame.msg.buf [1] = nPlayer;
MultiSendData (gameData.multigame.msg.buf, 2, 0);
}

//-----------------------------------------------------------------------------

void MultiDoDropBlob (uint8_t* buf)
{
DropAfterburnerBlobs (PLAYEROBJECT (int32_t (buf [1])), 2, I2X (1), -1, NULL, 0);
}

//-----------------------------------------------------------------------------

void MultiSendPowerupUpdate (void)
{
if (IAmGameHost ()) {
	int32_t bufP = 0;
	gameData.multigame.msg.buf [bufP++] = MULTI_POWERUP_UPDATE;
	ADD_MSG_ID
	for (int32_t i = 0; i < MAX_POWERUP_TYPES; i++) {
		PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, gameData.multiplayer.maxPowerupsAllowed [i]);
		bufP += sizeof (uint16_t);
		}
	SET_MSG_ID
	MultiSendData (gameData.multigame.msg.buf, bufP, 1);
	}
}

//-----------------------------------------------------------------------------

void MultiDoPowerupUpdate (uint8_t* buf)
{
if (!IAmGameHost ()) {
	int32_t bufP = 1;
	CHECK_MSG_ID
	for (int32_t i = 0; i < MAX_POWERUP_TYPES; i++) {
		gameData.multiplayer.maxPowerupsAllowed [i] = GET_INTEL_SHORT (buf + bufP);
		bufP += sizeof (uint16_t);
		}
	}
}

//-----------------------------------------------------------------------------

#if 0 // never used...
void MultiSendActiveDoor (int32_t i)
{
	int32_t count;

gameData.multigame.msg.buf [0] = MULTI_ACTIVE_DOOR;
gameData.multigame.msg.buf [1] = i;
gameData.multigame.msg.buf [2] = gameData.wallData.nOpenDoors;
count = 3;
memcpy (gameData.multigame.msg.buf + 3, gameData.wallData.activeDoors + i, sizeof (CActiveDoor);
count += sizeof (CActiveDoor);
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
CActiveDoor *doorP = reinterpret_cast<CActiveDoor*> (gameData.multigame.msg.buf + 3);
doorP->nPartCount = INTEL_INT (doorP->nPartCount);
doorP->nFrontWall [0] = INTEL_SHORT (doorP->nFrontWall [0]);
doorP->nFrontWall [1] = INTEL_SHORT (doorP->nFrontWall [1]);
doorP->nBackWall [0] = INTEL_SHORT (doorP->nBackWall [0]);
doorP->nBackWall [1] = INTEL_SHORT (doorP->nBackWall [1]);
doorP->time = INTEL_INT (doorP->time);
#endif
//MultiSendData (gameData.multigame.msg.buf, sizeof (CActiveDoor)+3, 1);
MultiSendData (gameData.multigame.msg.buf, count, 1);
}
#endif // 0 (never used)

//-----------------------------------------------------------------------------

void MultiDoActiveDoor (uint8_t* buf)
{
CActiveDoor *doorP = reinterpret_cast<CActiveDoor*> (gameData.multigame.msg.buf + 3);
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
doorP->nPartCount = INTEL_INT (doorP->nPartCount);
doorP->nFrontWall [0] = INTEL_SHORT (doorP->nFrontWall [0]);
doorP->nFrontWall [1] = INTEL_SHORT (doorP->nFrontWall [1]);
doorP->nBackWall [0] = INTEL_SHORT (doorP->nBackWall [0]);
doorP->nBackWall [1] = INTEL_SHORT (doorP->nBackWall [1]);
doorP->time = INTEL_INT (doorP->time);
#endif //WORDS_BIGENDIAN
if (!FindActiveDoor (doorP->nFrontWall [0]))
	gameData.wallData.activeDoors.Push (*doorP);
}

//-----------------------------------------------------------------------------

void MultiSendSoundFunction (char nFunction, char sound)
{
gameData.multigame.msg.buf [0] = MULTI_SOUND_FUNCTION;
gameData.multigame.msg.buf [1] = N_LOCALPLAYER;
gameData.multigame.msg.buf [2] = nFunction;
gameData.multigame.msg.buf [3] = sound;
MultiSendData (gameData.multigame.msg.buf, 4, 0);
}

//-----------------------------------------------------------------------------

#define AFTERBURNER_LOOP_START  20098
#define AFTERBURNER_LOOP_END    25776

void MultiDoSoundFunction (uint8_t* buf)
{
// for afterburner
if (!LOCALPLAYER.Connected (CONNECT_PLAYING))
	return;
int32_t nPlayer = int32_t (buf [1]);
int32_t nFunction = int32_t (buf [2]);
if (nFunction == 0)
	audio.DestroyObjectSound (PLAYER (nPlayer).nObject);
else if (nFunction == 3) {
	int16_t nSound = int16_t (uint8_t (buf [3]));
	if (nSound == SOUND_AFTERBURNER_PLAY)
		audio.CreateObjectSound ((int16_t) SOUND_AFTERBURNER_PLAY, SOUNDCLASS_PLAYER, int16_t (PLAYER (nPlayer).nObject));
	else
		audio.CreateObjectSound (nSound, SOUNDCLASS_PLAYER, int16_t (PLAYER (nPlayer).nObject), 1, I2X (1), I2X (256),
										 AFTERBURNER_LOOP_START, AFTERBURNER_LOOP_END);
	}
}

//-----------------------------------------------------------------------------

void MultiSendCaptureBonus (uint8_t nPlayer)
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_CAPTURE_BONUS;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = nPlayer;
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
MultiDoCaptureBonus (gameData.multigame.msg.buf);
}

//-----------------------------------------------------------------------------

void MultiSendOrbBonus (uint8_t nPlayer)
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_ORB_BONUS;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = nPlayer;
gameData.multigame.msg.buf [bufP++] = (uint8_t) LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX];
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
MultiDoOrbBonus (gameData.multigame.msg.buf);
}

//-----------------------------------------------------------------------------

void MultiDoCaptureBonus (uint8_t* buf)
{
	// Figure out the results of a network kills and add it to the
	// appropriate player's tally.
	int32_t bufP = 1;

CHECK_MSG_ID

int32_t nPlayer = int32_t (buf [bufP]);
int16_t penalty = 0;
	//char	szTeam [20];

int16_t bonus = BonusScore ();
gameData.score.nKillsChanged = 1;
if (nPlayer < 0) {
	penalty = 1;
	nPlayer = -nPlayer - 1;
	}
int16_t nTeam = GetTeam (nPlayer) ^ penalty;
if (nPlayer == N_LOCALPLAYER)
	HUDInitMessage (TXT_SCORED);
else
	HUDInitMessage (TXT_SCORED2, PLAYER (nPlayer).callsign);
gameData.multigame.score.nTeam [nTeam] += bonus;
if (nPlayer == N_LOCALPLAYER)
	audio.PlaySound (SOUND_HUD_YOU_GOT_GOAL, SOUNDCLASS_GENERIC, I2X (2));
else if (GetTeam (nPlayer) == TEAM_RED)
	audio.PlaySound (SOUND_HUD_RED_GOT_GOAL, SOUNDCLASS_GENERIC, I2X (2));
else
	audio.PlaySound (SOUND_HUD_BLUE_GOT_GOAL, SOUNDCLASS_GENERIC, I2X (2));
PLAYER (nPlayer).flags &= ~(PLAYER_FLAGS_FLAG);  // Clear capture flag
if (penalty) {
#if 0
	PLAYER (nPlayer).netKillsTotal -= bonus;
	PLAYER (nPlayer).nScoreGoalCount -= bonus;
	if (gameData.multigame.score.nTeam [nTeam] >= ScoreGoal ()) {
		sprintf (szTeam, "%s Team", nTeam ? TXT_RED : TXT_BLUE);
		HUDInitMessage (TXT_REACH_SCOREGOAL, szTeam);
		HUDInitMessage (TXT_CTRLCEN_DEAD);
		NetDestroyReactor (ObjFindFirstOfType (OBJ_REACTOR));
		}
#endif
	}
else {
	PLAYER (nPlayer).netKillsTotal += bonus;
	PLAYER (nPlayer).nScoreGoalCount += bonus;
	CheckScoreGoal (nPlayer);
	}
MultiSortKillList ();
MultiShowPlayerList ();
}

//-----------------------------------------------------------------------------

int32_t GetOrbBonus (char num)
{
return num * (num + 1) / 2; // 1 + 2 + ... + num
}

//-----------------------------------------------------------------------------
	// Figure out the results of a network kills and add it to the
	// appropriate player's tally.

void MultiDoOrbBonus (uint8_t* buf)
{
	int32_t bufP = 1;

CHECK_MSG_ID

int32_t nPlayer = int32_t (buf [bufP++]);
int32_t bonus = GetOrbBonus (buf [bufP]);
int32_t nTeam = GetTeam (nPlayer);

gameData.score.nKillsChanged = 1;
if (nPlayer == N_LOCALPLAYER)
	HUDInitMessage (TXT_SCORED_ORBS, bonus);
else
	HUDInitMessage (TXT_SCORED_ORBS2, PLAYER (nPlayer).callsign, buf [2]);
if (nPlayer == N_LOCALPLAYER)
	soundQueue.StartSound (SOUND_HUD_YOU_GOT_GOAL, I2X (2));
else if (IsTeamGame) {
	if (nTeam == TEAM_RED)
		audio.PlaySound (SOUND_HUD_RED_GOT_GOAL, SOUNDCLASS_GENERIC, I2X (2));
	else
		audio.PlaySound (SOUND_HUD_BLUE_GOT_GOAL, SOUNDCLASS_GENERIC, I2X (2));
	}
else
	audio.PlaySound (SOUND_OPPONENT_HAS_SCORED, SOUNDCLASS_GENERIC, I2X (2));
if (bonus > gameData.score.nHighscore) {
	if (nPlayer == N_LOCALPLAYER)
		HUDInitMessage (TXT_RECORD, bonus);
	else
		HUDInitMessage (TXT_RECORD2, PLAYER (nPlayer).callsign, bonus);
	audio.PlaySound (SOUND_BUDDY_MET_GOAL, SOUNDCLASS_GENERIC, I2X (2));
	gameData.score.nChampion = nPlayer;
	gameData.score.nHighscore = bonus;
	}
PLAYER (nPlayer).flags &= ~(PLAYER_FLAGS_FLAG);  // Clear orb flag
PLAYER (nPlayer).netKillsTotal += bonus;
PLAYER (nPlayer).netKillsTotal %= 1000;
PLAYER (nPlayer).nScoreGoalCount += bonus;
PLAYER (nPlayer).nScoreGoalCount %= 1000;
gameData.multigame.score.nTeam [nTeam] += bonus;
gameData.multigame.score.nTeam [nTeam] %= 1000;
CheckScoreGoal (nPlayer);
MultiSortKillList ();
MultiShowPlayerList ();
}

//-----------------------------------------------------------------------------

void MultiSendShield (void)
{
gameData.multigame.msg.buf [0] = MULTI_PLAYER_SHIELDS;
gameData.multigame.msg.buf [1] = N_LOCALPLAYER;
PUT_INTEL_INT (gameData.multigame.msg.buf+2, LOCALPLAYER.Shield (false));
MultiSendData (gameData.multigame.msg.buf, 6, 1);
}

//-----------------------------------------------------------------------------

void MultiDoCheating (uint8_t* buf)
{
HUDInitMessage (TXT_PLAYER_CHEATED, PLAYER (int32_t (buf [1])).callsign);
audio.PlaySound (SOUND_CONTROL_CENTER_WARNING_SIREN, SOUNDCLASS_GENERIC, DEFAULT_VOLUME, DEFAULT_PAN, 0, 3);
}

//-----------------------------------------------------------------------------

void MultiSendCheating (void)
{
gameData.multigame.msg.buf [0] = MULTI_CHEATING;
gameData.multigame.msg.buf [1] = N_LOCALPLAYER;
MultiSendData (gameData.multigame.msg.buf, 2, 1);
}

//-----------------------------------------------------------------------------

void MultiSendGotFlag (uint8_t nPlayer)
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_GOT_FLAG;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP] = nPlayer;
soundQueue.StartSound (SOUND_HUD_YOU_GOT_FLAG, I2X (2));
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, 2, 1);
MultiSendFlags (N_LOCALPLAYER);
}

//-----------------------------------------------------------------------------

int32_t bSoundHacked = 0;
CSoundSample reversedSound;

void MultiSendGotOrb (uint8_t nPlayer)
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_GOT_ORB;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = nPlayer;
audio.PlaySound (SOUND_YOU_GOT_ORB, SOUNDCLASS_GENERIC, I2X (2));
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
MultiSendFlags (N_LOCALPLAYER);
}

//-----------------------------------------------------------------------------

void MultiDoGotFlag (uint8_t* buf)
{
	int32_t bufP = 1;

CHECK_MSG_ID

int32_t nPlayer = int32_t (buf [bufP]);
if (nPlayer == N_LOCALPLAYER)
	soundQueue.StartSound (SOUND_HUD_YOU_GOT_FLAG, I2X (2));
else if (GetTeam (nPlayer) == TEAM_RED)
	soundQueue.StartSound (SOUND_HUD_RED_GOT_FLAG, I2X (2));
else
	soundQueue.StartSound (SOUND_HUD_BLUE_GOT_FLAG, I2X (2));
PLAYER (nPlayer).flags |= PLAYER_FLAGS_FLAG;
gameData.pig.flags [!GetTeam (nPlayer)].path.Reset (10, -1);
HUDInitMessage (TXT_PICKFLAG2, PLAYER (nPlayer).callsign);
}

//-----------------------------------------------------------------------------

void MultiDoGotOrb (uint8_t* buf)
{
	int32_t bufP = 1;

CHECK_MSG_ID

	int32_t nPlayer = int32_t (buf [bufP]);

if (IsTeamGame) {
	if (GetTeam (nPlayer) == GetTeam (N_LOCALPLAYER))
		audio.PlaySound (SOUND_FRIEND_GOT_ORB, SOUNDCLASS_GENERIC, I2X (2));
	else
		audio.PlaySound (SOUND_OPPONENT_GOT_ORB, SOUNDCLASS_GENERIC, I2X (2));
   }
else
	audio.PlaySound (SOUND_OPPONENT_GOT_ORB, SOUNDCLASS_GENERIC, I2X (2));
PLAYER (nPlayer).flags|= PLAYER_FLAGS_FLAG;
if (IsEntropyGame)
	HUDInitMessage (TXT_PICKVIRUS2, PLAYER (nPlayer).callsign);
else
	HUDInitMessage (TXT_PICKORB2, PLAYER (nPlayer).callsign);
}

//-----------------------------------------------------------------------------

void DropOrb (void)
{
	int32_t nObject;

if (!(gameData.app.nGameMode &(GM_HOARD | GM_ENTROPY)))
	return; // How did we get here? Get Leighton!
if (!LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX]) {
	HUDInitMessage (IsHoardGame ? TXT_NO_ORBS : TXT_NO_VIRUS);
	return;
	}
nObject = SpitPowerup (gameData.objData.consoleP, POW_HOARD_ORB);
if (nObject < 0)
	return;
HUDInitMessage (IsHoardGame ? TXT_DROP_ORB : TXT_DROP_VIRUS);
audio.PlaySound (SOUND_DROP_WEAPON);
if (nObject > -1)
	if (gameData.app.nGameMode &(GM_HOARD | GM_ENTROPY))
		MultiSendDropFlag (nObject, RandShort ());
// If empty, tell everyone to stop drawing the box around me
if (!--LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX])
	MultiSendFlags (N_LOCALPLAYER);
}

//-----------------------------------------------------------------------------

void DropFlag (void)
{
	int32_t nObject;

if (!(gameData.app.GameMode (GM_CAPTURE)) && !(gameData.app.nGameMode &(GM_HOARD | GM_ENTROPY)))
	return;
if (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)) {
	DropOrb ();
	return;
	}
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_FLAG)) {
	HUDInitMessage (TXT_NO_FLAG);
	return;
	}
HUDInitMessage (TXT_DROP_FLAG);
audio.PlaySound (SOUND_DROP_WEAPON);
nObject = SpitPowerup (gameData.objData.consoleP, (uint8_t) ((GetTeam (N_LOCALPLAYER) == TEAM_RED) ? POW_BLUEFLAG : POW_REDFLAG));
if (nObject < 0)
	return;
if ((gameData.app.GameMode (GM_CAPTURE)) && (nObject > -1))
	MultiSendDropFlag (nObject, RandShort ());
LOCALPLAYER.flags &= ~ (PLAYER_FLAGS_FLAG);
}

//-----------------------------------------------------------------------------

void MultiSendDropFlag (int32_t nObject, int32_t seed)
{
	CObject *objP = OBJECT (nObject);

if (!objP)
	return;

	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_DROP_FLAG;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = (uint8_t) objP->info.nId;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, N_LOCALPLAYER);
bufP += 2;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, nObject);
bufP += 2;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, objP->cType.powerupInfo.nCount);
bufP += 2;
PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, seed);
SetLocalObjNumMapping (nObject);
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 2);
}

//-----------------------------------------------------------------------------

void MultiDoDropFlag (uint8_t* buf)
{
	uint8_t	nPlayer;
	int32_t	bufP = 1;

CHECK_MSG_ID

uint8_t powerupId = (uint8_t) buf [bufP++];
nPlayer = (uint8_t) GET_INTEL_SHORT (buf + bufP);
bufP += 2;
int16_t nRemoteObj = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
int16_t ammo = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
int32_t seed = GET_INTEL_INT (buf + bufP);

CObject* objP = OBJECT (PLAYER (nPlayer).nObject);
if (!objP)
	return;
int16_t nObject = SpitPowerup (objP, powerupId, seed);
objP = OBJECT (nObject);
if (objP) {
	SetObjNumMapping (nObject, nRemoteObj, nPlayer);
	objP->cType.powerupInfo.nCount = ammo;
	if (IsEntropyGame)
		objP->info.nCreator = GetTeam (nPlayer) + 1;
	else if (!(gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY))) {
		PLAYER (nPlayer).flags &= ~(PLAYER_FLAGS_FLAG);
		}
	}
}

//-----------------------------------------------------------------------------

void MultiBadRestore (void)
{
SetFunctionMode (FMODE_MENU);
TextBox (NULL, BG_STANDARD, 1, TXT_OK,
	            "A multi-save game was restored\nthat you are missing or does not\nmatch that of the others.\nYou must rejoin if you wish to\ncontinue.");
SetFunctionMode (FMODE_GAME);
gameData.multigame.bQuitGame = 1;
gameData.multigame.menu.bLeave = 1;
MultiResetStuff ();
}

//-----------------------------------------------------------------------------

void MultiSendRobotControls (uint8_t nPlayer)
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_ROBOT_CONTROLS;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = nPlayer;
memcpy (&(gameData.multigame.msg.buf [bufP]), &gameData.multigame.robots.controlled, MAX_ROBOTS_CONTROLLED * 4);
bufP += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&(gameData.multigame.msg.buf [bufP]), &gameData.multigame.robots.agitation, MAX_ROBOTS_CONTROLLED * 4);
bufP += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&(gameData.multigame.msg.buf [bufP]), &gameData.multigame.robots.controlledTime, MAX_ROBOTS_CONTROLLED * 4);
bufP += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&(gameData.multigame.msg.buf [bufP]), &gameData.multigame.robots.lastSendTime, MAX_ROBOTS_CONTROLLED * 4);
bufP += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&(gameData.multigame.msg.buf [bufP]), &gameData.multigame.robots.lastMsgTime, MAX_ROBOTS_CONTROLLED * 4);
bufP += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&(gameData.multigame.msg.buf [bufP]), &gameData.multigame.robots.sendPending, MAX_ROBOTS_CONTROLLED * 4);
bufP += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&(gameData.multigame.msg.buf [bufP]), &gameData.multigame.robots.fired, MAX_ROBOTS_CONTROLLED * 4);
bufP += (MAX_ROBOTS_CONTROLLED * 4);
SET_MSG_ID
NetworkSendMineSyncPacket (gameData.multigame.msg.buf, 1, nPlayer);
}

//-----------------------------------------------------------------------------

void MultiDoRobotControls (uint8_t* buf)
{
	int32_t bufP = 1;

CHECK_MSG_ID

if (buf [bufP++] != N_LOCALPLAYER) {
	Int3 (); // Get Jason!  Recieved a coop_sync that wasn't ours!
	return;
	}
memcpy (&gameData.multigame.robots.controlled, buf + bufP, MAX_ROBOTS_CONTROLLED * 4);
bufP += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&gameData.multigame.robots.agitation, buf + bufP, MAX_ROBOTS_CONTROLLED * 4);
bufP += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&gameData.multigame.robots.controlledTime, buf + bufP, MAX_ROBOTS_CONTROLLED * 4);
bufP += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&gameData.multigame.robots.lastSendTime, buf + bufP, MAX_ROBOTS_CONTROLLED * 4);
bufP += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&gameData.multigame.robots.lastMsgTime, buf + bufP, MAX_ROBOTS_CONTROLLED * 4);
bufP += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&gameData.multigame.robots.sendPending, buf + bufP, MAX_ROBOTS_CONTROLLED * 4);
bufP += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&gameData.multigame.robots.fired, buf + bufP, MAX_ROBOTS_CONTROLLED * 4);
bufP += (MAX_ROBOTS_CONTROLLED * 4);
}

//-----------------------------------------------------------------------------

#define POWERUPADJUSTS 5
int32_t PowerupAdjustMapping [] = {11, 19, 39, 41, 44};

int32_t MultiPowerupIs4Pack (int32_t id)
{
	int32_t i;

for (i = 0; i < POWERUPADJUSTS; i++)
	if (id == PowerupAdjustMapping [i])
		return 1;
return 0;
}

//-----------------------------------------------------------------------------

int32_t MultiPowerupIsAllowed (int32_t nId)
{
#if 1
return powerupFilter [nId];
#else
switch (nId) {
	case POW_INVUL:
		if (!netGameInfo.m_info.DoInvulnerability)
			return (0);
		break;
	case POW_CLOAK:
		if (!netGameInfo.m_info.DoCloak)
			return (0);
		break;
	case POW_AFTERBURNER:
		if (!netGameInfo.m_info.DoAfterburner)
			return (0);
		break;
	case POW_FUSION:
		if (!netGameInfo.m_info.DoFusions)
			return (0);
		break;
	case POW_PHOENIX:
		if (!netGameInfo.m_info.DoPhoenix)
			return (0);
		break;
	case POW_HELIX:
		if (!netGameInfo.m_info.DoHelix)
			return (0);
		break;
	case POW_MEGAMSL:
		if (!netGameInfo.m_info.DoMegas)
			return (0);
		break;
	case POW_SMARTMSL:
		if (!netGameInfo.m_info.DoSmarts)
			return (0);
		break;
	case POW_GAUSS:
		if (!netGameInfo.m_info.DoGauss)
			return (0);
		break;
	case POW_VULCAN:
		if (!netGameInfo.m_info.DoVulcan)
			return (0);
		break;
	case POW_PLASMA:
		if (!netGameInfo.m_info.DoPlasma)
			return (0);
		break;
	case POW_OMEGA:
		if (!netGameInfo.m_info.DoOmega)
			return (0);
		break;
	case POW_SUPERLASER:
		if (!netGameInfo.m_info.DoSuperLaser)
			return (0);
		break;
	case POW_PROXMINE:
		if (!netGameInfo.m_info.DoProximity)
			return (0);
		break;
	case POW_VULCAN_AMMO:
		if (!(netGameInfo.m_info.DoVulcan || netGameInfo.m_info.DoGauss))
			return (0);
		break;
	case POW_SPREADFIRE:
		if (!netGameInfo.m_info.DoSpread)
			return (0);
		break;
	case POW_SMARTMINE:
		if (!netGameInfo.m_info.DoSmartMine)
			return (0);
		break;
	case POW_FLASHMSL_1:
		if (!netGameInfo.m_info.DoFlash)
			return (0);
		break;
	case POW_FLASHMSL_4:
		if (!netGameInfo.m_info.DoFlash)
			return (0);
		break;
	case POW_GUIDEDMSL_1:
		if (!netGameInfo.m_info.DoGuided)
			return (0);
		break;
	case POW_GUIDEDMSL_4:
		if (!netGameInfo.m_info.DoGuided)
			return (0);
		break;
	case POW_EARTHSHAKER:
		if (!netGameInfo.m_info.DoEarthShaker)
			return (0);
		break;
	case POW_MERCURYMSL_1:
		if (!netGameInfo.m_info.DoMercury)
			return (0);
		break;
	case POW_MERCURYMSL_4:
		if (!netGameInfo.m_info.DoMercury)
			return (0);
		break;
	case POW_CONVERTER:
		if (!netGameInfo.m_info.DoConverter)
			return (0);
		break;
	case POW_AMMORACK:
		if (!netGameInfo.m_info.DoAmmoRack)
			return (0);
		break;
	case POW_HEADLIGHT:
		if (!netGameInfo.m_info.DoHeadlight)
			return (0);
		break;
	case POW_LASER:
		if (!netGameInfo.m_info.DoLaserUpgrade)
			return (0);
		break;
	case POW_HOMINGMSL_1:
		if (!netGameInfo.m_info.DoHoming)
			return (0);
		break;
	case POW_HOMINGMSL_4:
		if (!netGameInfo.m_info.DoHoming)
			return (0);
		break;
	case POW_QUADLASER:
		if (!netGameInfo.m_info.DoQuadLasers)
			return (0);
		break;
	case POW_BLUEFLAG:
		if (!(gameData.app.GameMode (GM_CAPTURE)))
			return (0);
		break;
	case POW_REDFLAG:
		if (!(gameData.app.GameMode (GM_CAPTURE)))
			return (0);
		break;
	}
return 1;
#endif
}

//-----------------------------------------------------------------------------

void MultiSendFinishGame ()
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_FINISH_GAME;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = N_LOCALPLAYER;
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
}

//-----------------------------------------------------------------------------

extern void DoFinalBossHacks ();

void MultiDoFinishGame (uint8_t* buf)
{
if (buf [0] != MULTI_FINISH_GAME)
	return;

int32_t bufP = 1;

CHECK_MSG_ID

if (missionManager.nCurrentLevel != missionManager.nLastLevel)
	return;
DoFinalBossHacks ();
}

//-----------------------------------------------------------------------------

void MultiSendTriggerSpecific (uint8_t nPlayer, uint8_t trig)
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_START_TRIGGER;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = trig;
SET_MSG_ID
NetworkSendMineSyncPacket (gameData.multigame.msg.buf, bufP, nPlayer);
}

//-----------------------------------------------------------------------------

void MultiDoStartTrigger (uint8_t* buf)
{
	int32_t bufP = 1;

CHECK_MSG_ID

CTrigger* trigP = GEOTRIGGER ((int32_t) buf [bufP]);
if (trigP)
	trigP->m_info.flags |= TF_DISABLED;
}

//-----------------------------------------------------------------------------
// This function adds a kill to lifetime stats of this player, and possibly
// gives a promotion.  If so, it will tell everyone else

void MultiAddLifetimeKills (void)
{
if (!IsNetworkGame)
	return;
int32_t oldrank = GetMyNetRanking ();
++networkData.nNetLifeKills;
if (oldrank != GetMyNetRanking ()) {
	MultiSendRanking ();
	if (!gameOpts->multi.bNoRankings) {
		HUDInitMessage (TXT_PROMOTED, pszRankStrings [GetMyNetRanking ()]);
		audio.PlaySound (SOUND_BUDDY_MET_GOAL, SOUNDCLASS_GENERIC, I2X (2));
		NETPLAYER (N_LOCALPLAYER).rank = GetMyNetRanking ();
		}
	}
if (gameStates.multi.nGameType != UDP_GAME)
	SavePlayerProfile ();
}

//-----------------------------------------------------------------------------

void MultiAddLifetimeKilled (void)
{
	// This function adds a "nKilled" to lifetime stats of this player, and possibly
	// gives a demotion.  If so, it will tell everyone else

if (!IsNetworkGame)
	return;
int32_t oldrank = GetMyNetRanking ();
++networkData.nNetLifeKilled;
if (oldrank != GetMyNetRanking ()) {
	MultiSendRanking ();
	NETPLAYER (N_LOCALPLAYER).rank = GetMyNetRanking ();
	if (!gameOpts->multi.bNoRankings)
		HUDInitMessage (TXT_DEMOTED, pszRankStrings [GetMyNetRanking ()]);
	}
if (gameStates.multi.nGameType != UDP_GAME)
	SavePlayerProfile ();
}

//-----------------------------------------------------------------------------

void MultiSendRanking ()
{
	gameData.multigame.msg.buf [0] = MULTI_RANK;
	gameData.multigame.msg.buf [1] = (char)N_LOCALPLAYER;
	gameData.multigame.msg.buf [2] = (char)GetMyNetRanking ();

	MultiSendData (gameData.multigame.msg.buf, 3, 1);
}

//-----------------------------------------------------------------------------

void MultiDoRanking (uint8_t* buf)
{
	char rankstr [20];
	int32_t nPlayer = int32_t (buf [1]);
	char rank = buf [2];

	if (NETPLAYER (nPlayer).rank<rank)
		strcpy (rankstr, TXT_RANKUP);
	else if (NETPLAYER (nPlayer).rank>rank)
		strcpy (rankstr, TXT_RANKDOWN);
	else
		return;

	NETPLAYER (nPlayer).rank = rank;

	if (!gameOpts->multi.bNoRankings)
		HUDInitMessage (TXT_RANKCHANGE2, PLAYER (nPlayer).callsign,
								rankstr, pszRankStrings [(int32_t)rank]);
}

//-----------------------------------------------------------------------------

void MultiSendModemPing ()
{
gameData.multigame.msg.buf [0] = MULTI_MODEM_PING;
MultiSendData (gameData.multigame.msg.buf, 1, 1);
}

//-----------------------------------------------------------------------------

void MultiSendModemPingReturn (uint8_t* buf)
{
gameData.multigame.msg.buf [0] = MULTI_MODEM_PING_RETURN;
MultiSendData (gameData.multigame.msg.buf, 1, 1);
}

//-----------------------------------------------------------------------------

void  MultiDoModemPingReturn (uint8_t* buf)
{
if (pingStats [0].launchTime <= 0)
	return;
xPingReturnTime = TimerGetFixedSeconds ();
HUDInitMessage (TXT_PINGTIME, SDL_GetTicks () - pingStats [0].launchTime);
					 //X2I (FixMul (xPingReturnTime - pingStats [0].launchTime, I2X (1000))));
pingStats [0].launchTime = 0;
}

//-----------------------------------------------------------------------------

void MultiQuickSoundHack (int32_t nSound)
{
	int32_t			l, i;
	CSoundSample*	soundP = gameData.pig.sound.sounds [gameStates.sound.bD1Sound] + nSound;
	uint8_t*			dataP;

nSound = audio.XlatSound ((int16_t) nSound);
l = soundP->nLength [soundP->bCustom];
if (reversedSound.data [0].Create (l)) {
	reversedSound.nLength [0] = l;
	dataP = soundP->data [soundP->bCustom] + l;
	for (i = 0; i < l; i++)
		reversedSound.data [0][i] = *(--dataP);
	bSoundHacked = 1;
	}
}

//-----------------------------------------------------------------------------

void MultiSendPlayByPlay (int32_t msgType, int32_t killer, int32_t victim)
{
if (!IsHoardGame)
	return;
int32_t bufP = 0;
gameData.multigame.msg.buf [bufP++] = MULTI_PLAY_BY_PLAY;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = (uint8_t) msgType;
gameData.multigame.msg.buf [bufP++] = (uint8_t) killer;
gameData.multigame.msg.buf [bufP++] = (uint8_t) victim;
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
MultiDoPlayByPlay (gameData.multigame.msg.buf);
}

//-----------------------------------------------------------------------------

void MultiDoPlayByPlay (uint8_t* buf)
{
if (!IsHoardGame) 
	return;

	int32_t bufP = 1;

CHECK_MSG_ID

uint8_t msgType = buf [bufP++];
uint8_t killer = buf [bufP++];
uint8_t victim = buf [bufP++];

if (msgType == 0)
	HUDInitMessage (TXT_SMACKED, PLAYER (victim).callsign, PLAYER (killer).callsign);
else if (msgType == 1)
	HUDInitMessage (TXT_SPANKED, PLAYER (victim).callsign, PLAYER (killer).callsign);
}

//-----------------------------------------------------------------------------

void MultiDoReturnFlagHome (uint8_t* buf)
{
	int32_t bufP = 1;

CHECK_MSG_ID

	uint16_t		nType = buf [bufP++];
	uint16_t		id = buf [bufP++];
	CObject*		objP;

FORALL_POWERUP_OBJS (objP) {
	if ((objP->info.nType == nType) && (objP->info.nId == id)) {
		ReturnFlagHome (objP);
		break;
		}
	}
}

//-----------------------------------------------------------------------------

void MultiSendReturnFlagHome (int16_t nObject)
{
CObject* objP = OBJECT (nObject);
if (!objP)
	return;

int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_RETURN_FLAG;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = (char) objP->info.nType;
gameData.multigame.msg.buf [bufP++] = objP->info.nId;
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 0);
}

//-----------------------------------------------------------------------------

void MultiDoConquerWarning (uint8_t* buf)
{
audio.PlaySound (SOUND_CONTROL_CENTER_WARNING_SIREN, SOUNDCLASS_GENERIC, I2X (3));
}

//-----------------------------------------------------------------------------

void MultiSendConquerWarning (void)
{
gameData.multigame.msg.buf [0] = MULTI_CONQUER_WARNING;
gameData.multigame.msg.buf [1] = 0; // dummy values
gameData.multigame.msg.buf [2] = 0;
MultiSendData (gameData.multigame.msg.buf, 3, 0);
}

//-----------------------------------------------------------------------------

void MultiDoStopConquerWarning (uint8_t* buf)
{
audio.StopSound (SOUND_CONTROL_CENTER_WARNING_SIREN);
}

//-----------------------------------------------------------------------------

void MultiSendStopConquerWarning (void)
{
gameData.multigame.msg.buf [0] = MULTI_STOP_CONQUER_WARNING;
gameData.multigame.msg.buf [1] = 0; // dummy values
gameData.multigame.msg.buf [2] = 0;
MultiSendData (gameData.multigame.msg.buf, 3, 0);
}

//-----------------------------------------------------------------------------

void MultiDoConquerRoom (uint8_t* buf)
{
	int32_t bufP = 1;

CHECK_MSG_ID

ConquerRoom (buf [bufP], buf [bufP + 1], buf [bufP + 2]);
}

//-----------------------------------------------------------------------------

void MultiSendConquerRoom (uint8_t nOwner, uint8_t prevOwner, uint8_t group)
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_CONQUER_ROOM;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = nOwner;
gameData.multigame.msg.buf [bufP++] = prevOwner;
gameData.multigame.msg.buf [bufP++] = group;
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 0);
}

//-----------------------------------------------------------------------------

void MultiSendTeleport (uint8_t nPlayer, int16_t nSegment, uint8_t nSide)
{
	int32_t bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_TELEPORT;
ADD_MSG_ID
gameData.multigame.msg.buf [bufP++] = nPlayer; // dummy values
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, nSegment);
bufP += 2;
gameData.multigame.msg.buf [bufP] = nSide; // dummy values
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, bufP, 0);
}

//-----------------------------------------------------------------------------

void MultiDoTeleport (uint8_t* buf)
{
	int32_t bufP = 1;

CHECK_MSG_ID

	uint8_t nPlayer = buf [bufP++];
	int16_t	nObject = PLAYER (int32_t (nPlayer)).nObject;
	int16_t nSegment = GET_INTEL_SHORT (buf + bufP);
//	int16_t	nSide = buf [bufP];

TriggerSetObjPos (nObject, nSegment);
gameData.multiplayer.bTeleport [nPlayer] = 1;
if (OBJECT (nObject))
	OBJECT (nObject)->CreateAppearanceEffect ();
}

//-----------------------------------------------------------------------------

void MultiSendConfirmMessage (int32_t nId)
{
gameData.multigame.msg.buf [0] = PID_CONFIRM_MESSAGE;
gameData.multigame.msg.buf [1] = N_LOCALPLAYER; 
PUT_INTEL_INT (gameData.multigame.msg.buf + 2, nId);
IpxSendPlayerPacket (importantMessages [1].GetSender (nId), gameData.multigame.msg.buf, 6);
}

//-----------------------------------------------------------------------------

void MultiDoConfirmMessage (uint8_t* buf)
{
uint8_t nPlayer = gameData.multigame.msg.buf [1];
int32_t nId = GET_INTEL_INT (buf + 2);
importantMessages [0].Acknowledge (nPlayer, nId);
}

//-----------------------------------------------------------------------------
// PSALM - Powerup Spam And Loss Minimizer
// Compare allowed number of each powerup type with number of powerups of that type in the mine and on ships
// Add/remove missing/excess powerups

extern int32_t nDbgPowerup;

void MultiAdjustPowerups (void)
{
	static	time_t	t0 = 0;
	time_t	t = gameStates.app.nSDLTicks [0];

if (gameData.multiplayer.WaitingForExplosion () || gameData.multiplayer.WaitingForWeaponInfo ()) { // don't act if player ship status pending
	if (t - t0 < 180000) 		// enforce after 3 minutes of inactivity though
		return;
#if DBG
	t0 = t;
#endif
	}

	int32_t h, i, j;

if (gameStates.multi.nGameType != UDP_GAME)
	return;
if (t - t0 < 1000)
	return;
t0 = t;
for (i = 0; i < MAX_POWERUP_TYPES; i++) {
	if (MultiPowerupIs4Pack (i))
		continue;
	h = MissingPowerups (i, 1);
#if DBG
	if (i == nDbgPowerup)
		BRP;
#endif
	if (h < 0) {
		if (gameData.multiplayer.powerupsInMine [i] > 0) 
			j = i;
		else {
			j = i + 1;
			if ((gameData.multiplayer.powerupsInMine [j] <= 0) || (h > -4) || !MultiPowerupIs4Pack (j))
				continue;
			}
	#if DBG
		if (j == nDbgPowerup)
			BRP;
		MissingPowerups (i, 1);
#endif
		CObject* objP, * delObjP = NULL;
		int32_t tCreate = -0x7FFFFFFF;

		FORALL_STATIC_OBJS (objP) {
			if ((objP->Id () == j) && (tCreate < objP->CreationTime ())) {
				tCreate = objP->CreationTime ();
				delObjP = objP;
				}
			}
		if (delObjP) {
			delObjP->Die ();
			//MultiSendRemoveObject (OBJ_IDX (oldestObjP));
			}
		}
	else if (h > 0) {
		if ((i == POW_ENERGY) || (i == POW_SHIELD_BOOST))
			continue;
	#if DBG
		if (i == nDbgPowerup)
			BRP;
		MissingPowerups (i, 1);
	#endif
		if (MultiPowerupIs4Pack (i + 1) && (MissingPowerups (i + 1) > 0)) {
			for (j = h / 4; j; j--)
				MaybeDropNetPowerup (-1, i + 1, FORCE_DROP);
			h %= 4;
			i++;
			}
		else 
			{
			for (j = h; j; j--)
				MaybeDropNetPowerup (-1, i, FORCE_DROP);
			}
		}
	}
}

//-----------------------------------------------------------------------------

tMultiHandlerInfo multiHandlers [MULTI_MAX_TYPE] = {
	{MultiDoPosition, 1},
	{MultiDoReappear, 1},
	{MultiDoFire, 1},
	{MultiDoKill, 0},
	{MultiDoRemoveObject, 1},
	{MultiDoPlayerExplode, 1},
	{MultiDoMsg, 1},
	{MultiDoQuit, 1},
	{MultiDoPlaySound, 1},
	{NULL, 1},
	{MultiDoDestroyReactor, 1},
	{MultiDoClaimRobot, 1},
	{NULL, 1},
	{MultiDoCloak, 1},
	{MultiDoEscape, 1},
	{MultiDoDoorOpen, 1},
	{MultiDoCreateExplosion, 1},
	{MultiDoCtrlcenFire, 1},
	{MultiDoPlayerExplode, 1},
	{(pMultiHandler) MultiDoCreatePowerup, 1},
	{NULL, 1},
	{MultiDoDeCloak, 1},
	{NULL, 1},
	{MultiDoRobotPosition, 1},
	{MultiDoRobotExplode, 1},
	{MultiDoReleaseRobot, 1},
	{MultiDoRobotFire, 1},
	{MultiDoScore, 1},
	{MultiDoCreateRobot, 1},
	{MultiDoTrigger, 1},
	{MultiDoBossActions, 1},
	{MultiDoCreateRobotPowerups, 1},
	{MultiDoHostageDoorStatus, 1},

	{MultiDoSaveGame, 1},
	{MultiDoRestoreGame, 1},

	{MultiDoReqPlayer, 1},
	{MultiDoSendPlayer, 1},
	{MultiDoDropMarker, 1},
	{MultiDoDropWeapon, 1},
	{MultiDoGuided, 1},
	{MultiDoStolenItems, 1},
	{MultiDoWallStatus, 1},
	{MultiDoHeartBeat, 1},
	{MultiDoScoreGoalCounts, 1},
	{MultiDoSeismic, 1},
	{MultiDoLight, 1},
	{MultiDoStartTrigger, 1},
	{MultiDoFlags, 1},
	{MultiDoDropBlob, 1},
	{MultiDoPowerupUpdate, 1},
	{MultiDoActiveDoor, 1},
	{MultiDoSoundFunction, 1},
	{MultiDoCaptureBonus, 1},
	{MultiDoGotFlag, 1},
	{MultiDoDropFlag, 1},
	{MultiDoRobotControls, 1},
	{MultiDoFinishGame, 1},
	{MultiDoRanking, 1},
	{MultiSendModemPingReturn, 1},
	{MultiDoModemPingReturn, 1},
	{MultiDoOrbBonus, 1},
	{MultiDoGotOrb, 1},
	{NULL, 1},
	{MultiDoPlayByPlay, 1},
	{MultiDoReturnFlagHome, 1},
	{MultiDoConquerRoom, 1},
	{MultiDoConquerWarning, 1},
	{MultiDoStopConquerWarning, 1},
	{MultiDoTeleport, 1},
	{MultiDoSetTeam, 1},
	{MultiDoStartTyping, 1},
	{MultiDoQuitTyping, 1},
	{MultiDoObjTrigger, 1},
	{MultiDoShield, 1},
	{MultiDoInvul, 1},
	{MultiDoDeInvul, 1},
	{MultiDoWeapons, 1},
	{MultiDoMonsterball, 1},
	{MultiDoCheating, 1},
	{MultiDoSyncKills, 1},
	{MultiDoCountdown, 1},
	{MultiDoWeaponStates, 1},
	{MultiDoSyncMonsterball, 1},
	{MultiDoDropPowerup, 1},
	{MultiDoCreateWeapon, 1},
	{MultiDoAmmo, 1},
	{MultiDoFusionCharge, 1},
	{MultiDoPlayerThrust, 1}
	};

//-----------------------------------------------------------------------------

int MultiProcessData (uint8_t* buf, int32_t len)
{
	// Take an entire message (that has already been checked for validity,
	// if necessary) and act on it.

	uint8_t nType = buf [0];

if (nType > MULTI_MAX_TYPE) {
	Int3 ();
	return 0;
	}

console.printf (CON_VERBOSE, "multi data %d\n", nType);
#if !DBG
if (nType <= MULTI_MAX_TYPE) {
	tMultiHandlerInfo	*pmh = multiHandlers + nType;
	if (pmh->fpMultiHandler && !(gameStates.app.bEndLevelSequence && pmh->noEndLevelSeq))
		pmh->fpMultiHandler (buf);
	}
#else //DBG
switch (nType) {
	case MULTI_POSITION:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoPosition (buf);
		break;
	case MULTI_REAPPEAR:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoReappear (buf);
		break;
	case MULTI_FIRE:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoFire (buf);
		break;
	case MULTI_KILL:
			MultiDoKill (buf);
		break;
	case MULTI_REMOVE_OBJECT:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoRemoveObject (buf);
		break;
	case MULTI_PLAYER_DROP:
	case MULTI_PLAYER_EXPLODE:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoPlayerExplode (buf);
		break;
	case MULTI_MESSAGE:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoMsg (buf);
		break;
	case MULTI_QUIT:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoQuit (buf);
		break;
	case MULTI_BEGIN_SYNC:
		break;
	case MULTI_DESTROY_REACTOR:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoDestroyReactor (buf);
		break;
	case MULTI_POWERUP_UPDATE:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoPowerupUpdate (buf);
		break;
	case MULTI_SOUND_FUNCTION:
		MultiDoSoundFunction (buf);
		break;
	case MULTI_MARKER:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoDropMarker (buf);
		break;
	case MULTI_DROP_WEAPON:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoDropWeapon (buf);
		break;
	case MULTI_DROP_FLAG:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoDropFlag (buf);
		break;
	case MULTI_GUIDED:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoGuided (buf);
		break;
	case MULTI_STOLEN_ITEMS:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoStolenItems (buf);
		break;
	case MULTI_WALL_STATUS:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoWallStatus (buf);
		break;
	case MULTI_HEARTBEAT:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoHeartBeat (buf);
		break;
	case MULTI_SEISMIC:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoSeismic (buf);
		break;
	case MULTI_LIGHT:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoLight (buf);
		break;
	case MULTI_SCOREGOALS:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoScoreGoalCounts (buf);
		break;
	case MULTI_ENDLEVEL_START:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoEscape (buf);
		break;
	case MULTI_END_SYNC:
		break;
	case MULTI_INVUL:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoInvul (buf);
		break;
	case MULTI_DEINVUL:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoDeInvul (buf);
		break;
	case MULTI_CLOAK:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoCloak (buf);
		break;
	case MULTI_DECLOAK:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoDeCloak (buf);
		break;
	case MULTI_DOOR_OPEN:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoDoorOpen (buf);
		break;
	case MULTI_CREATE_EXPLOSION:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoCreateExplosion (buf);
		break;
	case MULTI_CONTROLCEN_FIRE:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoCtrlcenFire (buf);
		break;
	case MULTI_CREATE_POWERUP:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoCreatePowerup (buf);
		break;
	case MULTI_PLAY_SOUND:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoPlaySound (buf);
		break;
	case MULTI_CAPTURE_BONUS:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoCaptureBonus (buf);
		break;
	case MULTI_ORB_BONUS:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoOrbBonus (buf);
		break;
	case MULTI_GOT_FLAG:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoGotFlag (buf);
		break;
	case MULTI_GOT_ORB:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoGotOrb (buf);
		break;
	case MULTI_PLAY_BY_PLAY:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoPlayByPlay (buf);
		break;
	case MULTI_RANK:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoRanking (buf);
		break;
	case MULTI_MODEM_PING:
		if (!gameStates.app.bEndLevelSequence)
			MultiSendModemPingReturn (buf);
		break;
	case MULTI_MODEM_PING_RETURN:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoModemPingReturn (buf);
		break;
	case MULTI_FINISH_GAME:
		MultiDoFinishGame (buf);
		break;  // do this one regardless of endsequence
	case MULTI_ROBOT_CONTROLS:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoRobotControls (buf);
		break;
	case MULTI_ROBOT_CLAIM:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoClaimRobot (buf);
		break;
	case MULTI_ROBOT_POSITION:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoRobotPosition (buf);
		break;
	case MULTI_ROBOT_EXPLODE:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoRobotExplode (buf);
		break;
	case MULTI_ROBOT_RELEASE:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoReleaseRobot (buf);
		break;
	case MULTI_ROBOT_FIRE:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoRobotFire (buf);
		break;
	case MULTI_SCORE:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoScore (buf);
		break;
	case MULTI_CREATE_ROBOT:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoCreateRobot (buf);
		break;
	case MULTI_TRIGGER:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoTrigger (buf);
		break;
	case MULTI_OBJ_TRIGGER:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoObjTrigger (buf);
		break;
	case MULTI_START_TRIGGER:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoStartTrigger (buf);
		break;
	case MULTI_FLAGS:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoFlags (buf);
		break;
	case MULTI_WEAPONS:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoWeapons (buf);
		break;
	case MULTI_MONSTERBALL:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoMonsterball (buf);
		break;
	case MULTI_DROP_BLOB:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoDropBlob (buf);
		break;
	case MULTI_ACTIVE_DOOR:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoActiveDoor (buf);
		break;
	case MULTI_BOSS_ACTIONS:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoBossActions (buf);
		break;
	case MULTI_CREATE_ROBOT_POWERUPS:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoCreateRobotPowerups (buf);
		break;
	case MULTI_HOSTAGE_DOOR:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoHostageDoorStatus (buf);
		break;
	case MULTI_SAVE_GAME:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoSaveGame (buf);
		break;
	case MULTI_RESTORE_GAME:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoRestoreGame (buf);
		break;
	case MULTI_REQ_PLAYER:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoReqPlayer (buf);
		break;
	case MULTI_SEND_PLAYER:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoSendPlayer (buf);
		break;
	case MULTI_RETURN_FLAG:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoReturnFlagHome (buf);
		break;
	case MULTI_CONQUER_ROOM:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoConquerRoom (buf);
		break;
	case MULTI_CONQUER_WARNING:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoConquerWarning (buf);
		break;
	case MULTI_STOP_CONQUER_WARNING:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoStopConquerWarning (buf);
		break;
	case MULTI_TELEPORT:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoTeleport (buf);
		break;
	case MULTI_SET_TEAM:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoSetTeam (buf);
		break;
	case MULTI_START_TYPING:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoStartTyping (buf);
		break;
	case MULTI_QUIT_TYPING:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoQuitTyping (buf);
		break;
	case MULTI_PLAYER_SHIELDS:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoShield (buf);
		break;
	case MULTI_CHEATING:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoCheating (buf);
		break;
	case MULTI_SYNC_KILLS:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoSyncKills (buf);
		break;
	case MULTI_COUNTDOWN:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoCountdown (buf);
		break;
	case MULTI_PLAYER_WEAPONS:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoWeaponStates (buf);
		break;
	case MULTI_SYNC_MONSTERBALL:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoSyncMonsterball (buf);
		break;
	case MULTI_DROP_POWERUP:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoDropPowerup (buf);
		break;
	case MULTI_CREATE_WEAPON:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoCreateWeapon (buf);
		break;
	case MULTI_AMMO:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoAmmo (buf);
		break;
	case MULTI_FUSION_CHARGE:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoFusionCharge (buf);
		break;
	case MULTI_PLAYER_THRUST:
		if (!gameStates.app.bEndLevelSequence)
			MultiDoPlayerThrust (buf);
		break;
	default:
		return 0;
	}
#endif //DBG
return 1;
}

//-----------------------------------------------------------------------------
//eof
