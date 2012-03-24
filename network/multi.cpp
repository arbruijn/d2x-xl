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

typedef void tMultiHandler (char *);
typedef tMultiHandler *pMultiHandler;
typedef struct tMultiHandlerInfo {
	pMultiHandler	fpMultiHandler;
	char				noEndLevelSeq;
} tMultiHandlerInfo;

void MultiResetPlayerObject (CObject *objP);
void MultiSetObjectTextures (CObject *objP);
void MultiAddLifetimeKilled (void);
void MultiAddLifetimeKills (void);
void MultiSendPlayByPlay (int num, int spnum, int dpnum);
void MultiSendHeartBeat (void);
void MultiCapObjects (void);
void MultiSaveGame (char slot, uint id, char *desc);
void MultiRestoreGame (char slot, uint id);
void MultiSetRobotAI (void);
void MultiSendPowerupUpdate (void);
void FreeHoardData (void);
void InitHoardData (void);
void MultiApplyGoalTextures (void);
void MultiBadRestore (void);
void MultiDoCaptureBonus (char *buf);
void MultiDoOrbBonus (char *buf);
void MultiSendDropFlag (int nObject, int seed);
void MultiSendRanking (void);
void MultiDoPlayByPlay (char *buf);
void MultiDoConquerRoom (char *buf);
void MultiDoConquerWarning (char *buf);
void MultiAdjustPowerupCap (void);

//
// Local macros and prototypes
//

// LOCALIZE ME!!

void DropPlayerEggs (CObject *player); // from collide.c

//
// Global variaszPlayers
//

extern void SetFunctionMode (int);

CNetGameInfo netGame;

extern CAllNetPlayersInfo netPlayers [2];

tBitmapIndex mpTextureIndex [MAX_PLAYERS][N_PLAYER_SHIP_TEXTURES];

typedef struct tNetPlayerStats {
	ubyte  messageType;
	ubyte  nLocalPlayer;          // Who am i?
	uint   flags;                 // Powerup flags, see below...
	fix    energy;                // Amount of energy remaining.
	fix    shield;                // shield remaining (protection)
	ubyte  lives;                 // Lives remaining, 0 = game over.
	ubyte  laserLevel;				// Current level of the laser.
	ubyte  primaryWeaponFlags;		// bit set indicates the player has this weapon.
	ubyte  secondaryWeaponFlags;  // bit set indicates the player has this weapon.
	ushort primaryAmmo [MAX_PRIMARY_WEAPONS];     // How much ammo of each nType.
	ushort secondaryAmmo [MAX_SECONDARY_WEAPONS]; // How much ammo of each nType.
	int    lastScore;             // Score at beginning of current level.
	int    score;                 // Current score.
	fix    cloakTime;             // Time cloaked
	fix    invulnerableTime;      // Time invulnerable
	fix    homingObjectDist;      // Distance of nearest homing CObject.
	short  nScoreGoalCount;
	short  netKilledTotal;        // Number of times nKilled total
	short  netKillsTotal;         // Number of net kills total
	short  numKillsLevel;         // Number of kills this level
	short  numKillsTotal;         // Number of kills total
	short  numRobotsLevel;        // Number of initial robots this level
	short  numRobotsTotal;        // Number of robots total
	ushort nHostagesRescued;		// Total number of hostages rescued.
	ushort nHostagesTotal;        // Total number of hostages.
	ubyte  nHostagesOnBoard;      // Number of hostages on ship.
	ubyte  unused [16];
} tNetPlayerStats;

static int multiMessageLengths [MULTI_MAX_TYPE+1][2] = {
	{24, -1}, // POSITION
	{3, -1},  // REAPPEAR
	{8, -1}, //9 + MAX_FIRED_OBJECTS * sizeof (short)},  // FIRE
	{5, -1},  // KILL
	{4, 5},  // REMOVE_OBJECT
	{97+9, 97+13}, // PLAYER_EXPLODE
	{37, -1}, // MESSAGE (MAX_MESSAGE_LENGTH = 40)
	{2, -1},  // QUIT
	{4, -1},  // PLAY_SOUND
	{41, -1}, // BEGIN_SYNC
	{4, -1},  // CONTROLCEN
	{5, -1},  // CLAIM ROBOT
	{4, -1},  // END_SYNC
	{2, -1},  // CLOAK
	{3, -1},  // ENDLEVEL_START
	{5, -1},  // DOOR_OPEN
	{2, -1},  // CREATE_EXPLOSION
	{16, -1}, // CONTROLCEN_FIRE
	{97+9, 97+11}, // PLAYER_DROP
	{19, -1}, // CREATE_POWERUP
	{9, -1},  // MISSILE_TRACK
	{2, -1},  // DE-CLOAK
	{2, -1},  // MENU_CHOICE
	{28, -1}, // ROBOT_POSITION  (shortpos_length (23) + 5 = 28)
	{9, -1},  // ROBOT_EXPLODE
	{5, -1},  // ROBOT_RELEASE
	{18, -1}, // ROBOT_FIRE
	{6, -1},  // SCORE
	{6, -1},  // CREATE_ROBOT
	{3, -1},  // TRIGGER
	{10, -1}, // BOSS_ACTIONS
	{27, 29}, // ROBOT_POWERUPS
	{7, -1},  // HOSTAGE_DOOR
	{2+24, -1}, // SAVE_GAME      (ubyte slot, -1}, uint id, -1}, char name [20])
	{2+4, -1},  // RESTORE_GAME   (ubyte slot, -1}, uint id)
	{1+1, -1},  // MULTI_REQ_PLAYER
	{sizeof (tNetPlayerStats), -1}, // MULTI_SEND_PLAYER
	{55, -1}, // MULTI_MARKER
	{12, -1}, // MULTI_DROP_WEAPON
	{3+sizeof (tShortPos), -1}, // MULTI_GUIDED
	{11, -1}, // MULTI_STOLEN_ITEMS
	{6, -1},  // MULTI_WALL_STATUS
	{5, -1},  // MULTI_HEARTBEAT
	{9, 17},  // MULTI_SCOREGOALS
	{9, -1},  // MULTI_SEISMIC
	{18, -1}, // MULTI_LIGHT
	{2, -1},  // MULTI_START_TRIGGER
	{6, -1},  // MULTI_FLAGS
	{2, -1},  // MULTI_DROP_BLOB
	{/*2 **/ MAX_POWERUP_TYPES + 1, -1}, // MULTI_POWERUP_UPDATE
	{sizeof (CActiveDoor) + 3, -1}, // MULTI_ACTIVE_DOOR
	{4, -1},  // MULTI_SOUND_FUNCTION
	{2, -1},  // MULTI_CAPTURE_BONUS
	{2, -1},  // MULTI_GOT_FLAG
	{12, -1}, // MULTI_DROP_FLAG
	{142, -1}, // MULTI_ROBOT_CONTROLS
	{2, -1},  // MULTI_FINISH_GAME
	{3, -1},  // MULTI_RANK
	{1, -1},  // MULTI_MODEM_PING
	{1, -1},  // MULTI_MODEM_PING_RETURN
	{3, -1},  // MULTI_ORB_BONUS
	{2, -1},  // MULTI_GOT_ORB
	{12, -1}, // MULTI_DROP_ORB
	{4, -1},  // MULTI_PLAY_BY_PLAY
	{3, -1},	 // MULTI_RETURN_FLAG
	{4, -1},	 // MULTI_CONQUER_ROOM
	{3, -1},  // MULTI_CONQUER_WARNING
	{3, -1},  // MULTI_STOP_CONQUER_WARNING
	{5, -1},  // MULTI_TELEPORT
	{4, -1},  // MULTI_SET_TEAM
	{3, -1},  // MULTI_MESSAGE_START
	{3, -1},  // MULTI_MESSAGE_QUIT
	{3, -1},	 // MULTI_OBJECT_TRIGGER
	{6, -1},	 // MULTI_PLAYER_SHIELDS, -1},
	{2, -1},	 // MULTI_INVUL
	{2, -1},	 // MULTI_DEINVUL
	{33, -1}, // MULTI_WEAPONS
	{40, -1}, // MULTI_MONSTERBALL
	{2, -1},  // MULTI_CHEATING
	{5, -1},  // MULTI_TRIGGER_EXT
	{16, -1}, // MULTI_SYNC_KILLS
	{5, -1},	 // MULTI_COUNTDOWN
	{29, -1}, // MULTI_PLAYER_WEAPONS
	{99, -1}, // MULTI_SYNC_MONSTERBALL
	{31, -1}, // MULTI_DROP_POWERUP
	{31, -1}, // MULTI_CREATE_WEAPON
	{6, -1},  // MULTI_AMMO
	{6, -1},  // MULTI_FUSION_CHARGE
	{7, -1}   // MULTI_PLAYER_THRUST
};

void ExtractNetPlayerStats (tNetPlayerStats *ps, CPlayerInfo * pd);
void UseNetPlayerStats (CPlayerInfo * ps, tNetPlayerStats *pd);

//-----------------------------------------------------------------------------

inline int MultiMsgLen (int nMsg)
{
#if 0
return multiMessageLengths [nMsg][0];
#else
	int l = multiMessageLengths [nMsg][gameStates.multi.nGameType == UDP_GAME];

return (l > 0) ? l : multiMessageLengths [nMsg][0];
#endif
}

//-----------------------------------------------------------------------------

ushort tNetGameInfoLite::GetTeamVector (void) 
{ 
return ((gameStates.multi.nGameType == UDP_GAME) && !IsCoopGame) ? ushort (nLevel >> 16) : ushort (teamVector); 
}

//-----------------------------------------------------------------------------

void tNetGameInfoLite::SetTeamVector (ushort n) 
{ 
if ((gameStates.multi.nGameType == UDP_GAME) && !IsCoopGame) 
	nLevel = (nLevel & 0xFFFF) | (int (n) << 16); 
else 
	teamVector = ubyte (n); 
}

//-----------------------------------------------------------------------------
// check protection
// return 0 if player cheats and cheat cannot be neutralized
// currently doesn't do anything besides setting player ship physics to default values.

int MultiProtectGame (void)
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

int ObjnumRemoteToLocal (int nRemoteObj, int nOwner)
{
	int nObject;

if ((nOwner >= gameData.multiplayer.nPlayers) || (nOwner < -1)) {
	Int3 (); // Illegal!
	return nRemoteObj;
	}
if (nOwner == -1)
	return nRemoteObj;
if ((nRemoteObj < 0) || (nRemoteObj >= LEVEL_OBJECTS))
	return -1;
nObject = gameData.multigame.remoteToLocal [nOwner * LEVEL_OBJECTS + nRemoteObj];
if (nObject < 0)
	return -1;
return nObject;
}

//-----------------------------------------------------------------------------
// Map a local CObject number to a remote + nOwner

int ObjnumLocalToRemote (int nLocalObj, sbyte *nOwner)
{
if ((nLocalObj < 0) || (nLocalObj > gameData.objs.nLastObject [0])) {
	*nOwner = -1;
	return -1;
	}
*nOwner = gameData.multigame.nObjOwner [nLocalObj];
if (*nOwner == -1)
	return nLocalObj;
if ((*nOwner >= gameData.multiplayer.nPlayers) || (*nOwner < -1)) {
	Int3 (); // Illegal!
	*nOwner = -1;
	return nLocalObj;
	}
return gameData.multigame.localToRemote [nLocalObj];
}

//-----------------------------------------------------------------------------

void RemapLocalPlayerObject (int nLocalObj, int nRemoteObj)
{
if (nLocalObj != nRemoteObj)
	for (int i = 0; i < gameData.multiplayer.nPlayerPositions; i++)
		if (gameData.multiplayer.players [i].nObject == nRemoteObj) {
			gameData.multiplayer.players [i].SetObject (nLocalObj);
			if (i == N_LOCALPLAYER)
				gameData.objs.consoleP = OBJECTS + nLocalObj;
			break;
			}
}

//-----------------------------------------------------------------------------
// Add a mapping from a network remote object number to a local one

void MapObjnumLocalToRemote (int nLocalObj, int nRemoteObj, int nOwner)
{
if ((nLocalObj > -1) && (nLocalObj < LEVEL_OBJECTS) && 
	 (nRemoteObj > -1) && (nRemoteObj < LEVEL_OBJECTS) && 
	 (nOwner > -1) && (nOwner != N_LOCALPLAYER)) {
	gameData.multigame.nObjOwner [nLocalObj] = nOwner;
	gameData.multigame.remoteToLocal [nOwner * LEVEL_OBJECTS + nRemoteObj] = nLocalObj;
	gameData.multigame.localToRemote [nLocalObj] = nRemoteObj;
	}
}

//-----------------------------------------------------------------------------
// Add a mapping for our locally created OBJECTS

void MapObjnumLocalToLocal (int nLocalObj)
{
if ((nLocalObj > -1) && (nLocalObj < LEVEL_OBJECTS)) {
	gameData.multigame.nObjOwner [nLocalObj] = N_LOCALPLAYER;
	gameData.multigame.remoteToLocal [N_LOCALPLAYER * LEVEL_OBJECTS + nLocalObj] = nLocalObj;
	gameData.multigame.localToRemote [nLocalObj] = nLocalObj;
	}
}

//-----------------------------------------------------------------------------

void ResetNetworkObjects (void)
{
gameData.multigame.localToRemote.Clear (0xff);
gameData.multigame.remoteToLocal.Clear (0xff);
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
	int		i;

for (i = 0; i < MAX_PLAYERS; i++, playerP++)
	gameData.render.thrusters [i].path.Reset (10, 1);
}

// -----------------------------------------------------------------------------

void SetPlayerPaths (void)
{
	CPlayerData	*playerP = gameData.multiplayer.players;
	int		i;

for (i = 0; i < gameData.multiplayer.nPlayers; i++, playerP++)
	gameData.render.thrusters [i].path.SetPoint (OBJECTS + playerP->nObject);
}

// -----------------------------------------------------------------------------

static inline int BonusScore (void)
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

static inline int ScoreGoal (bool bNone = false)
{
return bNone ? 0 : netGame.GetScoreGoal () ? netGame.GetScoreGoal () * BonusScore () : 0x7fffffff;
}

// -----------------------------------------------------------------------------

static void CheckScoreGoal (int nPlayer, bool bForce = false)
{
if (gameData.multiplayer.players [nPlayer].nScoreGoalCount >= ScoreGoal (bForce)) {
	if (nPlayer != N_LOCALPLAYER) 
		HUDInitMessage (TXT_REACH_SCOREGOAL2, gameData.multiplayer.players [nPlayer].callsign);
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
for (int i = 0; i < gameData.multiplayer.nPlayers; i++, playerP++)
	if (playerP->flags & PLAYER_FLAGS_FLAG)
		gameData.pig.flags [!GetTeam (i)].path.SetPoint (OBJECTS + playerP->nObject);
#else
SetPathPoint (&gameData.pig.flags [0].path, OBJECTS + playerP->nObject);
#endif
}

//-----------------------------------------------------------------------------
// Show a score list to end of net players
// Save connect state and change to new connect state

void MultiEndLevelScore (void)
{
	int oldConnect = 0;
	int i;

if (IsNetworkGame) {
	oldConnect = LOCALPLAYER.Connected () ? LOCALPLAYER.connected : 0;
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
		gameData.multiplayer.players [i].flags &= ~(PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY);
	}
for (i = 0; i < gameData.multiplayer.nMaxPlayers; i++)
	gameData.multiplayer.players [i].flags &= ~(PLAYER_FLAGS_FLAG);  // Clear capture flag

for (i = 0; i < MAX_PLAYERS; i++)
	gameData.multiplayer.players [i].nScoreGoalCount = 0;
gameData.multiplayer.maxPowerupsAllowed.Clear (0);
gameData.multiplayer.powerupsInMine.Clear (0);
}

//-----------------------------------------------------------------------------

short GetTeam (int nPlayer)
{
return (netGame.m_info.GetTeamVector () & (1 << nPlayer)) ? 1 : 0;
}

//-----------------------------------------------------------------------------

bool SameTeam (int nPlayer1, int nPlayer2)
{
if (IsCoopGame)
	return true;
if (!IsTeamGame)
	return false;
return GetTeam (nPlayer1) == GetTeam (nPlayer2);
}

//-----------------------------------------------------------------------------

void MultiSendPlayerWeapons (void)
{
	CWeaponState	*wsP = gameData.multiplayer.weaponStates + N_LOCALPLAYER;

gameData.multigame.msg.buf [0] = char (MULTI_PLAYER_WEAPONS);
gameData.multigame.msg.buf [1] = char (N_LOCALPLAYER);
gameData.multigame.msg.buf [2] = wsP->nPrimary;
gameData.multigame.msg.buf [3] = wsP->nSecondary;
gameData.multigame.msg.buf [4] = wsP->nMissiles;
gameData.multigame.msg.buf [5] = wsP->nLaserLevel;
gameData.multigame.msg.buf [6] = wsP->bQuadLasers;
PUT_INTEL_INT (gameData.multigame.msg.buf + 7, wsP->firing [0].nDuration);
PUT_INTEL_INT (gameData.multigame.msg.buf + 11, wsP->firing [1].nDuration);
gameData.multigame.msg.buf [15] = (ubyte) wsP->firing [0].bSpeedUp;
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
	CWeaponState	*wsP = gameData.multiplayer.weaponStates + N_LOCALPLAYER;

gameData.multigame.msg.buf [0] = char (MULTI_PLAYER_THRUST);
gameData.multigame.msg.buf [1] = char (N_LOCALPLAYER);
memcpy (gameData.multigame.msg.buf + 2, wsP->nThrusters, sizeof (wsP->nThrusters));
MultiSendData (gameData.multigame.msg.buf, 2 + sizeof (wsP->nThrusters), 0);
}

//-----------------------------------------------------------------------------

void MultiDoPlayerWeapons (char *buf)
{
	CWeaponState	*wsP = gameData.multiplayer.weaponStates + int (buf [1]);
	tFiringData		*fP;
	int				i;

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

void MultiDoPlayerThrust (char *buf)
{
	CWeaponState	*wsP = gameData.multiplayer.weaponStates + int (buf [1]);

memcpy (wsP->nThrusters, buf + 2, sizeof (wsP->nThrusters));
}

//-----------------------------------------------------------------------------

void MultiSendSetTeam (int nPlayer)
{
gameData.multigame.msg.buf [0] = char (MULTI_SET_TEAM);
gameData.multigame.msg.buf [1] = char (nPlayer);
gameData.multigame.msg.buf [2] = char (GetTeam (nPlayer));
gameData.multigame.msg.buf [3] = 0;
MultiSendData (gameData.multigame.msg.buf, 4, 0);
}

//-----------------------------------------------------------------------------

static const char *szTeamColors [2] = {"blue", "red"};

void SetTeam (int nPlayer, int team)
{
for (int i = 0; i < gameData.multiplayer.nPlayers; i++)
	if (gameData.multiplayer.players [i].Connected ())
		MultiSetObjectTextures (OBJECTS + gameData.multiplayer.players [i].nObject);

if (team >= 0) {
	if (team)
		netGame.m_info.AddTeamPlayer (nPlayer);
	else
		netGame.m_info.RemoveTeamPlayer (nPlayer);
	}
else {
	team = !GetTeam (nPlayer);
	if (team)
		netGame.m_info.AddTeamPlayer (nPlayer);
	else
		netGame.m_info.RemoveTeamPlayer (nPlayer);
	NetworkSendNetGameUpdate ();
	}
sprintf (gameData.multigame.msg.szMsg, TXT_TEAMCHANGE3, gameData.multiplayer.players [nPlayer].callsign);
if (nPlayer == N_LOCALPLAYER) {
	HUDInitMessage (TXT_TEAMJOIN, szTeamColors [team]);
	}
else
	HUDInitMessage (TXT_TEAMJOIN2, gameData.multiplayer.players [nPlayer].callsign, szTeamColors [team]);
}

//-----------------------------------------------------------------------------

void MultiDoSetTeam (char *buf)
{
SetTeam (int (buf [1]), int (buf [2]));
}

//-----------------------------------------------------------------------------

void StartPlayerDeathSequence (CObject *player);

void SwitchTeam (int nPlayer, int bForce)
{
if (gameStates.app.bHaveExtraGameInfo [1] &&(!extraGameInfo [1].bAutoBalanceTeams || bForce)) {
	int t = !GetTeam (nPlayer);
	SetTeam (nPlayer, t);
	MultiSendSetTeam (nPlayer);
	if (!bForce) {
		gameData.multiplayer.players [nPlayer].SetShield (-1);
		StartPlayerDeathSequence (OBJECTS + gameData.multiplayer.players [nPlayer].nObject);
		}
	}
}

//-----------------------------------------------------------------------------

void ChoseTeam (int nPlayer, bool bForce)
{
if (gameStates.app.bHaveExtraGameInfo && (bForce || extraGameInfo [1].bAutoBalanceTeams) && IsTeamGame && IAmGameHost ()) {
	int	h, i, t, teamScore [2] = {0, 0}, teamSize [2] = {0, 0};

	for (h = i = 0; i < gameData.multiplayer.nPlayers; i++) {
		t = GetTeam (i);
		teamSize [t]++;
		teamScore [t] += gameData.multiplayer.players [i].score;
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

		static int nTimeout = 0;

	if (gameStates.app.nSDLTicks [0] - nTimeout < 1000)
		return;
	nTimeout = gameStates.app.nSDLTicks [0];

		int	h, i, t, teamSize [2], teamScore [2];

	teamSize [0] =
	teamSize [1] =
	teamScore [0] =
	teamScore [1] = 0;
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		t = GetTeam (i);
		teamSize [t]++;
		teamScore [t] = gameData.multiplayer.players [i].score;
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
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (GetTeam (i) != t)
			continue;
		if ((gameData.app.GameMode (GM_CAPTURE)) && (gameData.multiplayer.players [i].flags & PLAYER_FLAGS_FLAG))
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
	int i;

memset (gameData.multigame.score.matrix, 0, MAX_NUM_NET_PLAYERS*MAX_NUM_NET_PLAYERS*2); // Clear kill matrix
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++) {
	gameData.multigame.score.nSorted [i] = i;
	gameData.multiplayer.players [i].netKilledTotal = 0;
	gameData.multiplayer.players [i].netKillsTotal = 0;
	gameData.multiplayer.players [i].flags = 0;
	gameData.multiplayer.players [i].nScoreGoalCount = 0;
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
gameData.multigame.laser.bFired = 0;  // How many times we shot
gameData.multigame.msg.bSending = 0;
gameData.multigame.msg.bDefining = 0;
gameData.multigame.msg.nIndex = 0;
gameData.multigame.msg.nReceiver = -1;
gameData.multigame.bGotoSecret = 0;
gameData.multigame.menu.bInvoked = 0;
gameData.multigame.menu.bLeave = 0;
gameData.multigame.bQuitGame = 0;
GameDisableCheats ();
gameData.multiplayer.players [N_LOCALPLAYER].m_bExploded = 0;
gameData.objs.deadPlayerCamera = 0;
}

//-----------------------------------------------------------------------------

void MultiMakePlayerGhost (int nPlayer)
{
	CObject *objP;

if ((nPlayer == N_LOCALPLAYER) || (nPlayer >= MAX_NUM_NET_PLAYERS) || (nPlayer < 0)) {
	Int3 (); // Non-terminal, see Rob
	return;
	}
objP = OBJECTS + gameData.multiplayer.players [nPlayer].nObject;
objP->SetType (OBJ_GHOST);
objP->info.renderType = RT_NONE;
objP->info.movementType = MT_NONE;
MultiResetPlayerObject (objP);
if (gameData.app.GameMode (GM_MULTI_ROBOTS))
	MultiStripRobots (nPlayer);
}

//-----------------------------------------------------------------------------

void MultiMakeGhostPlayer (int nPlayer)
{
	CObject *objP;

if ((nPlayer == N_LOCALPLAYER) || (nPlayer >= MAX_NUM_NET_PLAYERS)) {
	Int3 (); // Non-terminal, see rob
	return;
	}
objP = OBJECTS + gameData.multiplayer.players [nPlayer].nObject;
objP->SetType (OBJ_PLAYER);
objP->info.movementType = MT_PHYSICS;
MultiResetPlayerObject (objP);
}

//-----------------------------------------------------------------------------
// Returns the number of active net players and their sorted order of kills

int MultiGetKillList (int *listP)
{
for (int i = 0; i < gameData.multiplayer.nPlayers; i++)
	listP [i] = gameData.multigame.score.nSorted [i];
return  gameData.multiplayer.nPlayers;
}

//-----------------------------------------------------------------------------

static void QSortScoreList (int* score, int left, int right)
{
	int	l = left,
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

	int score [MAX_PLAYERS];
	int i;

for (i = 0; i < MAX_NUM_NET_PLAYERS; i++) {
	if (IsCoopGame)
		score [i] = gameData.multiplayer.players [i].score;
	else if (gameData.multigame.score.bShowList != 2) 
		score [i] = gameData.multiplayer.players [i].netKillsTotal;
	else {
		if (gameData.multiplayer.players [i].netKilledTotal + gameData.multiplayer.players [i].netKillsTotal)
			score [i] = int (double (gameData.multiplayer.players [i].netKillsTotal) / 
								  double (gameData.multiplayer.players [i].netKilledTotal + gameData.multiplayer.players [i].netKillsTotal) / 100.0);
		else
			score [i] = -1;  // always draw the ones without any ratio last
		}
	}
QSortScoreList (score, 0, gameData.multiplayer.nPlayers - 1);
}

//-----------------------------------------------------------------------------

char bMultiSuicide = 0;

void MultiComputeKill (int nKiller, int nKilled, int nKillerPlayer = -1)
{
	// Figure out the results of a network kills and add it to the
	// appropriate player's tally.

	int				nKilledPlayer, killedType, t0, t1;
	int				killerType, nKillerId;
	char				szKilled [CALLSIGN_LEN * 2 + 4];
	char				szKiller [CALLSIGN_LEN * 2 + 4];
	CPlayerInfo*	killerP, * killedP;
	CObject*			objP;

gameData.score.nKillsChanged = 1;
bMultiSuicide = 0;

// Both CObject numbers are localized already!
if ((nKilled < 0) || (nKilled > gameData.objs.nLastObject [0]) ||
	 (nKiller < 0) || (nKiller > gameData.objs.nLastObject [0])) {
	Int3 (); // See Rob, illegal value passed to computeKill;
	return;
	}
objP = OBJECTS + nKilled;
killedType = objP->info.nType;
nKilledPlayer = objP->info.nId;
if ((nKillerPlayer >= 0) && (nKillerPlayer < gameData.multiplayer.nPlayers)) {
	nKiller = gameData.multiplayer.players [nKillerPlayer].nObject;
	objP = OBJECTS + nKiller;
	}
else {
	objP = OBJECTS + nKiller;
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
	sprintf (szKilled, "%s (%s)", killedP->callsign, netGame.m_info.szTeamName [GetTeam (nKilledPlayer)]);
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
	sprintf (szKiller, "%s (%s)", killerP->callsign, netGame.m_info.szTeamName [GetTeam (nKillerPlayer)]);
else
	sprintf (szKiller, "%s", killerP->callsign);
// Beyond this point, it was definitely a player-player kill situation
#if DBG
if ((nKillerPlayer < 0) || (nKillerPlayer  >= gameData.multiplayer.nPlayers))
	Int3 (); // See rob, tracking down bug with kill HUD messages
if ((nKilledPlayer < 0) || (nKilledPlayer  >= gameData.multiplayer.nPlayers))
	Int3 (); // See rob, tracking down bug with kill HUD messages
#endif
t0 = GetTeam (nKilledPlayer);
t1 = GetTeam (nKillerPlayer);
if (nKillerPlayer == nKilledPlayer) {
	if (!(gameData.app.nGameMode &(GM_HOARD | GM_ENTROPY))) {
		if (IsTeamGame)
			gameData.multigame.score.nTeam [GetTeam (nKilledPlayer)]--;
		gameData.multiplayer.players [nKilledPlayer].netKilledTotal++;
		gameData.multiplayer.players [nKilledPlayer].netKillsTotal--;
		if (gameData.demo.nState == ND_STATE_RECORDING)
			NDRecordMultiKill (nKilledPlayer, -1);
		}
	gameData.multigame.score.matrix [nKilledPlayer][nKilledPlayer]++; // # of suicides
	if (nKillerPlayer == N_LOCALPLAYER) {
		HUDInitMessage ("%s %s %s!", TXT_YOU, TXT_KILLED, TXT_YOURSELF);
		bMultiSuicide = 1;
		MultiAddLifetimeKilled ();
		}
	else
		HUDInitMessage ("%s %s", szKilled, TXT_SUICIDE);
	}
else {
	if (IsHoardGame) {
		if (IsTeamGame) {
			if ((nKilledPlayer == N_LOCALPLAYER) && (t0 == t1))
				bMultiSuicide = 1;
			}
		}
	else if (IsEntropyGame) {
		if (t0 == t1) {
			if (nKilledPlayer == N_LOCALPLAYER)
				bMultiSuicide = 1;
			}
		else {
			killerP->secondaryAmmo [SMARTMINE_INDEX] += extraGameInfo [1].entropy.nBumpVirusCapacity;
			if (extraGameInfo [1].entropy.nMaxVirusCapacity)
				if (gameData.multiplayer.players [nKillerPlayer].secondaryAmmo [SMARTMINE_INDEX] > extraGameInfo [1].entropy.nMaxVirusCapacity)
					gameData.multiplayer.players [nKillerPlayer].secondaryAmmo [SMARTMINE_INDEX] = extraGameInfo [1].entropy.nMaxVirusCapacity;
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
		gameData.multigame.msg.buf [0] = (char) MULTI_SYNC_MONSTERBALL;
		int i = 1;
		PUT_INTEL_SHORT (gameData.multigame.msg.buf + i, gameData.hoard.monsterballP->info.nSegment);
		i += sizeof (short);
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

void MultiDoSyncMonsterball (char* buf)
{
if ((gameData.app.GameMode (GM_MONSTERBALL)) && !IAmGameHost ()) {
	CFixVector	vPos;
	bool			bSync = false;
	int			i = 1;

	short nSegment = GET_INTEL_SHORT (buf + i);
	if (nSegment >= 0) {
		if (!FindMonsterball ()) {
			gameData.hoard.nMonsterballSeg = nSegment;
			if (!CreateMonsterball ())
				return;
			bSync = true;
			}
		i += sizeof (short);
		memcpy (&vPos, buf + i, sizeof (gameData.hoard.monsterballP->info.position.vPos));
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
		INTEL_VECTOR (vPos);
#endif
		if (bSync || (CFixVector::Dist (vPos, gameData.hoard.monsterballP->info.position.vPos) > I2X (20))) {
			gameData.hoard.monsterballP->info.position.vPos = vPos;
			i += sizeof (gameData.hoard.monsterballP->info.position.vPos);
			memcpy (&gameData.hoard.monsterballP->info.position.mOrient, buf + i, sizeof (gameData.hoard.monsterballP->info.position.mOrient));
			i += sizeof (gameData.hoard.monsterballP->info.position.mOrient);
			memcpy (&gameData.hoard.monsterballP->mType.physInfo.velocity, buf + i, sizeof (gameData.hoard.monsterballP->mType.physInfo.velocity));
			i += sizeof (gameData.hoard.monsterballP->mType.physInfo.velocity);
			memcpy (&gameData.hoard.monsterballP->mType.physInfo.rotThrust, buf + i, sizeof (gameData.hoard.monsterballP->mType.physInfo.thrust));
			i += sizeof (gameData.hoard.monsterballP->mType.physInfo.thrust);
			memcpy (&gameData.hoard.monsterballP->mType.physInfo.rotVel, buf + i, sizeof (gameData.hoard.monsterballP->mType.physInfo.rotVel));
			i += sizeof (gameData.hoard.monsterballP->mType.physInfo.rotVel);
			memcpy (&gameData.hoard.monsterballP->mType.physInfo.rotThrust, buf + i, sizeof (gameData.hoard.monsterballP->mType.physInfo.rotThrust));
			i += sizeof (gameData.hoard.monsterballP->mType.physInfo.rotThrust);
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
	static int lasttime = 0;
	int i;

if (!IsMultiGame) {
	Int3 ();
	return;
	}

if (IsNetworkGame && netGame.GetPlayTimeAllowed () && (lasttime != X2I (gameStates.app.xThisLevelTime))) {
	for (i = 0; i < gameData.multiplayer.nPlayers; i++)
		if (gameData.multiplayer.players [i].Connected ()) {
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
NetworkDoFrame (0, 1);
if (gameData.multigame.bQuitGame && !(gameData.multigame.menu.bInvoked || gameStates.menus.nInMenu)) {
	gameData.multigame.bQuitGame = 0;
	longjmp (gameExitPoint, 0);
	}
MultiAdjustPowerupCap ();
MultiSyncMonsterball ();
}

//-----------------------------------------------------------------------------

void MultiSendData (char *buf, int len, int bUrgent)
{
#if DBG
if (len != MultiMsgLen (int (buf [0])))
	len = len;
#endif
if (IsNetworkGame &&(buf [0] < 1))
	return;
else if (IsNetworkGame)
	NetworkSendData (reinterpret_cast<ubyte*> (buf), len, bUrgent);
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
	d_srand (gameStates.app.nRandSeed = TimerGetFixedSeconds ());
	fix shield = LOCALPLAYER.Shield ();
	LOCALPLAYER.SetShield (-1);
	DropPlayerEggs (gameData.objs.consoleP);
	LOCALPLAYER.SetShield (shield);
	MultiSendPosition (LOCALPLAYER.nObject);
	MultiSendPlayerExplode (MULTI_PLAYER_DROP);
	}
MultiSendQuit (MULTI_QUIT);
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

int MultiEndLevel (int *secret)
{
return IsNetworkGame ? NetworkEndLevel (secret) : 0;
}

//
// Part 2 : functions that act on network/serial messages and change
//          the state of the game in some way.
//

//-----------------------------------------------------------------------------

int MultiMenuPoll (void)
{
	fix xOldShield;
	int bWasFuelCenAlive;
	int bPlayerWasDead;

bWasFuelCenAlive = gameData.reactor.bDestroyed;
// Special polling function for in-game menus for multiplayer and serial
if (!(IsMultiGame &&(gameStates.app.nFunctionMode == FMODE_GAME)))
	return 0;
if (gameData.multigame.menu.bLeave)
	return -1;
xOldShield = LOCALPLAYER.Shield ();
bPlayerWasDead = gameStates.app.bPlayerIsDead;
if (!gameOpts->menus.nStyle && gameStates.app.bGameRunning) {
	gameData.multigame.menu.bInvoked++; // Track level of menu nesting
	GameFrame (0, -1, 40);
	gameData.multigame.menu.bInvoked--;
	G3_SLEEP (100);   // delay 100 milliseconds
	}
if (gameStates.app.bEndLevelSequence ||
	 (gameData.reactor.bDestroyed && !bWasFuelCenAlive) ||
	 (gameStates.app.bPlayerIsDead != bPlayerWasDead) ||
	 (LOCALPLAYER.Shield () < xOldShield)) {
	gameData.multigame.menu.bLeave = 1;
	return -1;
	}
if (gameData.reactor.bDestroyed &&(gameData.reactor.countdown.nSecsLeft < 10)) {
	gameData.multigame.menu.bLeave = 1;
	return -1;
	}
return 0;
}

//-----------------------------------------------------------------------------
// Do any miscellaneous stuff for a new network player after death

void MultiDoDeath (int nObject)
{
if (!IsCoopGame)
	LOCALPLAYER.flags |= (PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_GOLD_KEY);
}

//-----------------------------------------------------------------------------

void MultiDoFire (char *buf)
{
	int nPlayer = int (buf [1]);
	ubyte weapon = (ubyte) buf [2];
	int flags = int (buf [4]);

gameData.multigame.laser.nTrack = GET_INTEL_SHORT (buf + 6);
#if 0
if (gameStates.multi.nGameType == UDP_GAME) {
	gameData.multigame.laser.nFired [1] = buf [8];
	for (int i = 0; i < gameData.multigame.laser.nFired [1]; i++)
		gameData.multigame.laser.nObjects [1][i] = GET_INTEL_SHORT (buf + 9 + i * sizeof (short));
	}
#endif
if (OBJECTS [gameData.multiplayer.players [nPlayer].nObject].info.nType == OBJ_GHOST)
	MultiMakeGhostPlayer (nPlayer);
if (weapon == FLARE_ADJUST)
	LaserPlayerFire (OBJECTS + gameData.multiplayer.players [nPlayer].nObject, FLARE_ID, 6, 1, 0, -1);
else if (weapon >= MISSILE_ADJUST) {
	int h = weapon - MISSILE_ADJUST;
	ubyte weaponId = secondaryWeaponToWeaponInfo [h];
	int weaponGun = secondaryWeaponToGunNum [h] + (flags & 1);
	CPlayerInfo *playerP = gameData.multiplayer.players + nPlayer;
	if (h == GUIDED_INDEX)
		gameData.multigame.bIsGuided = 1;
	if (playerP->secondaryAmmo [h] > 0)
		playerP->secondaryAmmo [h]--;
	LaserPlayerFire (OBJECTS + gameData.multiplayer.players [nPlayer].nObject, weaponId, weaponGun, 1, 0, -1);
	}
else {
	fix xSaveCharge = gameData.FusionCharge ();
	if (weapon == FUSION_INDEX)
		gameData.SetFusionCharge (flags << 12, true);
	else if (weapon == LASER_ID) {
		if (flags & LASER_QUAD)
			gameData.multiplayer.players [nPlayer].flags |= PLAYER_FLAGS_QUAD_LASERS;
		else
			gameData.multiplayer.players [nPlayer].flags &= ~PLAYER_FLAGS_QUAD_LASERS;
		}
	FireWeapon (short (gameData.multiplayer.players [nPlayer].nObject), weapon, int (buf [3]), flags, int (buf [5]));
	gameData.SetFusionCharge (xSaveCharge, true);
	}
gameData.multigame.laser.nFired [0] =
gameData.multigame.laser.nFired [1] = 0;
}

//-----------------------------------------------------------------------------
// This routine does only player positions, mode game only

void MultiDoPosition (char *buf)
{
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tShortPos sp;
#endif
int nPlayer = (N_LOCALPLAYER + 1) % 2;
CObject *objP = OBJECTS + gameData.multiplayer.players [nPlayer].nObject;
if (IsNetworkGame) {
	Int3 (); // Get Jason, what the hell are we doing here?
	return;
	}
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
ExtractShortPos (objP, reinterpret_cast<tShortPos*> (buf+1), 0);
#else
memcpy (reinterpret_cast<ubyte*> (sp.orient), reinterpret_cast<ubyte*> (buf + 1), 9);
memcpy (reinterpret_cast<ubyte*> (&sp.coord), reinterpret_cast<ubyte*> (buf + 10), 14);
ExtractShortPos (&OBJECTS [gameData.multiplayer.players [nPlayer].nObject], &sp, 1);
#endif
if (objP->info.movementType == MT_PHYSICS)
	objP->SetThrustFromVelocity ();
}

//-----------------------------------------------------------------------------

void MultiDoReappear (char *buf)
{
	short nObject = GET_INTEL_SHORT (buf + 1);
	CObject *objP = OBJECTS + nObject;

MultiMakeGhostPlayer (objP->info.nId);
objP->CreateAppearanceEffect ();
gameData.multigame.score.pFlags [objP->info.nId] = 0;
}

//-----------------------------------------------------------------------------
// Only call this for players, not robots.  nPlayer is player number, not
// Object number.

void MultiDoPlayerExplode (char *buf)
{
	CObject*			objP;
	CPlayerData*	playerP;
	int				bufI, nPlayer, i;
	short				nRemoteObj;
	char				nRemoteCreated;

nPlayer = buf [1];
#if !DBG
if ((nPlayer < 0) || (nPlayer >= gameData.multiplayer.nPlayers))
	return;
#endif
// If we are in the process of sending OBJECTS to a new player, reset that process
NetworkResetObjSync (-1);
// Stuff the gameData.multiplayer.players structure to prepare for the explosion
gameData.multiplayer.weaponStates [nPlayer].nAmmoUsed = 0;
playerP = gameData.multiplayer.players + nPlayer;
bufI = 2;
playerP->primaryWeaponFlags = GET_INTEL_SHORT (buf + bufI);
bufI += 2;
playerP->secondaryWeaponFlags = GET_INTEL_SHORT (buf + bufI);
bufI += 2;
playerP->ComputeLaserLevels (buf [bufI++]);
playerP->secondaryAmmo [HOMING_INDEX] = buf [bufI++];
playerP->secondaryAmmo [CONCUSSION_INDEX] = buf [bufI++];
playerP->secondaryAmmo [SMART_INDEX] = buf [bufI++];
playerP->secondaryAmmo [MEGA_INDEX] = buf [bufI++];
playerP->secondaryAmmo [PROXMINE_INDEX] = buf [bufI++];
playerP->secondaryAmmo [FLASHMSL_INDEX] = buf [bufI++];
playerP->secondaryAmmo [GUIDED_INDEX] = buf [bufI++];
playerP->secondaryAmmo [SMARTMINE_INDEX] = buf [bufI++];
playerP->secondaryAmmo [MERCURY_INDEX] = buf [bufI++];
playerP->secondaryAmmo [EARTHSHAKER_INDEX] = buf [bufI++];
playerP->primaryAmmo [VULCAN_INDEX] = GET_INTEL_SHORT (buf + bufI);
bufI += 2;
playerP->primaryAmmo [GAUSS_INDEX] = GET_INTEL_SHORT (buf + bufI);
bufI += 2;
playerP->flags = GET_INTEL_INT (buf + bufI);
bufI += 4;
#if 1
if (multiMessageLengths [MULTI_PLAYER_EXPLODE][1] > 0) {
	if (gameStates.multi.nGameType == UDP_GAME) {
		playerP->SetLaserLevels (buf [bufI], buf [bufI+ 1]);
		bufI += 2;
		gameStates.app.nRandSeed = GET_INTEL_SHORT (buf + bufI);
		bufI += 2;
		}
	}
#endif
#if 0
MultiAdjustRemoteCap (nPlayer);
#endif
objP = OBJECTS + playerP->nObject;
nRemoteCreated = buf [bufI++]; // How many did the other guy create?
gameData.multigame.create.nCount = 0;
//if (gameStates.multi.nGameType != UDP_GAME) 
	{
	fix shield = playerP->Shield ();
	playerP->SetShield (-1, false);
#if 0
	if (multiMessageLengths [1][MULTI_PLAYER_EXPLODE] < 0)
	if (gameStates.multi.nGameType != UDP_GAME)
#endif
	DropPlayerEggs (objP);
	playerP->SetShield (shield);
// Create mapping from remote to local numbering system
	for (i = 0; (i < nRemoteCreated) && (i < gameData.multigame.create.nCount); i++) {
		nRemoteObj = GET_INTEL_SHORT (buf + bufI);
		if (nRemoteObj > 0)
			MapObjnumLocalToRemote ((short) gameData.multigame.create.nObjNums [i], nRemoteObj, nPlayer);
		bufI += 2;
		}
	for (; i < gameData.multigame.create.nCount; i++)
		OBJECTS [gameData.multigame.create.nObjNums [i]].Die ();
	}
if (buf [0] == MULTI_PLAYER_EXPLODE) {
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

void MultiDoKill (char *buf)
{
int nPlayer = (int) (buf [1]);
if ((nPlayer < 0) || (nPlayer >= gameData.multiplayer.nPlayers)) {
	Int3 (); // Invalid player number nKilled
	return;
	}
int nKilled = gameData.multiplayer.players [nPlayer].nObject;
gameData.multiplayer.players [nPlayer].SetShield (-1);
int nKiller = GET_INTEL_SHORT (buf + 2);
if (nKiller < 0) 
	nPlayer = -1;
else
	nKiller = ObjnumRemoteToLocal (nKiller, sbyte (nPlayer = int (buf [4])));
#if DBG
MultiComputeKill (nKiller, nKilled);
#else
MultiComputeKill (nKiller, nKilled, nPlayer);
#endif
}

//-----------------------------------------------------------------------------
// Changed by MK on 10/20/94 to send NULL as CObject to NetDestroyReactor if it got -1
// which means not a controlcen CObject, but contained in another CObject

void MultiDoDestroyReactor (char *buf)
{
short nObject = GET_INTEL_SHORT (buf + 1);
sbyte who = buf [3];
if (gameData.reactor.bDestroyed != 1) {
	if ((who < gameData.multiplayer.nPlayers) && (who != N_LOCALPLAYER))
		HUDInitMessage ("%s %s", gameData.multiplayer.players [who].callsign, TXT_HAS_DEST_CONTROL);
	else if (who == N_LOCALPLAYER)
		HUDInitMessage (TXT_YOU_DEST_CONTROL);
	else
		HUDInitMessage (TXT_CONTROL_DESTROYED);
	if (nObject != -1)
		NetDestroyReactor (OBJECTS + nObject);
	else
		NetDestroyReactor (NULL);
	}
}

//-----------------------------------------------------------------------------

void MultiDoCountdown (char *buf)
{
if (NetDestroyReactor (ObjFindFirstOfType (OBJ_REACTOR)))
	 InitCountdown (NULL, 1, GET_INTEL_INT (buf + 1));
}

//-----------------------------------------------------------------------------

void MultiDoEscape (char *buf)
{
	int nObject;

nObject = gameData.multiplayer.players [(int) buf [1]].nObject;
audio.PlaySound (SOUND_HUD_MESSAGE);
audio.DestroyObjectSound (nObject);
if (buf [2] == 0) {
	HUDInitMessage ("%s %s", gameData.multiplayer.players [(int) buf [1]].callsign, TXT_HAS_ESCAPED);
	if (IsNetworkGame)
		CONNECT ((int) buf [1], CONNECT_ESCAPE_TUNNEL);
	if (!gameData.multigame.bGotoSecret)
		gameData.multigame.bGotoSecret = 2;
	}
else if (buf [2] == 1) {
	HUDInitMessage ("%s %s", gameData.multiplayer.players [(int) buf [1]].callsign, TXT_HAS_FOUND_SECRET);
	if (IsNetworkGame)
		CONNECT ((int) buf [1], CONNECT_FOUND_SECRET);
	if (!gameData.multigame.bGotoSecret)
		gameData.multigame.bGotoSecret = 1;
	}
OBJECTS [nObject].CreateAppearanceEffect ();
MultiMakePlayerGhost (buf [1]);
}

//-----------------------------------------------------------------------------

void MultiDoRemoveObj (char *buf)
{
if ((gameStates.multi.nGameType == UDP_GAME) && (buf [4] == N_LOCALPLAYER))
	return;

short nObject = GET_INTEL_SHORT (buf + 1);
if (nObject < 1)
	return;
sbyte nObjOwner = buf [3];
short nLocalObj = ObjnumRemoteToLocal (nObject, nObjOwner); // translate to local nObject
if (nLocalObj < 0)
	return;
CObject* objP = OBJECTS + nLocalObj;
if ((gameStates.multi.nGameType == UDP_GAME) && (objP->info.nType == OBJ_ROBOT)) {
	if (objP->cType.aiInfo.xDyingStartTime > 0)	// robot death sequence
		return;
	}
else if ((objP->info.nType != OBJ_POWERUP) && (objP->info.nType != OBJ_HOSTAGE))
	return;
if (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_EXPLODING | OF_DESTROYED))
	return;
NetworkResetObjSync (nLocalObj);
if (objP->info.nType == OBJ_POWERUP)
	if (IsNetworkGame) 
		RemovePowerupInMine (objP->info.nId);
if (objP->info.nType == OBJ_ROBOT)
	MultiDestroyRobot (objP);
else
	objP->MultiDie (); // quick and painless
}

//-----------------------------------------------------------------------------

void MultiOnlyPlayerMsg (int bMsgBox)
{
if (bMsgBox)
	MsgBox (NULL, NULL, 1, TXT_OK, TXT_ONLY_PLAYER);
else {
	char szMsg [100];

	audio.PlaySound (SOUND_HUD_MESSAGE);
	sprintf (szMsg, "%c%c%c%c%s", 1, 127 + 128, 63 + 128, 128, TXT_ONLY_PLAYER);
	HUDInitMessage (szMsg);
	}
}

//-----------------------------------------------------------------------------

void MultiDoQuit (char *buf)
{
if (IsNetworkGame) {
	audio.PlaySound (SOUND_HUD_MESSAGE);
	int nPlayer = int (buf [1]);
	HUDInitMessage ("%s %s", gameData.multiplayer.players [nPlayer].callsign, TXT_HAS_LEFT_THE_GAME);
	NetworkDisconnectPlayer (buf [1]);
	*gameData.multiplayer.players [nPlayer].callsign = '\0';
	if (gameData.multigame.menu.bInvoked || gameStates.menus.nInMenu)
		return;
	int nPlayers = 0;
	for (int i = 0; i < gameData.multiplayer.nPlayers; i++)
		if (gameData.multiplayer.players [i].Connected ())
			nPlayers++;
	if (nPlayers == 1)
		MultiOnlyPlayerMsg (0);
	}
}

//-----------------------------------------------------------------------------

void MultiDoInvul (char *buf)
{
int nPlayer = (int) (buf [1]);
gameData.multiplayer.players [nPlayer].flags |= PLAYER_FLAGS_INVULNERABLE;
gameData.multiplayer.players [nPlayer].cloakTime = gameData.time.xGame;
if (gameData.app.GameMode (GM_MULTI_ROBOTS))
	MultiStripRobots (nPlayer);
}

//-----------------------------------------------------------------------------

void MultiDoDeInvul (char *buf)
{
int nPlayer = (int) (buf [1]);
gameData.multiplayer.players [nPlayer].flags &= ~PLAYER_FLAGS_INVULNERABLE;
}

//-----------------------------------------------------------------------------

void MultiDoCloak (char *buf)
{
int nPlayer = (int) (buf [1]);
gameData.multiplayer.players [nPlayer].flags |= PLAYER_FLAGS_CLOAKED;
gameData.multiplayer.players [nPlayer].cloakTime = gameData.time.xGame;
AIDoCloakStuff ();
if (gameData.app.GameMode (GM_MULTI_ROBOTS))
	MultiStripRobots (nPlayer);
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordMultiCloak (nPlayer);
}

//-----------------------------------------------------------------------------

void MultiDoDeCloak (char *buf)
{
int nPlayer = (int) (buf [1]);
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordMultiDeCloak (nPlayer);
}

//-----------------------------------------------------------------------------

void MultiDoDoorOpen (char *buf)
{
	int nSegment;
	sbyte nSide;
	CSegment *segP;
	CWall *wallP;
	ubyte flag;

nSegment = GET_INTEL_SHORT (buf + 1);
nSide = buf [3];
flag = buf [4];

if ((nSegment < 0) || (nSegment > gameData.segs.nLastSegment) || (nSide < 0) || (nSide > 5)) {
	Int3 ();
	return;
	}
segP = SEGMENTS + nSegment;
if (!(wallP = segP->Wall (nSide))) {  //Opening door on illegal CWall
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

void MultiDoCreateExplosion (char *buf)
{
int nPlayer = buf [1];
CreateSmallFireballOnObject (&OBJECTS [gameData.multiplayer.players [nPlayer].nObject], I2X (1), 1);
}

//-----------------------------------------------------------------------------

void MultiDoCtrlcenFire (char *buf)
{
	CFixVector	vTarget;
	char			nGun;
	short			nObject;
	int			i, count = 1;

memcpy (&vTarget, buf + count, 12);
count += 12;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)  // swap the vector to_target
vTarget.dir.coord.x = (fix)INTEL_INT ((int) vTarget.dir.coord.x);
vTarget.dir.coord.y = (fix)INTEL_INT ((int) vTarget.dir.coord.y);
vTarget.dir.coord.z = (fix)INTEL_INT ((int) vTarget.dir.coord.z);
#endif
nGun = buf [count++];
nObject = GET_INTEL_SHORT (buf + count);
if ((nObject < 0) || (nObject > gameData.objs.nLastObject [0]))
	return;
if (0 <= (i = FindReactor (OBJECTS + nObject)))
	CreateNewWeaponSimple (&vTarget, gameData.reactor.states [i].vGunPos + (int) nGun, nObject, CONTROLCEN_WEAPON_NUM, 1);
}

//-----------------------------------------------------------------------------

void MultiDoCreateWeapon (char *buf)
{
	short			nSegment;
	short			nObject;
	int			nLocalObj;
	int			nPlayer;
	int			bufI = 1;
	CFixVector	vPos, vVel;
	char			nId;

if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed)
	return;
nPlayer = int (buf [bufI++]);
if (nPlayer == N_LOCALPLAYER)
	return;
nId = buf [bufI++];
nSegment = GET_INTEL_SHORT (buf + bufI);
bufI += 2;
nObject = GET_INTEL_SHORT (buf + bufI);
bufI += 2;
if ((nSegment < 0) || (nSegment > gameData.segs.nLastSegment)) {
	Int3 ();
	return;
	}
memcpy (&vPos, buf + bufI, sizeof (CFixVector));
bufI += sizeof (CFixVector);
memcpy (&vVel, buf + bufI, sizeof (CFixVector));
bufI += sizeof (CFixVector);
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
INTEL_VECTOR (vPos);
INTEL_VECTOR (vVel);
#endif
gameData.multigame.create.nCount = 0;
nLocalObj = CreateNewWeapon (&vVel, &vPos, nSegment, nPlayer, nId, 0);
if (nLocalObj < 0)
	return;
NetworkResetObjSync (nLocalObj);
OBJECTS [nLocalObj].info.position.vPos = vPos;
OBJECTS [nLocalObj].mType.physInfo.velocity = vVel;
OBJECTS [nLocalObj].RelinkToSeg (nSegment);
MapObjnumLocalToRemote (nLocalObj, nObject, nPlayer);
return;
}

//-----------------------------------------------------------------------------

int MultiDoCreatePowerup (char *buf)
{
	short			nSegment;
	short			nObject;
	int			nLocalObj;
	int			nPlayer;
	int			bufI = 1;
	CFixVector	vPos;
	char			powerupType;

if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed)
	return -1;
nPlayer = int (buf [bufI++]);
if (nPlayer == N_LOCALPLAYER)
	return -1;
powerupType = buf [bufI++];
nSegment = GET_INTEL_SHORT (buf + bufI);
bufI += 2;
nObject = GET_INTEL_SHORT (buf + bufI);
bufI += 2;
if ((nSegment < 0) || (nSegment > gameData.segs.nLastSegment)) {
	Int3 ();
	return -1;
	}
memcpy (&vPos, buf + bufI, sizeof (CFixVector));
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
INTEL_VECTOR (vPos);
#endif
gameData.multigame.create.nCount = 0;
nLocalObj = CallObjectCreateEgg (OBJECTS + gameData.multiplayer.players [nPlayer].nObject, 1, OBJ_POWERUP, powerupType, true);
if (nLocalObj < 0)
	return -1;
NetworkResetObjSync (nLocalObj);
OBJECTS [nLocalObj].info.position.vPos = vPos;
OBJECTS [nLocalObj].mType.physInfo.velocity.SetZero ();
OBJECTS [nLocalObj].RelinkToSeg (nSegment);
MapObjnumLocalToRemote (nLocalObj, nObject, nPlayer);
CreateExplosion (nSegment, vPos, I2X (5), VCLIP_POWERUP_DISAPPEARANCE);
return nLocalObj;
}

//-----------------------------------------------------------------------------

void MultiDoDropPowerup (char *buf)
{
int nObject = MultiDoCreatePowerup (buf);
if (nObject >= 0)
OBJECTS [nObject].mType.physInfo.velocity = *reinterpret_cast<CFixVector*>(buf + 7 + sizeof (CFixVector));
}

//-----------------------------------------------------------------------------

void MultiDoPlaySound (char *buf)
{
	int nPlayer = (int) (buf [1]);
	short nSound = (int) (buf [2]);
	fix volume = (int) (buf [3]) << 12;

if (!gameData.multiplayer.players [nPlayer].Connected ())
	return;
if ((gameData.multiplayer.players [nPlayer].nObject >= 0) &&
	 (gameData.multiplayer.players [nPlayer].nObject <= gameData.objs.nLastObject [0]))
	audio.CreateObjectSound (nSound, SOUNDCLASS_PLAYER, (short) gameData.multiplayer.players [nPlayer].nObject, 0, volume);
}

//-----------------------------------------------------------------------------

void MultiDoScore (char *buf)
{
	int nPlayer = (int) (buf [1]);

if ((nPlayer < 0) || (nPlayer  >= gameData.multiplayer.nPlayers)) {
	Int3 (); // Non-terminal, see rob
	return;
	}
if (gameData.demo.nState == ND_STATE_RECORDING) {
	int score = GET_INTEL_INT (buf + 2);
	NDRecordMultiScore (nPlayer, score);
	}
gameData.multiplayer.players [nPlayer].score = GET_INTEL_INT (buf + 2);
MultiSortKillList ();
}

//-----------------------------------------------------------------------------

void MultiDoTrigger (char *buf)
{
	int nPlayer = (int) (buf [1]);
	int nTrigger = (int) ((ubyte) buf [2]);
	short nObject;

if ((nPlayer < 0) || (nPlayer >= gameData.multiplayer.nPlayers) || (nPlayer == N_LOCALPLAYER)) {
	Int3 (); // Got nTrigger from illegal nPlayer
	return;
	}
if ((nTrigger < 0) || (nTrigger  >= gameData.trigs.m_nTriggers)) {
	Int3 (); // Illegal nTrigger number in multiplayer
	return;
	}
if (gameStates.multi.nGameType == UDP_GAME)
	nObject = GET_INTEL_SHORT (buf + 3);
else
	nObject = gameData.multiplayer.players [nPlayer].nObject;
TRIGGERS [nTrigger].Operate (nObject, nPlayer, 0, 0);
}

//-----------------------------------------------------------------------------

void MultiDoShield (char *buf)
{
	int	nPlayer = int (buf [1]);
	int	shield = GET_INTEL_INT (buf+2);

if ((nPlayer < 0) || (nPlayer  >= gameData.multiplayer.nPlayers) || (nPlayer == N_LOCALPLAYER)) {
	Int3 (); // Got CTrigger from illegal nPlayer
	return;
	}
gameData.multiplayer.players [nPlayer].SetShield (shield, false);
}

//-----------------------------------------------------------------------------

void MultiDoObjTrigger (char *buf)
{
	int nPlayer = (int) (buf [1]);
	int nTrigger = (int) ((ubyte) buf [2]);

if ((nPlayer < 0) || (nPlayer  >= gameData.multiplayer.nPlayers) || (nPlayer == N_LOCALPLAYER)) {
	Int3 (); // Got nTrigger from illegal nPlayer
	return;
	}
if ((nTrigger < 0) || (nTrigger  >= gameData.trigs.m_nObjTriggers)) {
	Int3 (); // Illegal nTrigger number in multiplayer
	return;
	}
OBJTRIGGERS [nTrigger].Operate (gameData.multiplayer.players [nPlayer].nObject, nPlayer, 0, 1);
}

//-----------------------------------------------------------------------------

void MultiDoDropMarker (char *buf)
{
	int			nPlayer = int (buf [1]);
	int			nMsg = int (buf [2]);
	int			nMarker = nPlayer * 2 + nMsg;
	CFixVector	vPos;

if (nPlayer == N_LOCALPLAYER)  // my marker? don't set it down cuz it might screw up the orientation
	return;
vPos.v.coord.x = GET_INTEL_INT (buf + 3);
vPos.v.coord.y = GET_INTEL_INT (buf + 7);
vPos.v.coord.z = GET_INTEL_INT (buf + 11);
memcpy (markerManager.Message (2 * nPlayer + nMsg), buf + 15, 40);
markerManager.SetPosition (nMarker, vPos);
if ((markerManager.Objects (nMarker) != -1) &&
	 (OBJECTS [markerManager.Objects (nMarker)].info.nType != OBJ_NONE) &&
	 (markerManager.Objects (nMarker) != 0))
	ReleaseObject (markerManager.Objects (nMarker));
markerManager.SetObject (nPlayer * 2 + nMsg, 
								 DropMarkerObject (vPos, OBJECTS [LOCALPLAYER.nObject].info.nSegment, 
								 OBJECTS [LOCALPLAYER.nObject].info.position.mOrient, 
								 (ubyte) nMarker));
markerManager.SetOwner (nMarker, gameData.multiplayer.players [nPlayer].callsign);
}

//-----------------------------------------------------------------------------
// Update hit point status of a door

void MultiDoHostageDoorStatus (char *buf)
{
	CWall	*wallP;
	fix	hps;
	short	wallnum;

wallnum = GET_INTEL_SHORT (buf + 1);
if ((!IS_WALL (wallnum)) || (wallnum > gameData.walls.nWalls))
	return;
hps = GET_INTEL_INT (buf + 3);
if (hps < 0)
	return;
wallP = WALLS + wallnum;
if (wallP->nType != WALL_BLASTABLE)
	return;
if (hps < wallP->hps)
	SEGMENTS [wallP->nSegment].DamageWall ((short) wallP->nSide, wallP->hps - hps);
}

//-----------------------------------------------------------------------------

void MultiDoSaveGame (char *buf)
{
	char desc [25];

char slot = buf [1];
uint id = GET_INTEL_INT (buf + 2);
memcpy (desc, buf + 6, 20);
MultiSaveGame (slot, id, desc);
}

//-----------------------------------------------------------------------------

void MultiDoRestoreGame (char *buf)
{
char slot = buf [1];
uint id = GET_INTEL_INT (buf + 2);
MultiRestoreGame (slot, id);
}

//-----------------------------------------------------------------------------

void MultiDoReqPlayer (char *buf)
{
	ubyte nPlayer = *reinterpret_cast<ubyte*> (buf+1);

if ((nPlayer == N_LOCALPLAYER) || (nPlayer == 255)) {
		tNetPlayerStats ps;

	ExtractNetPlayerStats (&ps, gameData.multiplayer.players + N_LOCALPLAYER);
	ps.nLocalPlayer = N_LOCALPLAYER;
	ps.messageType = MULTI_SEND_PLAYER;            // SET
	MultiSendData (reinterpret_cast<char*> (&ps), sizeof (tNetPlayerStats), 0);
	}
}

//-----------------------------------------------------------------------------
// Got a player packet from someone!!!

void MultiDoSendPlayer (char *buf)
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
gameData.objs.deadPlayerCamera = 0;
gameStates.app.bEndLevelSequence = 0;
ResetRearView ();
}

//-----------------------------------------------------------------------------

void MultiResetPlayerObject (CObject *objP)
{
	int i;

//Init physics for a non-console player
if (objP > OBJECTS + gameData.objs.nLastObject [0])
	return;
if ((objP->info.nType != OBJ_PLAYER) &&(objP->info.nType != OBJ_GHOST))
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
objP->info.renderType = RT_POLYOBJ;
objP->rType.polyObjInfo.nModel = gameData.pig.ship.player->nModel;               //what model is this?
objP->rType.polyObjInfo.nSubObjFlags = 0;         //zero the flags
for (i = 0; i < MAX_SUBMODELS; i++)
	objP->rType.polyObjInfo.animAngles [i].SetZero ();
//reset textures for this, if not player 0
MultiSetObjectTextures (objP);
// Clear misc
objP->info.nFlags = 0;
if (objP->info.nType == OBJ_GHOST)
	objP->info.renderType = RT_NONE;
}

//-----------------------------------------------------------------------------

void MultiSetObjectTextures (CObject *objP)
{
	int				id, i, j;
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
extern int TTRecv [];
extern FILE *RecieveLogFile;
#endif

//-----------------------------------------------------------------------------
// Takes a bunch of messages, check them for validity,
// and pass them to MultiProcessData.

void MultiProcessBigData (char *buf, int len)
{
	int nType, nMsgLen, nBytesProcessed = 0;

while (nBytesProcessed < len) {
	nType = buf [nBytesProcessed];
	if ((nType < 0) || (nType > MULTI_MAX_TYPE))
		return;
	nMsgLen = MultiMsgLen (nType);
	if ((nBytesProcessed + nMsgLen) > len) {
		Int3 ();
		return;
		}
	MultiProcessData (buf + nBytesProcessed, nMsgLen);
	nBytesProcessed += nMsgLen;
	}
}

//
// Part 2 : Functions that send communication messages to inform the other
//          players of something we did.
//

//-----------------------------------------------------------------------------

void MultiSendFire (void)
{
if (!gameData.multigame.laser.bFired)
	return;
gameData.multigame.msg.buf [0] = char (MULTI_FIRE);
gameData.multigame.msg.buf [1] = char (N_LOCALPLAYER);
gameData.multigame.msg.buf [2] = char (gameData.multigame.laser.nGun);
gameData.multigame.msg.buf [3] = char (gameData.multigame.laser.nLevel);
gameData.multigame.msg.buf [4] = char (gameData.multigame.laser.nFlags);
gameData.multigame.msg.buf [5] = char (gameData.multigame.laser.bFired);
PUT_INTEL_SHORT (gameData.multigame.msg.buf + 6, gameData.multigame.laser.nTrack);
#if 0
if (gameStates.multi.nGameType == UDP_GAME) {
	gameData.multigame.msg.buf [8] = char (gameData.multigame.laser.nFired [0]);
	for (int i = 0; i < gameData.multigame.laser.nFired [0]; i++)
		PUT_INTEL_SHORT (gameData.multigame.msg.buf + 9 + 2 * i, gameData.multigame.laser.nObjects [0][i]);
	MultiSendData (gameData.multigame.msg.buf, 9 + MAX_FIRED_OBJECTS * sizeof (short), 0);
	}
else
#endif
	MultiSendData (gameData.multigame.msg.buf, 8, 0);
gameData.multigame.laser.nFired [0] = 0;
gameData.multigame.laser.bFired = 0;
}

//-----------------------------------------------------------------------------

void MultiSendDestroyReactor (int nObject, int nPlayer)
{
if (nPlayer == N_LOCALPLAYER)
	HUDInitMessage (TXT_YOU_DEST_CONTROL);
else if ((nPlayer > 0) && (nPlayer < gameData.multiplayer.nPlayers))
	HUDInitMessage ("%s %s", gameData.multiplayer.players [nPlayer].callsign, TXT_HAS_DEST_CONTROL);
else
	HUDInitMessage (TXT_CONTROL_DESTROYED);
gameData.multigame.msg.buf [0] = (char) MULTI_CONTROLCEN;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + 1, nObject);
gameData.multigame.msg.buf [3] = nPlayer;
MultiSendData (gameData.multigame.msg.buf, 4, 2);
}

//-----------------------------------------------------------------------------

void MultiSendCountdown (void)
{
gameData.multigame.msg.buf [0] = (char) MULTI_COUNTDOWN;
PUT_INTEL_INT (gameData.multigame.msg.buf+1, gameData.reactor.countdown.nTimer);
MultiSendData (gameData.multigame.msg.buf, 5, 2);
}

//-----------------------------------------------------------------------------

void MultiSendDropMarker (int nPlayer, CFixVector position, char nMessage, char text [])
{
if (nPlayer < gameData.multiplayer.nPlayers) {
	gameData.multigame.msg.buf [0] = (char) MULTI_MARKER;
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

void MultiSendEndLevelStart (int secret)
{
gameData.multigame.msg.buf [0] = (char) MULTI_ENDLEVEL_START;
gameData.multigame.msg.buf [1] = N_LOCALPLAYER;
gameData.multigame.msg.buf [2] = (char) secret;
if ((secret) && !gameData.multigame.bGotoSecret)
	gameData.multigame.bGotoSecret = 1;
else if (!gameData.multigame.bGotoSecret)
	gameData.multigame.bGotoSecret = 2;
MultiSendData (gameData.multigame.msg.buf, 3, 1);
if (IsNetworkGame) {
	CONNECT (N_LOCALPLAYER, CONNECT_ESCAPE_TUNNEL);
	NetworkSendEndLevelPacket ();
	}
}

//-----------------------------------------------------------------------------

void MultiSendPlayerExplode (char nType)
{
	int bufI = 0;
	int i;

MultiSendPosition (LOCALPLAYER.nObject);
NetworkResetObjSync (-1);
gameData.multigame.msg.buf [bufI++] = nType;
gameData.multigame.msg.buf [bufI++] = N_LOCALPLAYER;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufI, LOCALPLAYER.primaryWeaponFlags);
bufI += 2;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufI, LOCALPLAYER.secondaryWeaponFlags);
bufI += 2;
gameData.multigame.msg.buf [bufI++] = char (LOCALPLAYER.laserLevel);
gameData.multigame.msg.buf [bufI++] = char (gameData.multiplayer.SecondaryAmmo (N_LOCALPLAYER, HOMING_INDEX));
gameData.multigame.msg.buf [bufI++] = char (gameData.multiplayer.SecondaryAmmo (N_LOCALPLAYER, CONCUSSION_INDEX, 0));
gameData.multigame.msg.buf [bufI++] = char (gameData.multiplayer.SecondaryAmmo (N_LOCALPLAYER, SMART_INDEX));
gameData.multigame.msg.buf [bufI++] = char (gameData.multiplayer.SecondaryAmmo (N_LOCALPLAYER, MEGA_INDEX));
gameData.multigame.msg.buf [bufI++] = char (gameData.multiplayer.SecondaryAmmo (N_LOCALPLAYER, PROXMINE_INDEX));
gameData.multigame.msg.buf [bufI++] = char (gameData.multiplayer.SecondaryAmmo (N_LOCALPLAYER, FLASHMSL_INDEX));
gameData.multigame.msg.buf [bufI++] = char (gameData.multiplayer.SecondaryAmmo (N_LOCALPLAYER, GUIDED_INDEX));
gameData.multigame.msg.buf [bufI++] = char (gameData.multiplayer.SecondaryAmmo (N_LOCALPLAYER, SMARTMINE_INDEX));
gameData.multigame.msg.buf [bufI++] = char (gameData.multiplayer.SecondaryAmmo (N_LOCALPLAYER, MERCURY_INDEX));
gameData.multigame.msg.buf [bufI++] = char (gameData.multiplayer.SecondaryAmmo (N_LOCALPLAYER, EARTHSHAKER_INDEX));
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufI, LOCALPLAYER.primaryAmmo [VULCAN_INDEX] + 
					  gameData.multiplayer.weaponStates [N_LOCALPLAYER].nAmmoUsed);
bufI += 2;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufI, LOCALPLAYER.primaryAmmo [GAUSS_INDEX]);
bufI += 2;
PUT_INTEL_INT (gameData.multigame.msg.buf + bufI, LOCALPLAYER.flags);
bufI += 4;
#if 1
if (multiMessageLengths [(int) nType][1] > 0) {
	if (gameStates.multi.nGameType == UDP_GAME) {
		gameData.multigame.msg.buf [bufI++] = char (LOCALPLAYER.m_laserLevels [0]);
		gameData.multigame.msg.buf [bufI++] = char (LOCALPLAYER.m_laserLevels [1]);
		PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufI, gameStates.app.nRandSeed);
		bufI += 2;
		}
	}
#endif
gameData.multigame.msg.buf [bufI++] = gameData.multigame.create.nCount;
memset (gameData.multigame.msg.buf + bufI, -1, MAX_NET_CREATE_OBJECTS * sizeof (short));
for (i = 0; i < gameData.multigame.create.nCount; i++) {
	if (gameData.multigame.create.nObjNums [i] <= 0) {
		Int3 (); // Illegal value in created egg CObject numbers
		bufI += 2;
		continue;
		}
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufI, gameData.multigame.create.nObjNums [i]);
	bufI += 2;
	// We created these objs so our local number = the network number
	MapObjnumLocalToLocal (short (gameData.multigame.create.nObjNums [i]));
	}
gameData.multigame.create.nCount = 0;
#if DBG
if (bufI > MultiMsgLen (nType))
	Warning ("MultiSendPlayerExplode:\nMax. message length exceeded!"); // See Rob
#endif
MultiSendData (gameData.multigame.msg.buf, MultiMsgLen (nType), 2);
if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED)
	MultiSendDeCloak ();
if (gameData.app.GameMode (GM_MULTI_ROBOTS))
	MultiStripRobots (N_LOCALPLAYER);
gameData.multiplayer.weaponStates [N_LOCALPLAYER].nAmmoUsed = 0;
}

//-----------------------------------------------------------------------------

extern ubyte secondaryWeaponToPowerup [];
extern ubyte primaryWeaponToPowerup [];

static int nDeviceFlags [] = {PLAYER_FLAGS_FULLMAP, PLAYER_FLAGS_AMMO_RACK, PLAYER_FLAGS_CONVERTER, PLAYER_FLAGS_QUAD_LASERS, PLAYER_FLAGS_AFTERBURNER};
static int nDevicePowerups [] = {POW_FULL_MAP, POW_AMMORACK, POW_CONVERTER, POW_QUADLASER, POW_AFTERBURNER};

// This function is called if a player ship is about to explode. It will make sure the ship doesn't drop more powerups of each kind than 
// the entire mine initially contained to prevent rampant powerups in netgames

void MultiCapObjects (void)
{
	int	h, i, nType, nFlagType;

if (!IsNetworkGame)
	return;
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++) {
	nType = (i == SUPER_LASER_INDEX) ? POW_SUPERLASER : int (primaryWeaponToPowerup [i]);
	if (!LOCALPLAYER.primaryWeaponFlags & (1 << i))
		continue;
	h = MissingPowerups (nType);
	if (i == LASER_INDEX) {
		if (LOCALPLAYER.LaserLevel (0) > h)
			LOCALPLAYER.SetStandardLaser (h);
		}
	else if (i == SUPER_LASER_INDEX) {
		if (LOCALPLAYER.LaserLevel (1) > h)
			LOCALPLAYER.SetSuperLaser (h);
		}
	else {
		if ((i == FUSION_INDEX) && (h < 2))
			gameData.multiplayer.weaponStates [N_LOCALPLAYER].bTripleFusion = 0;
		if (h < 1)
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
	nType = int (secondaryWeaponToPowerup [i]);
	h = MissingPowerups (nType);
	if (LOCALPLAYER.secondaryAmmo [i] > h)
		LOCALPLAYER.secondaryAmmo [i] = max (h, 0);
	}

if (!(gameData.app.nGameMode &(GM_HOARD | GM_ENTROPY)))
	LOCALPLAYER.secondaryAmmo [2] *= 4;
LOCALPLAYER.secondaryAmmo [7] *= 4;

for (i = 0; i < int (sizeofa (nDeviceFlags)); i++) {
	if (LOCALPLAYER.flags & nDeviceFlags [i]) {
		nType = nDevicePowerups [i];
		if (0 >= MissingPowerups (nType))
			LOCALPLAYER.flags &= (~PLAYER_FLAGS_CLOAKED);
		}
	}

if (PlayerHasHeadlight (-1) && (0 >= MissingPowerups (POW_HEADLIGHT)))
	LOCALPLAYER.flags &= (~PLAYER_FLAGS_HEADLIGHT);

if (gameData.app.GameMode (GM_CAPTURE)) {
	if (LOCALPLAYER.flags & PLAYER_FLAGS_FLAG) {
		if (GetTeam (N_LOCALPLAYER) == TEAM_RED)
			nFlagType = POW_BLUEFLAG;
		else
			nFlagType = POW_REDFLAG;
		if (MissingPowerups (nFlagType) < 1)
			LOCALPLAYER.flags &= (~PLAYER_FLAGS_FLAG);
		}
	}
}

//-----------------------------------------------------------------------------
// adds players inventory to multi cap

void MultiAdjustCapForPlayer (int nPlayer)
{
	int	i, nType;

if (!IsNetworkGame)
	return;
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++) {
	nType = int (primaryWeaponToPowerup [i]);
	if (IsBuiltinWeapon (i))	// weapon is standard loadout
		continue;
	if (i == 0) {// laser
		if (IsBuiltinWeapon (SUPER_LASER_INDEX))
			continue;
		if (!IsBuiltinWeapon (LASER_INDEX))	// lasers or superlasers are standard loadout
			AddAllowedPowerup (POW_LASER, gameData.multiplayer.players [nPlayer].LaserLevel (0));
		if (gameData.multiplayer.players [nPlayer].laserLevel > MAX_LASER_LEVEL)
			AddAllowedPowerup (POW_SUPERLASER, gameData.multiplayer.players [nPlayer].LaserLevel (1));
		}
	else if (i != 5) { // super laser
		if (IsBuiltinWeapon (i))
			continue;
		if (gameData.multiplayer.players [nPlayer].primaryWeaponFlags & (1 << i)) {
		    AddAllowedPowerup (nType);
			if ((nType == POW_FUSION) && (gameData.multiplayer.weaponStates [nPlayer].bTripleFusion > 0))
				AddAllowedPowerup (primaryWeaponToPowerup [FUSION_INDEX]);
			}
		}
	}
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++) {
	nType = int (secondaryWeaponToPowerup [i]);
	AddAllowedPowerup (nType, i ? gameData.multiplayer.SecondaryAmmo (nPlayer, i) : gameData.multiplayer.SecondaryAmmo (nPlayer, i, 0));
	}

for (i = 0; i < int (sizeofa (nDeviceFlags)); i++)
	if (gameData.multiplayer.Flag (nPlayer, nDeviceFlags [i]) && !(extraGameInfo [IsMultiGame].loadout.nDevice & nDeviceFlags [i]))
		AddAllowedPowerup (nDevicePowerups [i]);
if (PlayerHasHeadlight (nPlayer) && !EGI_FLAG (headlight.bBuiltIn, 0, 1, 0))
	AddAllowedPowerup (POW_HEADLIGHT);
}

//-----------------------------------------------------------------------------

void MultiAdjustPowerupCap (void)
{
if (IAmGameHost ()) {
	for (int i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if ((i != N_LOCALPLAYER) && gameData.multiplayer.players [i].Connected () && gameData.multiplayer.bAdjustPowerupCap [i]) {
			MultiAdjustCapForPlayer (i);
			gameData.multiplayer.bAdjustPowerupCap [i] = false;
			}
		}
	}
}

//-----------------------------------------------------------------------------

void MultiSendReappear (void)
{
gameData.multigame.msg.buf [0] = (char) MULTI_REAPPEAR;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + 1, LOCALPLAYER.nObject);
MultiSendData (gameData.multigame.msg.buf, 3, 2);
gameData.multigame.score.pFlags [N_LOCALPLAYER] = 0;
}

//-----------------------------------------------------------------------------

void MultiSendPosition (int nObject)
{
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tShortPos sp;
#endif
	int count = 0;

if (IsNetworkGame)
	return;
gameData.multigame.msg.buf [count++] = (char)MULTI_POSITION;
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
CreateShortPos (reinterpret_cast<tShortPos*> (gameData.multigame.msg.buf + count), OBJECTS+nObject, 0);
count += sizeof (tShortPos);
#else
CreateShortPos (&sp, OBJECTS+nObject, 1);
memcpy (gameData.multigame.msg.buf + count, reinterpret_cast<ubyte*> (sp.orient), 9);
count += 9;
memcpy (gameData.multigame.msg.buf + count, reinterpret_cast<ubyte*> (&sp.coord), 14);
count += 14;
#endif
MultiSendData (gameData.multigame.msg.buf, count, 0);
}

//-----------------------------------------------------------------------------
// I died, tell the world.

void MultiSendKill (int nObject)
{
	int nKillerObj;

nKillerObj = LOCALPLAYER.nKillerObj;
MultiComputeKill (nKillerObj, nObject);
gameData.multigame.msg.buf [0] = (char) MULTI_KILL;
gameData.multigame.msg.buf [1] = N_LOCALPLAYER;
if (nKillerObj > -1) {
	// do it with variable player since INTEL_SHORT won't work on return val from function.
	short s = (short) ObjnumLocalToRemote (nKillerObj, reinterpret_cast<sbyte*> (&gameData.multigame.msg.buf [4]));
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + 2, s);
	}
else {
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + 2, -1);
	gameData.multigame.msg.buf [4] = (char) -1;
	}
MultiSendData (gameData.multigame.msg.buf, 5, 1);
if (gameData.app.GameMode (GM_MULTI_ROBOTS))
	MultiStripRobots (N_LOCALPLAYER);
}

//-----------------------------------------------------------------------------

void MultiDoSyncKills (char *buf)
{
	int	nPlayer = int (buf [1]);
	char	*bufP = buf + 2;

gameData.multiplayer.players [nPlayer].score = GET_INTEL_INT (bufP);
bufP += 4;
gameData.multiplayer.players [nPlayer].nScoreGoalCount = GET_INTEL_SHORT (bufP);
bufP += 2;
gameData.multiplayer.players [nPlayer].netKilledTotal = GET_INTEL_SHORT (bufP);
bufP += 2;
gameData.multiplayer.players [nPlayer].netKillsTotal = GET_INTEL_SHORT (bufP);
bufP += 2;
gameData.multiplayer.players [nPlayer].numKillsLevel = GET_INTEL_SHORT (bufP);
bufP += 2;
gameData.multiplayer.players [nPlayer].numKillsTotal = GET_INTEL_SHORT (bufP);
}

//-----------------------------------------------------------------------------

void MultiSendSyncKills (void)
{
	char	*bufP = gameData.multigame.msg.buf + 2;

gameData.multigame.msg.buf [0] = (char) MULTI_SYNC_KILLS;
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
// Tell the other guy to remove an CObject from his list

void MultiSendRemoveObj (int nObject)
{
if ((nObject < 0) || (nObject > gameData.objs.nLastObject [0]))
	return;

	sbyte nObjOwner;
	short nRemoteObj;

if ((OBJECTS [nObject].info.nType == OBJ_POWERUP) && IsNetworkGame) {
	RemovePowerupInMine (OBJECTS [nObject].info.nId);
	}
gameData.multigame.msg.buf [0] = char (MULTI_REMOVE_OBJECT);
nRemoteObj = ObjnumLocalToRemote (short (nObject), &nObjOwner);
PUT_INTEL_SHORT (gameData.multigame.msg.buf+1, nRemoteObj); // Map to network objnums
gameData.multigame.msg.buf [3] = nObjOwner;
if (gameStates.multi.nGameType == UDP_GAME)
	gameData.multigame.msg.buf [4] = N_LOCALPLAYER;
MultiSendData (gameData.multigame.msg.buf, (gameStates.multi.nGameType == UDP_GAME) ? 5 : 4, 0);
NetworkResetObjSync (nObject);
}

//-----------------------------------------------------------------------------
// I am quitting the game, tell the other guy the bad news.

void MultiSendQuit (int why)
{
gameData.multigame.msg.buf [0] = (char)why;
gameData.multigame.msg.buf [1] = N_LOCALPLAYER;
MultiSendData (gameData.multigame.msg.buf, 2, 1);
}

//-----------------------------------------------------------------------------
// Broadcast a change in our pflags (made to support cloaking)

void MultiSendInvul (void)
{
gameData.multigame.msg.buf [0] = MULTI_INVUL;
gameData.multigame.msg.buf [1] = (char)N_LOCALPLAYER;
MultiSendData (gameData.multigame.msg.buf, 2, 1);
if (gameData.app.GameMode (GM_MULTI_ROBOTS))
	MultiStripRobots (N_LOCALPLAYER);
}

//-----------------------------------------------------------------------------
// Broadcast a change in our pflags (made to support cloaking)

void MultiSendDeInvul (void)
{
gameData.multigame.msg.buf [0] = MULTI_DEINVUL;
gameData.multigame.msg.buf [1] = (char)N_LOCALPLAYER;
MultiSendData (gameData.multigame.msg.buf, 2, 1);
}

//-----------------------------------------------------------------------------
// Broadcast a change in our pflags (made to support cloaking)

void MultiSendCloak (void)
{
gameData.multigame.msg.buf [0] = MULTI_CLOAK;
gameData.multigame.msg.buf [1] = (char)N_LOCALPLAYER;
MultiSendData (gameData.multigame.msg.buf, 2, 1);
if (gameData.app.GameMode (GM_MULTI_ROBOTS))
	MultiStripRobots (N_LOCALPLAYER);
}

//-----------------------------------------------------------------------------
// Broadcast a change in our pflags (made to support cloaking)

void MultiSendDeCloak (void)
{
gameData.multigame.msg.buf [0] = MULTI_DECLOAK;
gameData.multigame.msg.buf [1] = (char)N_LOCALPLAYER;
MultiSendData (gameData.multigame.msg.buf, 2, 1);
}

//-----------------------------------------------------------------------------
// When we open a door make sure everyone else opens that door

void MultiSendDoorOpen (int nSegment, int CSide, ushort flags)
{
gameData.multigame.msg.buf [0] = MULTI_DOOR_OPEN;
PUT_INTEL_SHORT (gameData.multigame.msg.buf+1, nSegment);
gameData.multigame.msg.buf [3] = (sbyte)CSide;
gameData.multigame.msg.buf [4] = ubyte (flags);
MultiSendData (gameData.multigame.msg.buf, 5, 2);
}

//-----------------------------------------------------------------------------
// For sending doors only to a specific person (usually when they're joining)

extern void NetworkSendNakedPacket (char *, short, int);

void MultiSendDoorOpenSpecific (int nPlayer, int nSegment, int CSide, ushort flags)
{
gameData.multigame.msg.buf [0] = MULTI_DOOR_OPEN;
PUT_INTEL_SHORT (gameData.multigame.msg.buf+1, nSegment);
gameData.multigame.msg.buf [3] = (sbyte)CSide;
gameData.multigame.msg.buf [4] = ubyte (flags);
NetworkSendNakedPacket (gameData.multigame.msg.buf, 5, nPlayer);
}

//
// Part 3 : Functions that change or prepare the game for multiplayer use.
//          Not including functions needed to synchronize or start the
//          particular nType of multiplayer game.  Includes preparing the
//                      mines, player structures, etc.

//-----------------------------------------------------------------------------

void MultiSendCreateExplosion (int nPlayer)
{
// Send all data needed to create a remote explosion
gameData.multigame.msg.buf [0] = MULTI_CREATE_EXPLOSION;
gameData.multigame.msg.buf [1] = (sbyte)nPlayer;
MultiSendData (gameData.multigame.msg.buf, 2, 0);
}

//-----------------------------------------------------------------------------

void MultiSendCtrlcenFire (CFixVector *vGoal, int nBestGun, int nObject)
{
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	CFixVector vSwapped;
#endif
	int count = 0;

gameData.multigame.msg.buf [count++] = MULTI_CONTROLCEN_FIRE;
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
memcpy (gameData.multigame.msg.buf + count, vGoal, 12);
count += 12;
#else
vSwapped.dir.coord.x = (fix) INTEL_INT ((int) (*vGoal).dir.coord.x);
vSwapped.dir.coord.y = (fix) INTEL_INT ((int) (*vGoal).dir.coord.y);
vSwapped.dir.coord.z = (fix) INTEL_INT ((int) (*vGoal).dir.coord.z);
memcpy (gameData.multigame.msg.buf + count, &vSwapped, 12);
count += 12;
#endif
gameData.multigame.msg.buf [count++] = (char)nBestGun;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + count, nObject);
count += 2;
MultiSendData (gameData.multigame.msg.buf, count, 0);
}

//-----------------------------------------------------------------------------

void MultiSendCreateWeapon (int nObject)
{
	// Create a powerup on a remote machine, used for remote
	// placement of used powerups like missiles and cloaking
	// powerups.

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	CFixVector vSwapped;
#endif
	int count = 0;

gameData.multigame.msg.buf [count++] = MULTI_CREATE_WEAPON;
gameData.multigame.msg.buf [count++] = N_LOCALPLAYER;
gameData.multigame.msg.buf [count++] = OBJECTS [nObject].info.nId;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + count, OBJECTS [nObject].info.nSegment);
count += 2;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + count, nObject);
count += 2;
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
memcpy (gameData.multigame.msg.buf + count, &OBJECTS [nObject].info.position.vPos, sizeof (CFixVector));
count += sizeof (CFixVector);
memcpy (gameData.multigame.msg.buf + count, &OBJECTS [nObject].mType.physInfo.velocity, sizeof (CFixVector));
count += sizeof (CFixVector);
#else
vSwapped.dir.coord.x = (fix)INTEL_INT (int (OBJECTS [nObject].info.position.vPos.dir.coord.x));
vSwapped.dir.coord.y = (fix)INTEL_INT (int (OBJECTS [nObject].info.position.vPos.dir.coord.y));
vSwapped.dir.coord.z = (fix)INTEL_INT (int (OBJECTS [nObject].info.position.vPos.dir.coord.z));
memcpy (gameData.multigame.msg.buf + count, &vSwapped, 12);
count += 12;
vSwapped.dir.coord.x = (fix)INTEL_INT (int (OBJECTS [nObject].mType.physInfo.velocity.dir.coord.x));
vSwapped.dir.coord.y = (fix)INTEL_INT (int (OBJECTS [nObject].mType.physInfo.velocity.dir.coord.y));
vSwapped.dir.coord.z = (fix)INTEL_INT (int (OBJECTS [nObject].mType.physInfo.velocity.dir.coord.z));
memcpy (gameData.multigame.msg.buf + count, &vSwapped, 12);
count += 12;
#endif
MultiSendData (gameData.multigame.msg.buf, count, 1);
NetworkResetObjSync (nObject);
MapObjnumLocalToLocal (nObject);
}

//-----------------------------------------------------------------------------

void MultiSendCreatePowerup (int powerupType, int nSegment, int nObject, const CFixVector *vPos)
{
	// Create a powerup on a remote machine, used for remote
	// placement of used powerups like missiles and cloaking
	// powerups.

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	CFixVector vSwapped;
#endif
	int count = 0;

gameData.multigame.msg.buf [count++] = MULTI_CREATE_POWERUP;
gameData.multigame.msg.buf [count++] = N_LOCALPLAYER;
gameData.multigame.msg.buf [count++] = powerupType;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + count, nSegment);
count += 2;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + count, nObject);
count += 2;
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
memcpy (gameData.multigame.msg.buf + count, vPos, sizeof (CFixVector));
count += sizeof (CFixVector);
#else
vSwapped.dir.coord.x = (fix)INTEL_INT ((int) (*vPos).dir.coord.x);
vSwapped.dir.coord.y = (fix)INTEL_INT ((int) (*vPos).dir.coord.y);
vSwapped.dir.coord.z = (fix)INTEL_INT ((int) (*vPos).dir.coord.z);
memcpy (gameData.multigame.msg.buf + count, &vSwapped, 12);
count += 12;
#endif
MultiSendData (gameData.multigame.msg.buf, count, 1);
NetworkResetObjSync (nObject);
MapObjnumLocalToLocal (nObject);
}

//-----------------------------------------------------------------------------

void MultiSendDropPowerup (int powerupType, int nSegment, int nObject, const CFixVector *vPos, const CFixVector *vVel)
{
	// Create a powerup on a remote machine, used for remote
	// placement of used powerups like missiles and cloaking
	// powerups.

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	CFixVector vSwapped;
#endif
	int count = 0;

gameData.multigame.msg.buf [count++] = MULTI_DROP_POWERUP;
gameData.multigame.msg.buf [count++] = N_LOCALPLAYER;
gameData.multigame.msg.buf [count++] = powerupType;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + count, nSegment);
count += 2;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + count, nObject);
count += 2;
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
memcpy (gameData.multigame.msg.buf + count, vPos, sizeof (CFixVector));
count += sizeof (CFixVector);
memcpy (gameData.multigame.msg.buf + count, vVel, sizeof (CFixVector));
count += sizeof (CFixVector);
#else
vSwapped.dir.coord.x = (fix)INTEL_INT ((int) (*vPos).dir.coord.x);
vSwapped.dir.coord.y = (fix)INTEL_INT ((int) (*vPos).dir.coord.y);
vSwapped.dir.coord.z = (fix)INTEL_INT ((int) (*vPos).dir.coord.z);
memcpy (gameData.multigame.msg.buf + count, &vSwapped, 12);
count += 12;
vSwapped.dir.coord.x = (fix)INTEL_INT ((int) (*vVel).dir.coord.x);
vSwapped.dir.coord.y = (fix)INTEL_INT ((int) (*vVel).dir.coord.y);
vSwapped.dir.coord.z = (fix)INTEL_INT ((int) (*vVel).dir.coord.z);
memcpy (gameData.multigame.msg.buf + count, &vSwapped, 12);
count += 12;
#endif
MultiSendData (gameData.multigame.msg.buf, count, 1);
NetworkResetObjSync (nObject);
MapObjnumLocalToLocal (nObject);
}

//-----------------------------------------------------------------------------

void MultiSendPlaySound (int nSound, fix volume)
{
gameData.multigame.msg.buf [0] = MULTI_PLAY_SOUND;
gameData.multigame.msg.buf [1] = N_LOCALPLAYER;
gameData.multigame.msg.buf [2] = char (nSound);
gameData.multigame.msg.buf [3] = char (volume >> 12);
MultiSendData (gameData.multigame.msg.buf, 4, 0);
}

//-----------------------------------------------------------------------------

void MultiSendAudioTaunt (int taunt_num)
{
	return; // Taken out, awaiting sounds..

#if 0
	int audio_taunts [4] = {
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
	int count = 0;

if (IsCoopGame) {
	MultiSortKillList ();
	gameData.multigame.msg.buf [count++] = MULTI_SCORE;
	gameData.multigame.msg.buf [count++] = N_LOCALPLAYER;
	PUT_INTEL_INT (gameData.multigame.msg.buf + count, LOCALPLAYER.score);
	count += 4;
	MultiSendData (gameData.multigame.msg.buf, count, 0);
	}
}

//-----------------------------------------------------------------------------

void MultiSendSaveGame (ubyte slot, uint id, char * desc)
{
	int count = 0;

gameData.multigame.msg.buf [count++] = MULTI_SAVE_GAME;
gameData.multigame.msg.buf [count++] = slot;
PUT_INTEL_INT (gameData.multigame.msg.buf + count, id);
count += 4;             // Save id
memcpy (&gameData.multigame.msg.buf [count], desc, 20);
count += 20;
MultiSendData (gameData.multigame.msg.buf, count, 2);
}

//-----------------------------------------------------------------------------

void MultiSendRestoreGame (ubyte slot, uint id)
{
	int count = 0;

gameData.multigame.msg.buf [count++] = MULTI_RESTORE_GAME;
gameData.multigame.msg.buf [count++] = slot;
PUT_INTEL_INT (gameData.multigame.msg.buf + count, id);
count += 4;             // Save id
MultiSendData (gameData.multigame.msg.buf, count, 2);
}

//-----------------------------------------------------------------------------

void MultiSendNetPlayerStatsRequest (ubyte player_num)
{
	int count = 0;

gameData.multigame.msg.buf [count] = MULTI_REQ_PLAYER;
count += 1;
gameData.multigame.msg.buf [count] = player_num;
count += 1;
MultiSendData (gameData.multigame.msg.buf, count, 0);
}

//-----------------------------------------------------------------------------

void MultiSendTrigger (int nTrigger, int nObject)
{
	// Send an even to CTrigger something in the mine

	int count = 1;

gameData.multigame.msg.buf [count] = N_LOCALPLAYER;
count += 1;
gameData.multigame.msg.buf [count] = (ubyte)nTrigger;
count += 1;
if (gameStates.multi.nGameType == UDP_GAME) {
	gameData.multigame.msg.buf [0] = MULTI_TRIGGER_EXT;
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + count, nObject);
	count += 2;
	}
else
	gameData.multigame.msg.buf [0] = MULTI_TRIGGER;
MultiSendData (gameData.multigame.msg.buf, count, 1);
//MultiSendData (gameData.multigame.msg.buf, count, 1); // twice?
}

//-----------------------------------------------------------------------------

void MultiSendObjTrigger (int nTrigger)
{
	// Send an even to CTrigger something in the mine

	int count = 0;

gameData.multigame.msg.buf [count] = MULTI_OBJ_TRIGGER;
count += 1;
gameData.multigame.msg.buf [count] = N_LOCALPLAYER;
count += 1;
gameData.multigame.msg.buf [count] = (ubyte)nTrigger;
count += 1;
MultiSendData (gameData.multigame.msg.buf, count, 1);
//MultiSendData (gameData.multigame.msg.buf, count, 1); // twice?
}

//-----------------------------------------------------------------------------

void MultiSendHostageDoorStatus (int wallnum)
{
	// Tell the other player what the hit point status of a hostage door
	// should be
	int count = 0;

gameData.multigame.msg.buf [count] = MULTI_HOSTAGE_DOOR;
count += 1;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + count, wallnum);
count += 2;
PUT_INTEL_INT (gameData.multigame.msg.buf + count, WALLS [wallnum].hps);
count += 4;
MultiSendData (gameData.multigame.msg.buf, count, 0);
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

	int		i, nObject;
	int		cloakCount, invulCount;
	CObject	*objP;

gameData.score.nHighscore = 0;
gameData.score.nChampion = -1;
gameStates.render.bDropAfterburnerBlob = 0;
networkData.nConsistencyErrorCount = 0;
memset (gameData.multigame.score.pFlags, 0, MAX_NUM_NET_PLAYERS * sizeof (gameData.multigame.score.pFlags [0]));
for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++) {
	objP = OBJECTS + gameData.multiplayer.players [i].nObject;
	if (i != N_LOCALPLAYER)
		objP->info.controlType = CT_REMOTE;
	objP->info.movementType = MT_PHYSICS;
	MultiResetPlayerObject (objP);
	networkData.nLastPacketTime [i] = 0;
	}
for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++) {
	gameData.multigame.robots.controlled [i] = -1;
	gameData.multigame.robots.agitation [i] = 0;
	gameData.multigame.robots.fired [i] = 0;
	}
gameData.objs.viewerP = gameData.objs.consoleP = OBJECTS + LOCALPLAYER.nObject;
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
FORALL_STATIC_OBJS (objP, i) {
	if (objP->info.nType == OBJ_HOSTAGE) {
		if (!IsCoopGame) {
			nObject = CreatePowerup (POW_SHIELD_BOOST, -1, objP->info.nSegment, objP->info.position.vPos, 1);
			ReleaseObject (objP->Index ());
			if (nObject != -1) {
				CObject	*objP = OBJECTS + nObject;
				objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [POW_SHIELD_BOOST].nClipIndex;
				objP->rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][objP->rType.vClipInfo.nClipIndex].xFrameTime;
				objP->rType.vClipInfo.nCurFrame = 0;
				objP->mType.physInfo.drag = 512;     //1024;
				objP->mType.physInfo.mass = I2X (1);
				objP->mType.physInfo.velocity.SetZero ();
				}
			}
		}
	else if (objP->info.nType == OBJ_POWERUP) {
		switch (objP->info.nId) {
			case POW_EXTRA_LIFE:
				if (netGame.m_info.DoInvulnerability)
					objP->Bash (POW_INVUL);
				else
					objP->BashToShield (true);
				break;

			case POW_INVUL:
				if ((invulCount < 3) && netGame.m_info.DoInvulnerability)
					invulCount++;
				else
					objP->BashToShield (true);
				break;

			case POW_CLOAK:
				if ((cloakCount < 3) && netGame.m_info.DoCloak)
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
				objP->BashToShield (!netGame.m_info.DoAfterburner);
				break;
			case POW_FUSION:
				objP->BashToShield (!netGame.m_info.DoFusions);
				break;
			case POW_PHOENIX:
				objP->BashToShield (!netGame.m_info.DoPhoenix);
				break;
			case POW_HELIX:
				objP->BashToShield (!netGame.m_info.DoHelix);
				break;
			case POW_MEGAMSL:
				objP->BashToShield (!netGame.m_info.DoMegas);
				break;
			case POW_SMARTMSL:
				objP->BashToShield (!netGame.m_info.DoSmarts);
				break;
			case POW_GAUSS:
				objP->BashToShield (!netGame.m_info.DoGauss);
				break;
			case POW_VULCAN:
				objP->BashToShield (!netGame.m_info.DoVulcan);
				break;
			case POW_PLASMA:
				objP->BashToShield (!netGame.m_info.DoPlasma);
				break;
			case POW_OMEGA:
				objP->BashToShield (!netGame.m_info.DoOmega);
				break;
			case POW_SUPERLASER:
				objP->BashToShield (!netGame.m_info.DoSuperLaser);
				break;
			case POW_PROXMINE:
				objP->BashToShield (!netGame.m_info.DoProximity || (gameData.app.nGameMode &(GM_HOARD | GM_ENTROPY)));
				break;
			case POW_SMARTMINE:
				objP->BashToShield (!netGame.m_info.DoSmartMine || IsEntropyGame);
				break;
			case POW_VULCAN_AMMO:
				objP->BashToShield (!(netGame.m_info.DoVulcan || netGame.m_info.DoGauss));
				break;
			case POW_SPREADFIRE:
				objP->BashToShield (!netGame.m_info.DoSpread);
				break;
			case POW_FLASHMSL_1:
				objP->BashToShield (!netGame.m_info.DoFlash);
				break;
			case POW_FLASHMSL_4:
				objP->BashToShield (!netGame.m_info.DoFlash);
				break;
			case POW_GUIDEDMSL_1:
				objP->BashToShield (!netGame.m_info.DoGuided);
				break;
			case POW_GUIDEDMSL_4:
				objP->BashToShield (!netGame.m_info.DoGuided);
				break;
			case POW_EARTHSHAKER:
				objP->BashToShield (!netGame.m_info.DoEarthShaker);
				break;
			case POW_MERCURYMSL_1:
				objP->BashToShield (!netGame.m_info.DoMercury);
				break;
			case POW_MERCURYMSL_4:
				objP->BashToShield (!netGame.m_info.DoMercury);
				break;
			case POW_CONVERTER:
				objP->BashToShield (!netGame.m_info.DoConverter);
				break;
			case POW_AMMORACK:
				objP->BashToShield (!netGame.m_info.DoAmmoRack);
				break;
			case POW_HEADLIGHT:
				 objP->BashToShield (!netGame.m_info.DoHeadlight || (EGI_FLAG (bDarkness, 0, 0, 0) && !EGI_FLAG (headlight.bAvailable, 0, 1, 0)));
				break;
			case POW_LASER:
				objP->BashToShield (!netGame.m_info.DoLaserUpgrade);
				break;
			case POW_HOMINGMSL_1:
				objP->BashToShield (!netGame.m_info.DoHoming);
				break;
			case POW_HOMINGMSL_4:
				objP->BashToShield (!netGame.m_info.DoHoming);
				break;
			case POW_QUADLASER:
				objP->BashToShield (!netGame.m_info.DoQuadLasers);
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
gameData.objs.consoleP->info.controlType = CT_FLYING;
ResetPlayerObject ();
}

//-----------------------------------------------------------------------------

int MultiFindGoalTexture (short t)
{
	int i;

if (t == TMI_FUELCEN)
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

void CSegment::OverrideTextures (short nTexture, short nOldTexture, short nTexture2, int bFullBright, int bForce)
{
	int j, v;

nTexture = (nTexture < 0) ? -nTexture : MultiFindGoalTexture (nTexture);
nOldTexture = (nOldTexture < 0) ? -nOldTexture : MultiFindGoalTexture (nOldTexture);
if (nTexture >- 1)
	for (j = 0; j < 6; j++) {
		if (bForce || (m_sides [j].m_nBaseTex == nOldTexture)) {
			m_sides [j].m_nBaseTex = nTexture;
			if ((extraGameInfo [1].entropy.nOverrideTextures == 1) &&
				 (m_sides [j].m_nOvlTex > 0) &&(nTexture2 > 0))
				m_sides [j].m_nOvlTex = nTexture2;
			if ((extraGameInfo [1].entropy.nOverrideTextures == 1) && bFullBright)
				for (v = 0; v < 4; v++)
					m_sides [j].m_uvls [v].l = I2X (100);		//max out
			}
		}
if (bFullBright)
	m_xAvgSegLight = I2X (100);	//make static light bright
}

//-----------------------------------------------------------------------------

#define	TMI_RED_TEAM	TMI_GOAL_RED	//-313
#define	TMI_BLUE_TEAM	TMI_GOAL_BLUE	//-333

int nBlueGoalSegment, nRedGoalSegment;

void CSegment::ChangeTexture (int oldOwner)
{
	int		bFullBright = (IsHoardGame != 0) || (IsEntropyGame && extraGameInfo [1].entropy.bBrightenRooms);
	static	short texOverrides [3] = {-313, TMI_BLUE_TEAM, TMI_RED_TEAM};

//if (oldOwner < 0)
//	oldOwner = nOwner;
if (IsEntropyGame &&(extraGameInfo [1].entropy.nOverrideTextures == 2))
	return;
switch (m_function) {
	case SEGMENT_FUNC_GOAL_BLUE:
		nBlueGoalSegment = Index ();
		OverrideTextures ((short) (IsHoardGame ? TMI_GOAL_HOARD : TMI_GOAL_BLUE), -1, -1, bFullBright, 1);
		break;

	case SEGMENT_FUNC_GOAL_RED:
		nRedGoalSegment = Index ();
		// Make both textures the same if Hoard mode
		OverrideTextures ((short) (IsHoardGame ? TMI_GOAL_HOARD : TMI_GOAL_RED), -1, -1, bFullBright, 1);
		break;

	case SEGMENT_FUNC_ROBOTMAKER:
		if (IsEntropyGame &&(m_owner >= 0))
			OverrideTextures (texOverrides [(int) m_owner],
									 (short) ((oldOwner < 0) ? -1 : texOverrides [oldOwner]), 316, bFullBright, oldOwner < 0);
		break;

	case SEGMENT_FUNC_REPAIRCEN:
		if (IsEntropyGame &&(m_owner >= 0))
			OverrideTextures (texOverrides [(int) m_owner],
									 (short) ((oldOwner < 0) ? -1 : texOverrides [oldOwner]), 315, bFullBright, oldOwner < 0);
		break;

	case SEGMENT_FUNC_FUELCEN:
		if (IsEntropyGame &&(m_owner >= 0))
			OverrideTextures (texOverrides [(int) m_owner],
								   (short) ((oldOwner < 0) ? -1 : texOverrides [oldOwner]), 314, bFullBright, oldOwner < 0);
		break;

	default:
		if (IsEntropyGame &&(m_owner >= 0))
			OverrideTextures (texOverrides [(int) m_owner],
									 (short) ((oldOwner < 0) ? -1 : texOverrides [oldOwner]), -1, bFullBright, oldOwner < 0);
	}
}

//-----------------------------------------------------------------------------

void MultiApplyGoalTextures ()
{
	int		i;

if (!IsEntropyGame || (extraGameInfo [1].entropy.nOverrideTextures == 1))
	for (i = 0; i <= gameData.segs.nLastSegment; i++)
		SEGMENTS [i].ChangeTexture (-1);
}

//-----------------------------------------------------------------------------

void MultiSetRobotAI (void)
{
	// Go through the objects array looking for robots and setting
	// them to certain supported types of NET AI behavior.

	//      int i;
	//
	//      for (i = 0; i <= gameData.objs.nLastObject [0]; i++)
	// {
	//              if (OBJECTS [i].info.nType == OBJ_ROBOT) {
	//                      OBJECTS [i].aiInfo.REMOTE_OWNER = -1;
	//                      if (OBJECTS [i].aiInfo.behavior == AIB_STATION)
	//                              OBJECTS [i].aiInfo.behavior = AIB_NORMAL;
	//              }
	//      }
}

//-----------------------------------------------------------------------------

int MultiDeleteExtraObjects (void)
{
	int 		nType, nnp = 0;
	CObject *objP;

// Go through the CObject list and remove any objects not used in
// 'Anarchy!' games.
// This function also prints the total number of available multiplayer
// positions in this level, even though this should always be 8 or more!

FORALL_OBJS (objP, i) {
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
				ObjectCreateEgg (objP);
		objP->Die ();
		}
	}
return nnp;
}

//-----------------------------------------------------------------------------

void ChangePlayerNumTo (int nLocalPlayer)
{
if ((N_LOCALPLAYER > -1) &&(N_LOCALPLAYER != nLocalPlayer))
	memcpy (gameData.multiplayer.players [nLocalPlayer].callsign, LOCALPLAYER.callsign, CALLSIGN_LEN + 1);
N_LOCALPLAYER = nLocalPlayer;
}

//-----------------------------------------------------------------------------

int MultiAllPlayersAlive (void)
{
	int i;

for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	if (gameData.multigame.score.pFlags [i] && gameData.multiplayer.players [i].Connected ())
		return 0;
return 1;
}

//-----------------------------------------------------------------------------

void MultiInitiateSaveGame (int bSecret)
{
	uint	gameId;
	int	i;
	char	slot;

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
gameId ^= gameData.multiplayer.nPlayers << 4;
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	gameId ^= *((uint*) &gameData.multiplayer.players [i].callsign [0]);
if (gameId == 0)
	gameId = 1; // 0 is invalid
if (slot >= 0)
	MultiSendSaveGame (slot, gameId, saveGameManager.Description ());
MultiDoFrame ();
MultiSaveGame (slot, gameId, saveGameManager.Description ());
}

//-----------------------------------------------------------------------------

void MultiInitiateRestoreGame (int bSecret)
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

void MultiSaveGame (char slot, uint id, char *description)
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

void MultiRestoreGame (char slot, uint id)
{
	char			szFile [FILENAME_LEN];
	//CPlayerInfo	savedPlayer = LOCALPLAYER;
	int			i;

#if !DBG
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed)
	return;
#endif
if (slot < 0)
	missionManager.LevelStateName (szFile, -slot);
else
	sprintf (szFile, "%s.mg%d", LOCALPLAYER.callsign, slot);
gameData.app.bGamePaused = 1;
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	MultiStripRobots (i);
if ((uint) saveGameManager.GetGameId (szFile, (slot < 0) ? -1 : 0) != id) {
	MultiBadRestore ();
	gameData.app.bGamePaused = 0;
	return;
	}
int bGameRunning = gameStates.app.bGameRunning;
saveGameManager.LoadState (1, (slot < 0) ? -1 : 0, szFile);
gameStates.app.bGameRunning = bGameRunning;
ogl.RebuildContext (1);
gameData.app.bGamePaused = 0;
}

//-----------------------------------------------------------------------------

void ExtractNetPlayerStats (tNetPlayerStats *ps, CPlayerInfo * pd)
{
	int i;

ps->flags = INTEL_INT (pd->flags);                                   // Powerup flags, see below...
ps->energy = (fix)INTEL_INT (pd->energy);                            // Amount of energy remaining.
ps->shield = (fix)INTEL_INT (pd->shield);                          // shield remaining (protection)
ps->lives = pd->lives;                                              // Lives remaining, 0 = game over.
ps->laserLevel = pd->laserLevel;                                  // Current level of the laser.
ps->primaryWeaponFlags = (ubyte) pd->primaryWeaponFlags;                  // bit set indicates the player has this weapon.
ps->secondaryWeaponFlags = (ubyte) pd->secondaryWeaponFlags;              // bit set indicates the player has this weapon.
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	ps->primaryAmmo [i] = INTEL_SHORT (pd->primaryAmmo [i]);
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
	ps->secondaryAmmo [i] = INTEL_SHORT (pd->secondaryAmmo [i]);
//memcpy (ps->primaryAmmo, pd->primaryAmmo, MAX_PRIMARY_WEAPONS*sizeof (short));        // How much ammo of each nType.
//memcpy (ps->secondaryAmmo, pd->secondaryAmmo, MAX_SECONDARY_WEAPONS*sizeof (short)); // How much ammo of each nType.
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
	int i;

ps->flags = INTEL_INT (pd->flags);                       // Powerup flags, see below...
ps->energy = (fix)INTEL_INT ((int)pd->energy);           // Amount of energy remaining.
ps->shield = (fix)INTEL_INT ((int)pd->shield);         // shield remaining (protection)
ps->lives = pd->lives;                                  // Lives remaining, 0 = game over.
ps->laserLevel = pd->laserLevel;                      // Current level of the laser.
ps->primaryWeaponFlags = pd->primaryWeaponFlags;      // bit set indicates the player has this weapon.
ps->secondaryWeaponFlags = pd->secondaryWeaponFlags;  // bit set indicates the player has this weapon.
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	ps->primaryAmmo [i] = INTEL_SHORT (pd->primaryAmmo [i]);
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
	ps->secondaryAmmo [i] = INTEL_SHORT (pd->secondaryAmmo [i]);
//memcpy (ps->primaryAmmo, pd->primaryAmmo, MAX_PRIMARY_WEAPONS*sizeof (short));  // How much ammo of each nType.
//memcpy (ps->secondaryAmmo, pd->secondaryAmmo, MAX_SECONDARY_WEAPONS*sizeof (short)); // How much ammo of each nType.
ps->lastScore = INTEL_INT (pd->lastScore);             // Score at beginning of current level.
ps->score = INTEL_INT (pd->score);                       // Current score.
ps->cloakTime = (fix)INTEL_INT ((int)pd->cloakTime);   // Time cloaked
ps->homingObjectDist = (fix)INTEL_INT ((int)pd->homingObjectDist); // Distance of nearest homing CObject.
ps->invulnerableTime = (fix)INTEL_INT ((int)pd->invulnerableTime); // Time invulnerable
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

void MultiSendDropWeapon (int nObject)
{
	CObject *objP;
	int count = 0;
	int ammoCount;

if (nObject < 0)
	return;
objP = OBJECTS + nObject;
ammoCount = objP->cType.powerupInfo.nCount;
if (objP->info.nId == POW_OMEGA && ammoCount == I2X (1))
	ammoCount = I2X (1) - 1; //make fit in short
gameData.multigame.msg.buf [count++] = (char)MULTI_DROP_WEAPON;
gameData.multigame.msg.buf [count++] = (char)objP->info.nId;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + count, N_LOCALPLAYER);
count += 2;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + count, nObject);
count += 2;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + count, ammoCount);
count += 2;
PUT_INTEL_INT (gameData.multigame.msg.buf + count, gameStates.app.nRandSeed);
MapObjnumLocalToLocal (nObject);
MultiSendData (gameData.multigame.msg.buf, 12, 2);
}

//-----------------------------------------------------------------------------

void MultiDoDropWeapon (char *buf)
{
	int 		nPlayer, ammo, nObject, nRemoteObj, seed;
	CObject	*objP;
	ubyte		powerupId;

powerupId = (ubyte) (buf [1]);
nPlayer = GET_INTEL_SHORT (buf + 2);
nRemoteObj = GET_INTEL_SHORT (buf + 4);
ammo = GET_INTEL_SHORT (buf + 6);
seed = GET_INTEL_INT (buf + 8);
objP = OBJECTS + gameData.multiplayer.players [nPlayer].nObject;
nObject = SpitPowerup (objP, powerupId, seed);
if (nObject >= 0) {
	MapObjnumLocalToRemote (nObject, nRemoteObj, nPlayer);
	OBJECTS [nObject].cType.powerupInfo.nCount = ammo;
	}
}

//-----------------------------------------------------------------------------

void MultiSendGuidedInfo (CObject *miss, char done)
{
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tShortPos sp;
#endif
	int count = 0;

gameData.multigame.msg.buf [count++] = (char)MULTI_GUIDED;
gameData.multigame.msg.buf [count++] = (char)N_LOCALPLAYER;
gameData.multigame.msg.buf [count++] = done;
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
CreateShortPos (reinterpret_cast<tShortPos*> (gameData.multigame.msg.buf + count), miss, 0);
count += sizeof (tShortPos);
#else
CreateShortPos (&sp, miss, 1);
memcpy (&(gameData.multigame.msg.buf [count]), reinterpret_cast<ubyte*> (sp.orient), 9);
count += 9;
memcpy (&(gameData.multigame.msg.buf [count]), reinterpret_cast<ubyte*> (&sp.coord), 14);
count += 14;
#endif
MultiSendData (gameData.multigame.msg.buf, count, 0);
}

//-----------------------------------------------------------------------------

void MultiDoGuided (char *buf)
{
	int nPlayer = int (buf [1]);
	int count = 3;
	static int fun = 200;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tShortPos sp;
#endif
	CObject *gmObjP = gameData.objs.guidedMissile [nPlayer].objP;

if (gmObjP == NULL) {
	if (++fun >= 50)
		fun = 0;
	return;
	}
else if (++fun >= 50)
	fun = 0;
	if (buf [2]) {
		ReleaseGuidedMissile (nPlayer);
		return;
		}
	if ((gmObjP < OBJECTS.Buffer ()) ||
		 (gmObjP - OBJECTS > gameData.objs.nLastObject [0])) {
		Int3 ();  // Get Jason immediately!
		return;
		}
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
ExtractShortPos (gmObjP, reinterpret_cast<tShortPos*> (buf + count), 0);
#else
memcpy (reinterpret_cast<ubyte*>(sp.orient), reinterpret_cast<ubyte*>(buf + count), 9);
memcpy (reinterpret_cast<ubyte*> (&(sp.coord)), reinterpret_cast<ubyte*> (buf + count + 9), 14);
ExtractShortPos (gmObjP, &sp, 1);
#endif
count += sizeof (tShortPos);
UpdateObjectSeg (gmObjP);
}

//-----------------------------------------------------------------------------

void MultiSendStolenItems ()
{
	int i;

gameData.multigame.msg.buf [0] = MULTI_STOLEN_ITEMS;
for (i = 0; i < MAX_STOLEN_ITEMS; i++)
	gameData.multigame.msg.buf [i+1] = gameData.thief.stolenItems [i];
MultiSendData (gameData.multigame.msg.buf, MAX_STOLEN_ITEMS + 1, 1);
}

//-----------------------------------------------------------------------------

void MultiDoStolenItems (char *buf)
{
	int i;

for (i = 0; i < MAX_STOLEN_ITEMS; i++)
	gameData.thief.stolenItems [i] = buf [i+1];
}

//-----------------------------------------------------------------------------

void MultiSendWallStatus (int wallnum, ubyte nType, ushort flags, ubyte state)
{
	int count = 0;

gameData.multigame.msg.buf [count++] = MULTI_WALL_STATUS;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + count, wallnum);
count += 2;
gameData.multigame.msg.buf [count++] = nType;
gameData.multigame.msg.buf [count++] = ubyte (flags);
gameData.multigame.msg.buf [count++] = state;
MultiSendData (gameData.multigame.msg.buf, count, 1); // twice, just to be sure
MultiSendData (gameData.multigame.msg.buf, count, 1);
}

//-----------------------------------------------------------------------------

void MultiSendWallStatusSpecific (int nPlayer, int wallnum, ubyte nType, ushort flags, ubyte state)
{
	// Send wall states a specific rejoining player
	short count = 0;

gameData.multigame.msg.buf [count++] = MULTI_WALL_STATUS;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + count, wallnum);
count += 2;
gameData.multigame.msg.buf [count++] = nType;
gameData.multigame.msg.buf [count++] = ubyte (flags);
gameData.multigame.msg.buf [count++] = state;
NetworkSendNakedPacket (gameData.multigame.msg.buf, count, nPlayer); // twice, just to be sure
NetworkSendNakedPacket (gameData.multigame.msg.buf, count, nPlayer);
}

//-----------------------------------------------------------------------------

void MultiDoWallStatus (char *buf)
{
	short wallnum;
	ubyte flag, nType, state;

	wallnum = GET_INTEL_SHORT (buf + 1);
	nType = buf [3];
	flag = buf [4];
	state = buf [5];

WALLS [wallnum].nType = nType;
WALLS [wallnum].flags = flag;
WALLS [wallnum].state = state;
if (WALLS [wallnum].nType == WALL_OPEN)
	audio.DestroySegmentSound (
		 (short) WALLS [wallnum].nSegment, (short)
		 WALLS [wallnum].nSide, SOUND_FORCEFIELD_HUM);
}

//-----------------------------------------------------------------------------

void MultiSendJasonCheat (int num)
{
return;
}

//-----------------------------------------------------------------------------

void MultiSendScoreGoalCounts ()
{
	int h, i, count = 1;
	gameData.multigame.msg.buf [0] = MULTI_SCOREGOALS;

h = (gameStates.multi.nGameType == UDP_GAME) ? MAX_PLAYERS_D2X : MAX_PLAYERS_D2;
for (i = 0; i < h; i++)
		gameData.multigame.msg.buf [count++] = (char)gameData.multiplayer.players [i].nScoreGoalCount;
MultiSendData (gameData.multigame.msg.buf, count, 1);
}

//-----------------------------------------------------------------------------

void MultiDoScoreGoalCounts (char *buf)
{
	int h, i, count = 1;

h = (gameStates.multi.nGameType == UDP_GAME) ? MAX_PLAYERS_D2X : MAX_PLAYERS_D2;
for (i = 0; i < h; i++)
	gameData.multiplayer.players [i].nScoreGoalCount = buf [count++];
}

//-----------------------------------------------------------------------------

void MultiSendHeartBeat ()
{
if (netGame.GetPlayTimeAllowed ()) {
	gameData.multigame.msg.buf [0] = MULTI_HEARTBEAT;
	PUT_INTEL_INT (gameData.multigame.msg.buf+1, gameStates.app.xThisLevelTime);
	MultiSendData (gameData.multigame.msg.buf, 5, 0);
	}
}

//-----------------------------------------------------------------------------

void MultiDoHeartBeat (char *buf)
{
fix num = GET_INTEL_INT (buf + 1);
gameStates.app.xThisLevelTime = num;
}

//-----------------------------------------------------------------------------

void MultiCheckForEntropyWinner ()
{
#if 1//!DBG
	CSegment*	segP;
	int			h, i, t;
	char			bGotRoom [2] = {0, 0};

	static int		countDown;

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
for (i = 0, segP = SEGMENTS.Buffer (); i <= gameData.segs.nLastSegment; i++, segP++)
	if ((t = (int) segP->m_owner) > 0) {
		bGotRoom [--t] = 1;
		if (bGotRoom [!t])
			return;
	}
for (t = 0; t < 2; t++)
	if (bGotRoom [t])
		break;
if (t == 2)	// no team as at least one room -> this is probably not an entropy enaszPlayerd level
	return;
for (h = i = 0; i < gameData.multiplayer.nPlayers; i++)
	if (GetTeam (i) != t) {
		if (gameData.multiplayer.players [i].secondaryAmmo [PROXMINE_INDEX]  >= extraGameInfo [1].entropy.nCaptureVirusThreshold)
			return;
		h += gameData.multiplayer.players [i].secondaryAmmo [PROXMINE_INDEX];
		}
if ((h  >= extraGameInfo [1].entropy.nCaptureVirusThreshold) && extraGameInfo [1].entropy.nVirusStability)
	return;
HUDInitMessage (TXT_WINNING_TEAM, t ? TXT_RED : TXT_BLUE);
for (i = 0, segP = SEGMENTS.Buffer (); i <= gameData.segs.nLastSegment; i++, segP++) {
	if (segP->m_owner != t + 1)
		segP->m_owner = t + 1;
	segP->ChangeTexture (-1);
	}
gameStates.entropy.bExitSequence = 1;
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	if ((GetTeam (i) != t) && (gameData.multiplayer.players [i].Shield () >= 0))
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
	ubyte		bHaveObject [MAX_PLAYERS];
	CObject*	objP;
//	int		i;

memset (bHaveObject, 0, sizeof (bHaveObject));
FORALL_PLAYER_OBJS (objP, i) {
	if (objP->info.nType != OBJ_PLAYER)
		continue;
	if (bHaveObject [objP->info.nId] || (objP->info.nId >= gameData.multiplayer.nPlayers))
		objP->Die ();
	else {
		if (!gameData.multiplayer.players [objP->info.nId].Connected ()) {
			objP->SetType (OBJ_GHOST);
			objP->info.renderType = RT_NONE;
			objP->info.movementType = MT_NONE;
			}
		bHaveObject [objP->info.nId] = 1;
		}
	}
}

//-----------------------------------------------------------------------------

void MultiCheckForScoreGoalWinner (bool bForce)
{
	int h = 0, nPlayer = -1;

if (gameData.reactor.bDestroyed < 0)
	return;
for (int i = 0; i < gameData.multiplayer.nPlayers; i++)
	if (h < gameData.multiplayer.players [i].nScoreGoalCount) {
		h = gameData.multiplayer.players [i].nScoreGoalCount;
		nPlayer = i;
		}
CheckScoreGoal (nPlayer, bForce);
}

//-----------------------------------------------------------------------------

void MultiSendSeismic (fix start, fix end)
{
gameData.multigame.msg.buf [0] = MULTI_SEISMIC;
PUT_INTEL_INT (gameData.multigame.msg.buf + 1, start);
PUT_INTEL_INT (gameData.multigame.msg.buf + 1 + sizeof (fix), end);
MultiSendData (gameData.multigame.msg.buf, 1 + 2 * sizeof (fix), 1);
}

//-----------------------------------------------------------------------------

void MultiDoSeismic (char *buf)
{
gameStates.gameplay.seismic.nStartTime = GET_INTEL_INT (buf + 1);
gameStates.gameplay.seismic.nEndTime = GET_INTEL_INT (buf + 5);
audio.PlaySound (SOUND_SEISMIC_DISTURBANCE_START);
}

//-----------------------------------------------------------------------------

void MultiSendLight (int nSegment, ubyte val)
{
int count = 1, i;
gameData.multigame.msg.buf [0] = MULTI_LIGHT;
PUT_INTEL_INT (gameData.multigame.msg.buf + count, nSegment);
count += sizeof (int);
gameData.multigame.msg.buf [count++] = val;
for (i = 0; i < 6; i++,  count += 2)
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + count, SEGMENTS [nSegment].m_sides [i].m_nOvlTex);
MultiSendData (gameData.multigame.msg.buf, count, 1);
}

//-----------------------------------------------------------------------------

void MultiSendLightSpecific (int nPlayer, int nSegment, ubyte val)
{
	short count = 1, i;

gameData.multigame.msg.buf [0] = MULTI_LIGHT;
PUT_INTEL_INT (gameData.multigame.msg.buf + count, nSegment);
count += sizeof (int);
gameData.multigame.msg.buf [count++] = val;
for (i = 0; i < 6; i++, count += 2)
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + count, SEGMENTS [nSegment].m_sides [i].m_nOvlTex);
NetworkSendNakedPacket (gameData.multigame.msg.buf, count, nPlayer);
}

//-----------------------------------------------------------------------------

void MultiDoLight (char *buf)
{
	ubyte sides = buf [5];
	short i, seg = GET_INTEL_INT (buf + 1);

buf += 6;
for (i = 0; i < 6; i++, buf += 2) {
	if ((sides &(1 << i))) {
		SubtractLight (seg, i);
		SEGMENTS [seg].m_sides [i].m_nOvlTex = GET_INTEL_SHORT (buf);
		}
	}
}

//-----------------------------------------------------------------------------

void MultiDoFlags (char *buf)
{
	int nPlayer = int (buf [1]);
	uint flags;

flags = GET_INTEL_INT (buf + 2);
if (nPlayer!=N_LOCALPLAYER)
	gameData.multiplayer.players [nPlayer].flags = flags;
}

//-----------------------------------------------------------------------------

void MultiSendFlags (char nPlayer)
{
gameData.multigame.msg.buf [0] = MULTI_FLAGS;
gameData.multigame.msg.buf [1] = nPlayer;
PUT_INTEL_INT (gameData.multigame.msg.buf + 2, gameData.multiplayer.players [int (nPlayer)].flags);
MultiSendData (gameData.multigame.msg.buf, 6, 1);
}

//-----------------------------------------------------------------------------

void MultiDoWeapons (char *buf)
{
	int	i, bufP = 1;
	int	nPlayer = int (buf [bufP++]);

gameData.multiplayer.players [nPlayer].SetShield (GET_INTEL_INT (buf + bufP));
bufP += 4;
gameData.multiplayer.players [nPlayer].primaryWeaponFlags = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++) {
	gameData.multiplayer.players [nPlayer].secondaryAmmo [i] = GET_INTEL_SHORT (buf + bufP);
	bufP += 2;
	}
gameData.multiplayer.players [nPlayer].SetLaserLevels (gameData.multigame.msg.buf [bufP++], gameData.multigame.msg.buf [bufP++]);
gameData.multiplayer.weaponStates [nPlayer].nAmmoUsed = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
gameData.multiplayer.weaponStates [nPlayer].nBuiltinMissiles = buf [bufP];
}

//-----------------------------------------------------------------------------

void MultiDoAmmo (char *buf)
{
	int	nPlayer = int (buf [1]);

gameData.multiplayer.players [int (nPlayer)].primaryAmmo [VULCAN_INDEX] = ushort (GET_INTEL_SHORT (buf + 2));
gameData.multiplayer.weaponStates [int (nPlayer)].nAmmoUsed = GET_INTEL_SHORT (buf + 4);
}

//-----------------------------------------------------------------------------

void MultiDoFusionCharge (char *buf)
{
gameData.multiplayer.weaponStates [int (buf [1])].xFusionCharge = GET_INTEL_INT (buf + 2);
}

//-----------------------------------------------------------------------------

void MultiSendWeapons (int bForce)
{
	int t = gameStates.app.nSDLTicks [0];

	static int nTimeout = 0;

if (!IsMultiGame || (gameStates.multi.nGameType != UDP_GAME))
	return;
if (bForce || (t - nTimeout > 1000)) {
		int i, bufP = 0;

	nTimeout = t;
	gameData.multigame.msg.buf [bufP++] = (char) MULTI_WEAPONS;
	gameData.multigame.msg.buf [bufP++] = (char) N_LOCALPLAYER;
	PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, LOCALPLAYER.Shield ());
	bufP += 4;
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, LOCALPLAYER.primaryWeaponFlags);
	bufP += 2;
	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++) {
		PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, LOCALPLAYER.secondaryAmmo [i]);
		bufP += 2;
		}
	gameData.multigame.msg.buf [bufP++] = LOCALPLAYER.LaserLevel (0);
	gameData.multigame.msg.buf [bufP++] = LOCALPLAYER.LaserLevel (1);
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, gameData.multiplayer.weaponStates [N_LOCALPLAYER].nAmmoUsed);
	bufP += 2;
	gameData.multigame.msg.buf [bufP++] = gameData.multiplayer.weaponStates [N_LOCALPLAYER].nBuiltinMissiles;
	MultiSendData (gameData.multigame.msg.buf, bufP, 1);
	MultiSendShield ();
	}
}

//-----------------------------------------------------------------------------

void MultiSendAmmo (void)
{
if (!IsMultiGame || (gameStates.multi.nGameType != UDP_GAME))
	return;

	int bufP = 0;

gameData.multigame.msg.buf [bufP++] = (char) MULTI_AMMO;
gameData.multigame.msg.buf [bufP++] = (char) N_LOCALPLAYER;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, short (LOCALPLAYER.primaryAmmo [VULCAN_INDEX]));
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

	int bufP = 0;

gameData.multigame.msg.buf [bufP++] = (char) MULTI_FUSION_CHARGE;
gameData.multigame.msg.buf [bufP++] = (char) N_LOCALPLAYER;
PUT_INTEL_INT (gameData.multigame.msg.buf + bufP, int (gameData.FusionCharge ()));
bufP += 4;
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
}

//-----------------------------------------------------------------------------

void MultiDoMonsterball (char *buf)
{
	int			bCreate, bufP = 1;
	short			nSegment;
	CFixVector	v;

bCreate = (int) buf [bufP++];
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

void MultiSendMonsterball (int bForce, int bCreate)
{
	int t = gameStates.app.nSDLTicks [0];

	static int nTimeout = 0;

if (!(gameData.app.GameMode (GM_MONSTERBALL)))
	return;
if (!gameData.hoard.monsterballP)
	return;
if (bForce || (t - nTimeout > 1000)) {
		int bufP = 0;

	nTimeout = t;
	gameData.multigame.msg.buf [bufP++] = (char) MULTI_MONSTERBALL;
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

void MultiSendDropBlobs (char nPlayer)
{
gameData.multigame.msg.buf [0] = MULTI_DROP_BLOB;
gameData.multigame.msg.buf [1] = nPlayer;
MultiSendData (gameData.multigame.msg.buf, 2, 0);
}

//-----------------------------------------------------------------------------

void MultiDoDropBlob (char *buf)
{
DropAfterburnerBlobs (&OBJECTS [gameData.multiplayer.players [int (buf [1])].nObject], 2, I2X (1), -1, NULL, 0);
}

//-----------------------------------------------------------------------------

void MultiSendPowerupUpdate (void)
{
if (IAmGameHost ()) {
	gameData.multigame.msg.buf [0] = MULTI_POWERUP_UPDATE;
	for (int i = 0, j = 1; i < MAX_POWERUP_TYPES; i++) {
		gameData.multigame.msg.buf [j++] = gameData.multiplayer.maxPowerupsAllowed [i];
		//gameData.multigame.msg.buf [j++] = gameData.multiplayer.powerupsInMine [i];
		}
	MultiSendData (gameData.multigame.msg.buf, /*2 **/ MAX_POWERUP_TYPES + 1, 1);
	}
}

//-----------------------------------------------------------------------------

void MultiDoPowerupUpdate (char *buf)
{
if (!IAmGameHost ()) {
	for (int i = 0, j = 1; i < MAX_POWERUP_TYPES; i++) {
		//if (buf [i+1] > gameData.multiplayer.maxPowerupsAllowed [i])
			{
			gameData.multiplayer.maxPowerupsAllowed [i] = buf [j++];
			//gameData.multiplayer.powerupsInMine [i] = buf [j++];
			}
		}
	}
}

//-----------------------------------------------------------------------------

#if 0 // never used...
void MultiSendActiveDoor (int i)
{
	int count;

gameData.multigame.msg.buf [0] = MULTI_ACTIVE_DOOR;
gameData.multigame.msg.buf [1] = i;
gameData.multigame.msg.buf [2] = gameData.walls.nOpenDoors;
count = 3;
memcpy (gameData.multigame.msg.buf + 3, gameData.walls.activeDoors + i, sizeof (CActiveDoor);
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

void MultiDoActiveDoor (char *buf)
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
	gameData.walls.activeDoors.Push (*doorP);
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

void MultiDoSoundFunction (char *buf)
{
// for afterburner
if (LOCALPLAYER.Connected () != 1)
	return;
int nPlayer = int (buf [1]);
int nFunction = int (buf [2]);
if (nFunction == 0)
	audio.DestroyObjectSound (gameData.multiplayer.players [nPlayer].nObject);
else if (nFunction == 3) {
	short nSound = short (ubyte (buf [3]));
	if (nSound == SOUND_AFTERBURNER_PLAY)
		audio.CreateObjectSound ((short) SOUND_AFTERBURNER_PLAY, SOUNDCLASS_PLAYER, short (gameData.multiplayer.players [nPlayer].nObject));
	else
		audio.CreateObjectSound (nSound, SOUNDCLASS_PLAYER, short (gameData.multiplayer.players [nPlayer].nObject), 1, I2X (1), I2X (256),
										 AFTERBURNER_LOOP_START, AFTERBURNER_LOOP_END);
	}
}

//-----------------------------------------------------------------------------

void MultiSendCaptureBonus (char nPlayer)
{
gameData.multigame.msg.buf [0] = MULTI_CAPTURE_BONUS;
gameData.multigame.msg.buf [1] = nPlayer;
MultiSendData (gameData.multigame.msg.buf, 2, 1);
MultiDoCaptureBonus (gameData.multigame.msg.buf);
}

//-----------------------------------------------------------------------------

void MultiSendOrbBonus (char nPlayer)
{
gameData.multigame.msg.buf [0] = MULTI_ORB_BONUS;
gameData.multigame.msg.buf [1] = nPlayer;
gameData.multigame.msg.buf [2] = (char) LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX];
MultiSendData (gameData.multigame.msg.buf, 3, 1);
MultiDoOrbBonus (gameData.multigame.msg.buf);
}

//-----------------------------------------------------------------------------

void MultiDoCaptureBonus (char *buf)
{
	// Figure out the results of a network kills and add it to the
	// appropriate player's tally.

	int 	nPlayer = int (buf [1]);
	short	nTeam, penalty = 0, bonus;
	//char	szTeam [20];

bonus = BonusScore ();
gameData.score.nKillsChanged = 1;
if (nPlayer < 0) {
	penalty = 1;
	nPlayer = -nPlayer - 1;
	}
nTeam = GetTeam (nPlayer) ^ penalty;
if (nPlayer == N_LOCALPLAYER)
	HUDInitMessage (TXT_SCORED);
else
	HUDInitMessage (TXT_SCORED2, gameData.multiplayer.players [nPlayer].callsign);
gameData.multigame.score.nTeam [nTeam] += bonus;
if (nPlayer == N_LOCALPLAYER)
	audio.PlaySound (SOUND_HUD_YOU_GOT_GOAL, SOUNDCLASS_GENERIC, I2X (2));
else if (GetTeam (nPlayer) == TEAM_RED)
	audio.PlaySound (SOUND_HUD_RED_GOT_GOAL, SOUNDCLASS_GENERIC, I2X (2));
else
	audio.PlaySound (SOUND_HUD_BLUE_GOT_GOAL, SOUNDCLASS_GENERIC, I2X (2));
gameData.multiplayer.players [nPlayer].flags &= ~(PLAYER_FLAGS_FLAG);  // Clear capture flag
if (penalty) {
#if 0
	gameData.multiplayer.players [nPlayer].netKillsTotal -= bonus;
	gameData.multiplayer.players [nPlayer].nScoreGoalCount -= bonus;
	if (gameData.multigame.score.nTeam [nTeam] >= ScoreGoal ()) {
		sprintf (szTeam, "%s Team", nTeam ? TXT_RED : TXT_BLUE);
		HUDInitMessage (TXT_REACH_SCOREGOAL, szTeam);
		HUDInitMessage (TXT_CTRLCEN_DEAD);
		NetDestroyReactor (ObjFindFirstOfType (OBJ_REACTOR));
		}
#endif
	}
else {
	gameData.multiplayer.players [nPlayer].netKillsTotal += bonus;
	gameData.multiplayer.players [nPlayer].nScoreGoalCount += bonus;
	CheckScoreGoal (nPlayer);
	}
MultiSortKillList ();
MultiShowPlayerList ();
}

//-----------------------------------------------------------------------------

int GetOrbBonus (char num)
{
	int bonus;

	bonus = num* (num+1)/2;
	return (bonus);
}

//-----------------------------------------------------------------------------
	// Figure out the results of a network kills and add it to the
	// appropriate player's tally.

void MultiDoOrbBonus (char *buf)
{
	int nPlayer = int (buf [1]);
	int bonus = GetOrbBonus (buf [2]);
	int nTeam = GetTeam (nPlayer);

gameData.score.nKillsChanged = 1;
if (nPlayer == N_LOCALPLAYER)
	HUDInitMessage (TXT_SCORED_ORBS, bonus);
else
	HUDInitMessage (TXT_SCORED_ORBS2, gameData.multiplayer.players [nPlayer].callsign, buf [2]);
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
		HUDInitMessage (TXT_RECORD2, gameData.multiplayer.players [nPlayer].callsign, bonus);
	audio.PlaySound (SOUND_BUDDY_MET_GOAL, SOUNDCLASS_GENERIC, I2X (2));
	gameData.score.nChampion = nPlayer;
	gameData.score.nHighscore = bonus;
	}
gameData.multiplayer.players [nPlayer].flags &= ~(PLAYER_FLAGS_FLAG);  // Clear orb flag
gameData.multiplayer.players [nPlayer].netKillsTotal += bonus;
gameData.multiplayer.players [nPlayer].netKillsTotal %= 1000;
gameData.multiplayer.players [nPlayer].nScoreGoalCount += bonus;
gameData.multiplayer.players [nPlayer].nScoreGoalCount %= 1000;
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

void MultiDoCheating (char *buf)
{
HUDInitMessage (TXT_PLAYER_CHEATED, gameData.multiplayer.players [int (buf [1])].callsign);
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

void MultiSendGotFlag (char nPlayer)
{
gameData.multigame.msg.buf [0] = MULTI_GOT_FLAG;
gameData.multigame.msg.buf [1] = nPlayer;
soundQueue.StartSound (SOUND_HUD_YOU_GOT_FLAG, I2X (2));
MultiSendData (gameData.multigame.msg.buf, 2, 1);
MultiSendFlags ((char) N_LOCALPLAYER);
}

//-----------------------------------------------------------------------------

int bSoundHacked = 0;
CSoundSample reversedSound;

void MultiSendGotOrb (char nPlayer)
{
gameData.multigame.msg.buf [0] = MULTI_GOT_ORB;
gameData.multigame.msg.buf [1] = nPlayer;
audio.PlaySound (SOUND_YOU_GOT_ORB, SOUNDCLASS_GENERIC, I2X (2));
MultiSendData (gameData.multigame.msg.buf, 2, 1);
MultiSendFlags ((char) N_LOCALPLAYER);
}

//-----------------------------------------------------------------------------

void MultiDoGotFlag (char *buf)
{
	int nPlayer = int (buf [1]);

if (nPlayer == N_LOCALPLAYER)
	soundQueue.StartSound (SOUND_HUD_YOU_GOT_FLAG, I2X (2));
else if (GetTeam (nPlayer) == TEAM_RED)
	soundQueue.StartSound (SOUND_HUD_RED_GOT_FLAG, I2X (2));
else
	soundQueue.StartSound (SOUND_HUD_BLUE_GOT_FLAG, I2X (2));
gameData.multiplayer.players [nPlayer].flags |= PLAYER_FLAGS_FLAG;
gameData.pig.flags [!GetTeam (nPlayer)].path.Reset (10, -1);
HUDInitMessage (TXT_PICKFLAG2, gameData.multiplayer.players [nPlayer].callsign);
}

//-----------------------------------------------------------------------------

void MultiDoGotOrb (char *buf)
{
	int nPlayer = int (buf [1]);

if (IsTeamGame) {
	if (GetTeam (nPlayer) == GetTeam (N_LOCALPLAYER))
		audio.PlaySound (SOUND_FRIEND_GOT_ORB, SOUNDCLASS_GENERIC, I2X (2));
	else
		audio.PlaySound (SOUND_OPPONENT_GOT_ORB, SOUNDCLASS_GENERIC, I2X (2));
   }
else
	audio.PlaySound (SOUND_OPPONENT_GOT_ORB, SOUNDCLASS_GENERIC, I2X (2));
gameData.multiplayer.players [nPlayer].flags|= PLAYER_FLAGS_FLAG;
if (IsEntropyGame)
	HUDInitMessage (TXT_PICKVIRUS2, gameData.multiplayer.players [nPlayer].callsign);
else
	HUDInitMessage (TXT_PICKORB2, gameData.multiplayer.players [nPlayer].callsign);
}

//-----------------------------------------------------------------------------

void DropOrb (void)
{
	int nObject;

if (!(gameData.app.nGameMode &(GM_HOARD | GM_ENTROPY)))
	return; // How did we get here? Get Leighton!
if (!LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX]) {
	HUDInitMessage (IsHoardGame ? TXT_NO_ORBS : TXT_NO_VIRUS);
	return;
	}
nObject = SpitPowerup (gameData.objs.consoleP, POW_HOARD_ORB);
if (nObject < 0)
	return;
HUDInitMessage (IsHoardGame ? TXT_DROP_ORB : TXT_DROP_VIRUS);
audio.PlaySound (SOUND_DROP_WEAPON);
if (nObject > -1)
	if (gameData.app.nGameMode &(GM_HOARD | GM_ENTROPY))
		MultiSendDropFlag (nObject, RandShort ());
// If empty, tell everyone to stop drawing the box around me
if (!--LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX])
	MultiSendFlags ((char) N_LOCALPLAYER);
}

//-----------------------------------------------------------------------------

void DropFlag (void)
{
	int nObject;

if (!(gameData.app.GameMode (GM_CAPTURE)) && !(gameData.app.nGameMode &(GM_HOARD | GM_ENTROPY)))
	return;
if (gameData.app.nGameMode &(GM_HOARD | GM_ENTROPY)) {
	DropOrb ();
	return;
	}
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_FLAG)) {
	HUDInitMessage (TXT_NO_FLAG);
	return;
	}
HUDInitMessage (TXT_DROP_FLAG);
audio.PlaySound (SOUND_DROP_WEAPON);
nObject = SpitPowerup (gameData.objs.consoleP, (ubyte) ((GetTeam (N_LOCALPLAYER) == TEAM_RED) ? POW_BLUEFLAG : POW_REDFLAG));
if (nObject < 0)
	return;
if ((gameData.app.GameMode (GM_CAPTURE)) && (nObject > -1))
	MultiSendDropFlag (nObject, RandShort ());
LOCALPLAYER.flags &= ~ (PLAYER_FLAGS_FLAG);
}

//-----------------------------------------------------------------------------

void MultiSendDropFlag (int nObject, int seed)
{
	CObject *objP;
	int count = 0;

objP = &OBJECTS [nObject];
gameData.multigame.msg.buf [count++] = (char)MULTI_DROP_FLAG;
gameData.multigame.msg.buf [count++] = (char)objP->info.nId;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + count, N_LOCALPLAYER);
count += 2;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + count, nObject);
count += 2;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + count, objP->cType.powerupInfo.nCount);
count += 2;
PUT_INTEL_INT (gameData.multigame.msg.buf + count, seed);
MapObjnumLocalToLocal (nObject);
MultiSendData (gameData.multigame.msg.buf, 12, 2);
}

//-----------------------------------------------------------------------------

void MultiDoDropFlag (char *buf)
{
	int nPlayer, ammo, nObject, nRemoteObj, seed;
	CObject *objP;
	ubyte powerupId;

powerupId = (ubyte) buf [1];
nPlayer = GET_INTEL_SHORT (buf + 2);
nRemoteObj = GET_INTEL_SHORT (buf + 4);
ammo = GET_INTEL_SHORT (buf + 6);
seed = GET_INTEL_INT (buf + 8);

objP = OBJECTS + gameData.multiplayer.players [nPlayer].nObject;
nObject = SpitPowerup (objP, powerupId, seed);
MapObjnumLocalToRemote (nObject, nRemoteObj, nPlayer);
if (nObject!=-1)
	OBJECTS [nObject].cType.powerupInfo.nCount = ammo;
if (IsEntropyGame)
	OBJECTS [nObject].info.nCreator = GetTeam (nPlayer) + 1;
else if (!(gameData.app.nGameMode &(GM_HOARD | GM_ENTROPY))) {
	gameData.multiplayer.players [nPlayer].flags &= ~(PLAYER_FLAGS_FLAG);
	}
}

//-----------------------------------------------------------------------------

void MultiBadRestore ()
{
SetFunctionMode (FMODE_MENU);
MsgBox (NULL, NULL, 1, TXT_OK,
	            "A multi-save game was restored\nthat you are missing or does not\nmatch that of the others.\nYou must rejoin if you wish to\ncontinue.");
SetFunctionMode (FMODE_GAME);
gameData.multigame.bQuitGame = 1;
gameData.multigame.menu.bLeave = 1;
MultiResetStuff ();
}

//-----------------------------------------------------------------------------

void MultiSendRobotControls (char nPlayer)
{
	int count = 2;

gameData.multigame.msg.buf [0] = MULTI_ROBOT_CONTROLS;
gameData.multigame.msg.buf [1] = nPlayer;
memcpy (&(gameData.multigame.msg.buf [count]), &gameData.multigame.robots.controlled, MAX_ROBOTS_CONTROLLED * 4);
count += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&(gameData.multigame.msg.buf [count]), &gameData.multigame.robots.agitation, MAX_ROBOTS_CONTROLLED * 4);
count += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&(gameData.multigame.msg.buf [count]), &gameData.multigame.robots.controlledTime, MAX_ROBOTS_CONTROLLED * 4);
count += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&(gameData.multigame.msg.buf [count]), &gameData.multigame.robots.lastSendTime, MAX_ROBOTS_CONTROLLED * 4);
count += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&(gameData.multigame.msg.buf [count]), &gameData.multigame.robots.lastMsgTime, MAX_ROBOTS_CONTROLLED * 4);
count += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&(gameData.multigame.msg.buf [count]), &gameData.multigame.robots.sendPending, MAX_ROBOTS_CONTROLLED * 4);
count += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&(gameData.multigame.msg.buf [count]), &gameData.multigame.robots.fired, MAX_ROBOTS_CONTROLLED * 4);
count += (MAX_ROBOTS_CONTROLLED * 4);
NetworkSendNakedPacket (gameData.multigame.msg.buf, 1
, nPlayer);
}

//-----------------------------------------------------------------------------

void MultiDoRobotControls (char *buf)
{
	int count = 2;

if (buf [1]!=N_LOCALPLAYER) {
	Int3 (); // Get Jason!  Recieved a coop_sync that wasn't ours!
	return;
	}
memcpy (&gameData.multigame.robots.controlled, buf + count, MAX_ROBOTS_CONTROLLED * 4);
count += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&gameData.multigame.robots.agitation, buf + count, MAX_ROBOTS_CONTROLLED * 4);
count += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&gameData.multigame.robots.controlledTime, buf + count, MAX_ROBOTS_CONTROLLED * 4);
count += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&gameData.multigame.robots.lastSendTime, buf + count, MAX_ROBOTS_CONTROLLED * 4);
count += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&gameData.multigame.robots.lastMsgTime, buf + count, MAX_ROBOTS_CONTROLLED * 4);
count += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&gameData.multigame.robots.sendPending, buf + count, MAX_ROBOTS_CONTROLLED * 4);
count += (MAX_ROBOTS_CONTROLLED * 4);
memcpy (&gameData.multigame.robots.fired, buf + count, MAX_ROBOTS_CONTROLLED * 4);
count += (MAX_ROBOTS_CONTROLLED * 4);
}

//-----------------------------------------------------------------------------

#define POWERUPADJUSTS 5
int PowerupAdjustMapping [] = {11, 19, 39, 41, 44};

int MultiPowerupIs4Pack (int id)
{
	int i;

for (i = 0; i < POWERUPADJUSTS; i++)
	if (id == PowerupAdjustMapping [i])
		return 1;
return 0;
}

//-----------------------------------------------------------------------------

int MultiPowerupIsAllowed (int nId)
{
#if 1
return powerupFilter [nId];
#else
switch (nId) {
	case POW_INVUL:
		if (!netGame.m_info.DoInvulnerability)
			return (0);
		break;
	case POW_CLOAK:
		if (!netGame.m_info.DoCloak)
			return (0);
		break;
	case POW_AFTERBURNER:
		if (!netGame.m_info.DoAfterburner)
			return (0);
		break;
	case POW_FUSION:
		if (!netGame.m_info.DoFusions)
			return (0);
		break;
	case POW_PHOENIX:
		if (!netGame.m_info.DoPhoenix)
			return (0);
		break;
	case POW_HELIX:
		if (!netGame.m_info.DoHelix)
			return (0);
		break;
	case POW_MEGAMSL:
		if (!netGame.m_info.DoMegas)
			return (0);
		break;
	case POW_SMARTMSL:
		if (!netGame.m_info.DoSmarts)
			return (0);
		break;
	case POW_GAUSS:
		if (!netGame.m_info.DoGauss)
			return (0);
		break;
	case POW_VULCAN:
		if (!netGame.m_info.DoVulcan)
			return (0);
		break;
	case POW_PLASMA:
		if (!netGame.m_info.DoPlasma)
			return (0);
		break;
	case POW_OMEGA:
		if (!netGame.m_info.DoOmega)
			return (0);
		break;
	case POW_SUPERLASER:
		if (!netGame.m_info.DoSuperLaser)
			return (0);
		break;
	case POW_PROXMINE:
		if (!netGame.m_info.DoProximity)
			return (0);
		break;
	case POW_VULCAN_AMMO:
		if (!(netGame.m_info.DoVulcan || netGame.m_info.DoGauss))
			return (0);
		break;
	case POW_SPREADFIRE:
		if (!netGame.m_info.DoSpread)
			return (0);
		break;
	case POW_SMARTMINE:
		if (!netGame.m_info.DoSmartMine)
			return (0);
		break;
	case POW_FLASHMSL_1:
		if (!netGame.m_info.DoFlash)
			return (0);
		break;
	case POW_FLASHMSL_4:
		if (!netGame.m_info.DoFlash)
			return (0);
		break;
	case POW_GUIDEDMSL_1:
		if (!netGame.m_info.DoGuided)
			return (0);
		break;
	case POW_GUIDEDMSL_4:
		if (!netGame.m_info.DoGuided)
			return (0);
		break;
	case POW_EARTHSHAKER:
		if (!netGame.m_info.DoEarthShaker)
			return (0);
		break;
	case POW_MERCURYMSL_1:
		if (!netGame.m_info.DoMercury)
			return (0);
		break;
	case POW_MERCURYMSL_4:
		if (!netGame.m_info.DoMercury)
			return (0);
		break;
	case POW_CONVERTER:
		if (!netGame.m_info.DoConverter)
			return (0);
		break;
	case POW_AMMORACK:
		if (!netGame.m_info.DoAmmoRack)
			return (0);
		break;
	case POW_HEADLIGHT:
		if (!netGame.m_info.DoHeadlight)
			return (0);
		break;
	case POW_LASER:
		if (!netGame.m_info.DoLaserUpgrade)
			return (0);
		break;
	case POW_HOMINGMSL_1:
		if (!netGame.m_info.DoHoming)
			return (0);
		break;
	case POW_HOMINGMSL_4:
		if (!netGame.m_info.DoHoming)
			return (0);
		break;
	case POW_QUADLASER:
		if (!netGame.m_info.DoQuadLasers)
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
	gameData.multigame.msg.buf [0] = MULTI_FINISH_GAME;
	gameData.multigame.msg.buf [1] = N_LOCALPLAYER;

	MultiSendData (gameData.multigame.msg.buf, 2, 1);
}

//-----------------------------------------------------------------------------

extern void DoFinalBossHacks ();

void MultiDoFinishGame (char *buf)
{
if (buf [0]!=MULTI_FINISH_GAME)
	return;
if (missionManager.nCurrentLevel!=missionManager.nLastLevel)
	return;
DoFinalBossHacks ();
}

//-----------------------------------------------------------------------------

void MultiSendTriggerSpecific (char nPlayer, ubyte trig)
{
	gameData.multigame.msg.buf [0] = MULTI_START_TRIGGER;
	gameData.multigame.msg.buf [1] = trig;

NetworkSendNakedPacket (gameData.multigame.msg.buf, 2, nPlayer);
}

//-----------------------------------------------------------------------------

void MultiDoStartTrigger (char *buf)
{
TRIGGERS [(int) ((ubyte) buf [1])].m_info.flags |= TF_DISABLED;
}

//-----------------------------------------------------------------------------
// This function adds a kill to lifetime stats of this player, and possibly
// gives a promotion.  If so, it will tell everyone else

void MultiAddLifetimeKills (void)
{
if (!IsNetworkGame)
	return;
int oldrank = GetMyNetRanking ();
++networkData.nNetLifeKills;
if (oldrank != GetMyNetRanking ()) {
	MultiSendRanking ();
	if (!gameOpts->multi.bNoRankings) {
		HUDInitMessage (TXT_PROMOTED, pszRankStrings [GetMyNetRanking ()]);
		audio.PlaySound (SOUND_BUDDY_MET_GOAL, SOUNDCLASS_GENERIC, I2X (2));
		netPlayers [0].m_info.players [N_LOCALPLAYER].rank = GetMyNetRanking ();
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
int oldrank = GetMyNetRanking ();
++networkData.nNetLifeKilled;
if (oldrank != GetMyNetRanking ()) {
	MultiSendRanking ();
	netPlayers [0].m_info.players [N_LOCALPLAYER].rank = GetMyNetRanking ();
	if (!gameOpts->multi.bNoRankings)
		HUDInitMessage (TXT_DEMOTED, pszRankStrings [GetMyNetRanking ()]);
	}
if (gameStates.multi.nGameType != UDP_GAME)
	SavePlayerProfile ();
}

//-----------------------------------------------------------------------------

void MultiSendRanking ()
{
	gameData.multigame.msg.buf [0] = (char)MULTI_RANK;
	gameData.multigame.msg.buf [1] = (char)N_LOCALPLAYER;
	gameData.multigame.msg.buf [2] = (char)GetMyNetRanking ();

	MultiSendData (gameData.multigame.msg.buf, 3, 1);
}

//-----------------------------------------------------------------------------

void MultiDoRanking (char *buf)
{
	char rankstr [20];
	int nPlayer = int (buf [1]);
	char rank = buf [2];

	if (netPlayers [0].m_info.players [nPlayer].rank<rank)
		strcpy (rankstr, TXT_RANKUP);
	else if (netPlayers [0].m_info.players [nPlayer].rank>rank)
		strcpy (rankstr, TXT_RANKDOWN);
	else
		return;

	netPlayers [0].m_info.players [nPlayer].rank = rank;

	if (!gameOpts->multi.bNoRankings)
		HUDInitMessage (TXT_RANKCHANGE2, gameData.multiplayer.players [nPlayer].callsign,
								rankstr, pszRankStrings [(int)rank]);
}

//-----------------------------------------------------------------------------

void MultiSendModemPing ()
{
gameData.multigame.msg.buf [0] = MULTI_MODEM_PING;
MultiSendData (gameData.multigame.msg.buf, 1, 1);
}

//-----------------------------------------------------------------------------

void MultiSendModemPingReturn (char *buf)
{
gameData.multigame.msg.buf [0] = MULTI_MODEM_PING_RETURN;
MultiSendData (gameData.multigame.msg.buf, 1, 1);
}

//-----------------------------------------------------------------------------

void  MultiDoModemPingReturn (char *buf)
{
if (pingStats [0].launchTime <= 0)
	return;
xPingReturnTime = TimerGetFixedSeconds ();
HUDInitMessage (TXT_PINGTIME, SDL_GetTicks () - pingStats [0].launchTime);
					 //X2I (FixMul (xPingReturnTime - pingStats [0].launchTime, I2X (1000))));
pingStats [0].launchTime = 0;
}

//-----------------------------------------------------------------------------

void MultiQuickSoundHack (int nSound)
{
	int			l, i, j;
	CSoundSample	*soundP = gameData.pig.sound.sounds [gameStates.sound.bD1Sound] + nSound;
	ubyte			*dataP;

nSound = audio.XlatSound ((short) nSound);
l = soundP->nLength [soundP->bCustom];
if (reversedSound.data [0].Create (l)) {
	reversedSound.nLength [0] = l;
	dataP = soundP->data [soundP->bCustom] + l;
	for (i = 0, j = l; i < l; i++)
		reversedSound.data [0][i] = *(--dataP);
	bSoundHacked = 1;
	}
}

//-----------------------------------------------------------------------------

void MultiSendPlayByPlay (int num, int spnum, int dpnum)
{
#if 0
if (!IsHoardGame)
	return;
gameData.multigame.msg.buf [0] = MULTI_PLAY_BY_PLAY;
gameData.multigame.msg.buf [1] = (char)num;
gameData.multigame.msg.buf [2] = (char)spnum;
gameData.multigame.msg.buf [3] = (char)dpnum;
MultiSendData (gameData.multigame.msg.buf, 4, 1);
MultiDoPlayByPlay (gameData.multigame.msg.buf);
#endif
}

//-----------------------------------------------------------------------------

void MultiDoPlayByPlay (char *buf)
{
	int whichplay = buf [1];
	int spnum = buf [2];
	int dpnum = buf [3];

if (!IsHoardGame) {
	Int3 (); // Get Leighton, something bad has happened.
	return;
	}
switch (whichplay) {
	case 0: // Smacked!
		HUDInitMessage (TXT_SMACKED, gameData.multiplayer.players [dpnum].callsign, gameData.multiplayer.players [spnum].callsign);
		break;
	case 1: // Spanked!
		HUDInitMessage (TXT_SPANKED, gameData.multiplayer.players [dpnum].callsign, gameData.multiplayer.players [spnum].callsign);
		break;
	default:
		Int3 ();
	}
}

//-----------------------------------------------------------------------------

void MultiDoReturnFlagHome (char *buf)
{
	CObject	*objP = OBJECTS.Buffer ();
	int		i;
	ushort	nType = buf [1];
	ushort	id = buf [2];

for (i = 0; i < gameFileInfo.objects.count; i++, objP = OBJECTS + objP->info.nNextInSeg) {
	if ((objP->info.nType == nType) &&(objP->info.nId == id)) {
		ReturnFlagHome (objP);
		break;
		}
	}
}

//-----------------------------------------------------------------------------

void MultiSendReturnFlagHome (short nObject)
{
gameData.multigame.msg.buf [0] = (char) MULTI_RETURN_FLAG;
gameData.multigame.msg.buf [1] = (char) OBJECTS [nObject].info.nType;
gameData.multigame.msg.buf [2] = (char) OBJECTS [nObject].info.nId;
MultiSendData (gameData.multigame.msg.buf, 3, 0);
}

//-----------------------------------------------------------------------------

void MultiDoConquerWarning (char *buf)
{
audio.PlaySound (SOUND_CONTROL_CENTER_WARNING_SIREN, SOUNDCLASS_GENERIC, I2X (3));
}

//-----------------------------------------------------------------------------

void MultiSendConquerWarning ()
{
gameData.multigame.msg.buf [0] = (char) MULTI_CONQUER_WARNING;
gameData.multigame.msg.buf [1] = 0; // dummy values
gameData.multigame.msg.buf [2] = 0;
MultiSendData (gameData.multigame.msg.buf, 3, 0);
}

//-----------------------------------------------------------------------------

void MultiDoStopConquerWarning (char *buf)
{
audio.StopSound (SOUND_CONTROL_CENTER_WARNING_SIREN);
}

//-----------------------------------------------------------------------------

void MultiSendStopConquerWarning ()
{
gameData.multigame.msg.buf [0] = (char) MULTI_STOP_CONQUER_WARNING;
gameData.multigame.msg.buf [1] = 0; // dummy values
gameData.multigame.msg.buf [2] = 0;
MultiSendData (gameData.multigame.msg.buf, 3, 0);
}

//-----------------------------------------------------------------------------

void MultiDoConquerRoom (char *buf)
{
ConquerRoom (buf [1], buf [2], buf [3]);
}

//-----------------------------------------------------------------------------

void MultiSendConquerRoom (char nOwner, char prevOwner, char group)
{
gameData.multigame.msg.buf [0] = (char) MULTI_CONQUER_ROOM;
gameData.multigame.msg.buf [1] = nOwner;
gameData.multigame.msg.buf [2] = prevOwner;
gameData.multigame.msg.buf [3] = group;
MultiSendData (gameData.multigame.msg.buf, 4, 0);
}

//-----------------------------------------------------------------------------

void MultiSendTeleport (char nPlayer, short nSegment, char nSide)
{
gameData.multigame.msg.buf [0] = (char) MULTI_TELEPORT;
gameData.multigame.msg.buf [1] = nPlayer; // dummy values
*reinterpret_cast<short*> (gameData.multigame.msg.buf + 2) = INTEL_SHORT (nSegment);
gameData.multigame.msg.buf [4] = nSide; // dummy values
MultiSendData (gameData.multigame.msg.buf, 5, 0);
}

//-----------------------------------------------------------------------------

void MultiDoTeleport (char *buf)
{
	short	nObject = gameData.multiplayer.players [int (buf [1])].nObject;
	short nSegment = GET_INTEL_SHORT (buf + 2);
//	short	nSide = buf [4];

TriggerSetObjPos (nObject, nSegment);
OBJECTS [nObject].CreateAppearanceEffect ();
}

//-----------------------------------------------------------------------------

void MultiAdjustPowerups (void)
{
if (gameData.multiplayer.WaitingForExplosion () || gameData.multiplayer.WaitingForWeaponInfo ())
	return;

	int		h, i, j;
	time_t	t = gameStates.app.nSDLTicks [0];
	static	time_t	t0 = 0;

if (gameStates.multi.nGameType != UDP_GAME)
	return;
if (t - t0 < 1000)
	return;
t0 = t;
for (i = 0; i < MAX_POWERUP_TYPES; i++) {
	h = gameData.multiplayer.maxPowerupsAllowed [i] - PowerupsInMine (i);
	if (h < 0) {
		if (gameData.multiplayer.powerupsInMine [i] > 0) {
	#if DBG
			PowerupsInMine (i);
	#endif
			CObject* objP, * oldestObjP = NULL;
			int tCreate = 0x7FFFFFFF;

			FORALL_STATIC_OBJS (objP, i) {
				if ((objP->Id () == i) && (tCreate > objP->CreationTime ())) {
					tCreate = objP->CreationTime ();
					oldestObjP = objP;
					}
				}
			if (oldestObjP) {
				oldestObjP->Die ();
				MultiSendRemoveObj (OBJ_IDX (oldestObjP));
				}
			}
		}
	else if (h > 0) {
		if (MultiPowerupIs4Pack (i))
			continue;
		if ((i == POW_ENERGY) || (i == POW_SHIELD_BOOST))
			continue;
	#if DBG
		PowerupsInMine (i);
	#endif
		if (MultiPowerupIs4Pack (i + 1)) {
			if ((h = gameData.multiplayer.maxPowerupsAllowed [i + 1] - PowerupsInMine (i + 1))) {
				for (j = h / 4; j; j--)
					MaybeDropNetPowerup (-1, i + 1, FORCE_DROP);
				h %= 4;
				}
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

tMultiHandlerInfo multiHandlers [MULTI_MAX_TYPE + 1] = {
	{MultiDoPosition, 1},
	{MultiDoReappear, 1},
	{MultiDoFire, 1},
	{MultiDoKill, 0},
	{MultiDoRemoveObj, 1},
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
	{MultiDoTrigger, 1},
	{MultiDoSyncKills, 1},
	{MultiDoCountdown, 1},
	{MultiDoPlayerWeapons, 1},
	{MultiDoSyncMonsterball, 1},
	{MultiDoDropPowerup, 1},
	{MultiDoCreateWeapon, 1},
	{MultiDoAmmo, 1},
	{MultiDoFusionCharge, 1},
	{MultiDoPlayerThrust, 1}
	};

//-----------------------------------------------------------------------------

void MultiProcessData (char *buf, int len)
{
	// Take an entire message (that has already been checked for validity,
	// if necessary) and act on it.

	ubyte nType = buf [0];

if (nType > MULTI_MAX_TYPE) {
	Int3 ();
	return;
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
			MultiDoRemoveObj (buf);
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
	case MULTI_CONTROLCEN:
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
	case MULTI_TRIGGER_EXT:
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
			MultiDoPlayerWeapons (buf);
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
		Int3 ();
	}
#endif //DBG
}

//-----------------------------------------------------------------------------
//eof
