/* $Id: multi.c, v 1.15 2004/04/22 21:07:32 btb Exp $ */
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

#include "inferno.h"
#include "u_mem.h"
#include "strutil.h"
#include "game.h"
#include "modem.h"
#include "network.h"
#include "multi.h"
#include "object.h"
#include "laser.h"
#include "fuelcen.h"
#include "scores.h"
#include "gauges.h"
#include "collide.h"
#include "error.h"
#include "fireball.h"
#include "newmenu.h"
#include "mono.h"
#include "gamesave.h"
#include "wall.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "polyobj.h"
#include "bm.h"
#include "endlevel.h"
#include "key.h"
#include "playsave.h"
#include "timer.h"
#include "digi.h"
#include "sounds.h"
#include "kconfig.h"
#include "newdemo.h"
#include "text.h"
#include "kmatrix.h"
#include "multibot.h"
#include "gameseq.h"
#include "gameseg.h"
#include "physics.h"
#include "config.h"
#include "state.h"
#include "ai.h"
#include "switch.h"
#include "textures.h"
#include "byteswap.h"
#include "sounds.h"
#include "args.h"
#include "cfile.h"
#include "effects.h"
#include "automap.h"
#include "hudmsg.h"
#include "gamepal.h"
#include "banlist.h"
#include "render.h"
#include "multimsg.h"

typedef void tMultiHandler (char *);
typedef tMultiHandler *pMultiHandler;
typedef struct tMultiHandlerInfo {
	pMultiHandler	fpMultiHandler;
	char				noEndLevelSeq;
} tMultiHandlerInfo;

void MultiResetPlayerObject (tObject *objP);
void MultiResetObjectTexture (tObject *objP);
void MultiAddLifetimeKilled ();
void MultiAddLifetimeKills ();
void MultiSendPlayByPlay (int num, int spnum, int dpnum);
void MultiSendHeartBeat ();
void MultiCapObjects ();
void MultiAdjustRemoteCap (int nPlayer);
void MultiSaveGame (ubyte slot, uint id, char *desc);
void MultiRestoreGame (ubyte slot, uint id);
void MultiSetRobotAI (void);
void MultiSendPowerupUpdate ();
void BashToShield (int i, char *s);
void InitHoardData ();
void MultiApplyGoalTextures ();
void MultiBadRestore ();
void MultiDoCaptureBonus (char *buf);
void MultiDoOrbBonus (char *buf);
void MultiSendDropFlag (int nObject, int seed);
void MultiSendRanking ();
void MultiDoPlayByPlay (char *buf);
void MultiDoConquerRoom (char *buf);
void MultiDoConquerWarning (char *buf);

//
// Local macros and prototypes
//

// LOCALIZE ME!!

#define VmAngVecZero(v) (v)->p = (v)->b = (v)->h = 0

void DropPlayerEggs (tObject *player); // from collide.c
void GameLoop (int, int); // From game.c

//
// Global variaszPlayers
//

extern void SetFunctionMode (int);

tMultiData	multiData;

tNetgameInfo netGame;

tAllNetPlayersInfo netPlayers;

tBitmapIndex multi_player_textures [MAX_NUM_NET_PLAYERS][N_PLAYER_SHIP_TEXTURES];

typedef struct tNetPlayerStats {
	ubyte  messageType;
	ubyte  local_player;              // Who am i?
	uint   flags;                   // Powerup flags, see below...
	fix    energy;                  // Amount of energy remaining.
	fix    shields;                 // shields remaining (protection)
	ubyte  lives;                   // Lives remaining, 0 = game over.
	ubyte  laserLevel;             // Current level of the laser.
	ubyte  primaryWeaponFlags;    // bit set indicates the tPlayer has this weapon.
	ubyte  secondaryWeaponFlags;  // bit set indicates the tPlayer has this weapon.
	ushort primaryAmmo [MAX_PRIMARY_WEAPONS];     // How much ammo of each nType.
	ushort secondaryAmmo [MAX_SECONDARY_WEAPONS]; // How much ammo of each nType.
	int    last_score;              // Score at beginning of current level.
	int    score;                   // Current score.
	fix    cloakTime;              // Time cloaked
	fix    invulnerableTime;       // Time invulnerable
	fix    homingObjectDist;      // Distance of nearest homing tObject.
	short  nKillGoalCount;
	short  netKilledTotal;        // Number of times nKilled total
	short  netKillsTotal;         // Number of net kills total
	short  numKillsLevel;         // Number of kills this level
	short  numKillsTotal;         // Number of kills total
	short  numRobotsLevel;        // Number of initial robots this level
	short  numRobotsTotal;        // Number of robots total
	ushort hostages_rescuedTotal;  // Total number of hostages rescued.
	ushort hostagesTotal;          // Total number of hostages.
	ubyte  hostages_on_board;       // Number of hostages on ship.
	ubyte  unused [16];
} tNetPlayerStats;

int multiMessageLengths [MULTI_MAX_TYPE+1] = {
	24, // POSITION
	3,  // REAPPEAR
	8,  // FIRE
	5,  // KILL
	4,  // REMOVE_OBJECT
	97+9, // PLAYER_EXPLODE
	37, // MESSAGE (MAX_MESSAGE_LENGTH = 40)
	2,  // QUIT
	4,  // PLAY_SOUND
	41, // BEGIN_SYNC
	4,  // CONTROLCEN
	5,  // CLAIM ROBOT
	4,  // END_SYNC
	2,  // CLOAK
	3,  // ENDLEVEL_START
	5,  // DOOR_OPEN
	2,  // CREATE_EXPLOSION
	16, // CONTROLCEN_FIRE
	97+9, // PLAYER_DROP
	19, // CREATE_POWERUP
	9,  // MISSILE_TRACK
	2,  // DE-CLOAK
	2,  // MENU_CHOICE
	28, // ROBOT_POSITION  (shortpos_length (23) + 5 = 28)
	9,  // ROBOT_EXPLODE
	5,  // ROBOT_RELEASE
	18, // ROBOT_FIRE
	6,  // SCORE
	6,  // CREATE_ROBOT
	3,  // TRIGGER
	10, // BOSS_ACTIONS
	27, // ROBOT_POWERUPS
	7,  // HOSTAGE_DOOR
	2+24, // SAVE_GAME      (ubyte slot, uint id, char name [20])
	2+4,  // RESTORE_GAME   (ubyte slot, uint id)
	1+1,  // MULTI_REQ_PLAYER
	sizeof (tNetPlayerStats), // MULTI_SEND_PLAYER
	55, // MULTI_MARKER
	12, // MULTI_DROP_WEAPON
	3+sizeof (shortpos), // MULTI_GUIDED
	11, // MULTI_STOLEN_ITEMS
	6,  // MULTI_WALL_STATUS
	5,  // MULTI_HEARTBEAT
	9,  // MULTI_KILLGOALS
	9,  // MULTI_SEISMIC
	18, // MULTI_LIGHT
	2,  // MULTI_START_TRIGGER
	6,  // MULTI_FLAGS
	2,  // MULTI_DROP_BLOB
	MAX_POWERUP_TYPES+1, // MULTI_POWERUP_UPDATE
	sizeof (tActiveDoor)+3, // MULTI_ACTIVE_DOOR
	4,  // MULTI_SOUND_FUNCTION
	2,  // MULTI_CAPTURE_BONUS
	2,  // MULTI_GOT_FLAG
	12, // MULTI_DROP_FLAG
	142, // MULTI_ROBOT_CONTROLS
	2,  // MULTI_FINISH_GAME
	3,  // MULTI_RANK
	1,  // MULTI_MODEM_PING
	1,  // MULTI_MODEM_PING_RETURN
	3,  // MULTI_ORB_BONUS
	2,  // MULTI_GOT_ORB
	12, // MULTI_DROP_ORB
	4,  // MULTI_PLAY_BY_PLAY
	3,	 // MULTI_RETURN_FLAG
	4,	 // MULTI_CONQUER_ROOM
	3,  // MULTI_CONQUER_WARNING
	3,  // MULTI_STOP_CONQUER_WARNING
	5,  // MULTI_TELEPORT
	4,  // MULTI_SET_TEAM
	3,  // MULTI_MESSAGE_START
	3,  // MULTI_MESSAGE_QUIT
	3,	 // MULTI_OBJECT_TRIGGER
	6,	 // MULTI_PLAYER_SHIELDS, 
	2,	 // MULTI_INVUL
	2,	 // MULTI_DEINVUL
	29, // MULTI_WEAPONS
	40, // MULTI_MONSTERBALL
	2,  //MULTI_CHEATING
	5,  //MULTI_TRIGGER_EXT
	18	 //MULTI_SYNC_KILLS
};

void extract_netplayer_stats (tNetPlayerStats *ps, tPlayer * pd);
void use_netplayer_stats (tPlayer * ps, tNetPlayerStats *pd);
extern fix ThisLevelTime;

tPlayerShip defaultPlayerShip = {
	108, 58, 262144, 2162, 511180, 0, 0, F1_0 / 2, 9175, 
	{{146013, -59748, 35756}, 
	{-147477, -59892, 34430}, 
	{222008, -118473, 148201}, 
	{-223479, -118213, 148302}, 
	{153026, -185, -91405}, 
	{-156840, -185, -91405}, 
	{1608, -87663, 184978}, 
	{-1608, -87663, -190825}}};

//-----------------------------------------------------------------------------
// check protection
// return 0 if tPlayer cheats and cheat cannot be neutralized
// currently doesn't do anything besides setting tPlayer ship physics to default values.

int MultiProtectGame (void)
{
if (IsMultiGame) {
	gameData.pig.ship.player->brakes = defaultPlayerShip.brakes;
	gameData.pig.ship.player->drag = defaultPlayerShip.drag;
	gameData.pig.ship.player->mass = defaultPlayerShip.mass;
	gameData.pig.ship.player->max_thrust = defaultPlayerShip.max_thrust;
	gameData.pig.ship.player->reverse_thrust = defaultPlayerShip.reverse_thrust;
	gameData.pig.ship.player->brakes = defaultPlayerShip.brakes;
	gameData.pig.ship.player->wiggle = defaultPlayerShip.wiggle;
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
	int result;

if ((nOwner >= gameData.multi.nPlayers) || (nOwner < -1)) {
	Int3 (); // Illegal!
	return (nRemoteObj);
	}
if (nOwner == -1)
	return (nRemoteObj);
if ((nRemoteObj < 0) || (nRemoteObj  >= MAX_OBJECTS))
	return -1;
result = multiData.remoteToLocal [nOwner][nRemoteObj];
if (result < 0)
	return -1;
return (result);
}

//-----------------------------------------------------------------------------
// Map a local tObject number to a remote + nOwner

int ObjnumLocalToRemote (int nLocalObj, sbyte *nOwner)
{
	int nRemoteObj;

if ((nLocalObj < 0) || (nLocalObj > gameData.objs.nLastObject)) {
	*nOwner = -1;
	return -1;
	}
*nOwner = multiData.nObjOwner [nLocalObj];
if (*nOwner == -1)
	return (nLocalObj);
if ((*nOwner >= gameData.multi.nPlayers) || (*nOwner < -1)) {
	Int3 (); // Illegal!
	*nOwner = -1;
	return nLocalObj;
	}
if (0 > (nRemoteObj = multiData.localToRemote [nLocalObj]))
	Int3 (); // See Rob, object has no remote number!
return nRemoteObj;
}

//-----------------------------------------------------------------------------
// Add a mapping from a network remote object number to a local one

void MapObjnumLocalToRemote (int nLocalObj, int nRemoteObj, int nOwner)
{
Assert (nLocalObj > -1);
Assert (nLocalObj < MAX_OBJECTS);
Assert (nRemoteObj > -1);
Assert (nRemoteObj < MAX_OBJECTS);
Assert (nOwner > -1);
Assert (nOwner != gameData.multi.nLocalPlayer);
multiData.nObjOwner [nLocalObj] = nOwner;
multiData.remoteToLocal [nOwner][nRemoteObj] = nLocalObj;
multiData.localToRemote [nLocalObj] = nRemoteObj;
return;
}

//-----------------------------------------------------------------------------
// Add a mapping for our locally created gameData.objs.objects

void MapObjnumLocalToLocal (int nLocalObj)
{
Assert (nLocalObj > -1);
Assert (nLocalObj < MAX_OBJECTS);
multiData.nObjOwner [nLocalObj] = gameData.multi.nLocalPlayer;
multiData.remoteToLocal [gameData.multi.nLocalPlayer][nLocalObj] = nLocalObj;
multiData.localToRemote [nLocalObj] = nLocalObj;
return;
}

//-----------------------------------------------------------------------------

void ResetNetworkObjects ()
{
memset (multiData.localToRemote, -1, MAX_OBJECTS*sizeof (short));
memset (multiData.remoteToLocal, -1, MAX_NUM_NET_PLAYERS*MAX_OBJECTS*sizeof (short));
memset (multiData.nObjOwner, -1, MAX_OBJECTS);
}

//
// Part 1 : functions whose main purpose in life is to divert the flow
//          of execution to either network or serial specific code based
//          on the curretn gameData.app.nGameMode value.
//

// -----------------------------------------------------------------------------

void ResetPlayerPaths (void)
{
	tPlayer	*playerP = gameData.multi.players;
	int		i;

for (i = 0; i < MAX_PLAYERS; i++, playerP++)
	ResetFlightPath (&gameData.render.thrusters [i].path, 10, 1);
}

// -----------------------------------------------------------------------------

void SetPlayerPaths (void)
{
	tPlayer	*playerP = gameData.multi.players;
	int		i;

for (i = 0; i < gameData.multi.nPlayers; i++, playerP++)
	SetPathPoint (&gameData.render.thrusters [i].path, gameData.objs.objects + playerP->nObject);
}

// -----------------------------------------------------------------------------

void MultiSetFlagPos (void)
{
	tPlayer	*playerP = gameData.multi.players;
#if 1//ndef _DEBUG
	int		i;

for (i = 0; i < gameData.multi.nPlayers; i++, playerP++)
	if (playerP->flags & PLAYER_FLAGS_FLAG)
		SetPathPoint (&gameData.pig.flags [!GetTeam (i)].path, gameData.objs.objects + playerP->nObject);
#else
SetPathPoint (&gameData.pig.flags [0].path, gameData.objs.objects + playerP->nObject);
#endif
}

//-----------------------------------------------------------------------------
// Show a score list to end of net players
// Save connect state and change to new connect state

void MultiEndLevelScore (void)
{
	int old_connect = 0;
	int i;

if (gameData.app.nGameMode & GM_NETWORK) {
	old_connect = gameData.multi.players [gameData.multi.nLocalPlayer].connected;
	if (gameData.multi.players [gameData.multi.nLocalPlayer].connected != 3)
		gameData.multi.players [gameData.multi.nLocalPlayer].connected = CONNECT_END_MENU;
	networkData.nStatus = NETSTAT_ENDLEVEL;
	}
// Do the actual screen we wish to show
SetFunctionMode (FMODE_MENU);
KMatrixView (gameData.app.nGameMode & GM_NETWORK);
SetFunctionMode (FMODE_GAME);
// Restore connect state
if (gameData.app.nGameMode & GM_NETWORK)
	gameData.multi.players [gameData.multi.nLocalPlayer].connected = old_connect;
if (gameData.app.nGameMode & GM_MULTI_COOP) {
	int i;
	for (i = 0; i < gameData.multi.nMaxPlayers; i++) // Reset keys
		gameData.multi.players [i].flags &= ~(PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY);
	}
for (i = 0; i < gameData.multi.nMaxPlayers; i++)
	gameData.multi.players [i].flags &= ~(PLAYER_FLAGS_FLAG);  // Clear capture flag

for (i = 0; i < MAX_PLAYERS; i++)
	gameData.multi.players [i].nKillGoalCount = 0;
memset (gameData.multi.maxPowerupsAllowed, 0, sizeof (gameData.multi.maxPowerupsAllowed));
memset (gameData.multi.powerupsInMine, 0, sizeof (gameData.multi.powerupsInMine));
}

//-----------------------------------------------------------------------------

short GetTeam (int nPlayer)
{
if ((gameData.app.nGameMode & GM_CAPTURE) && ((gameData.app.nGameMode & GM_SERIAL) || (gameData.app.nGameMode & GM_MODEM)))
	return nPlayer;
if (netGame.team_vector & (1 << nPlayer))
	return 1;
else
	return 0;
}

//-----------------------------------------------------------------------------

void MultiSendSetTeam (int nPlayer)
{
multiData.msg.buf [0] = (char) MULTI_SET_TEAM;
multiData.msg.buf [1] = (char) nPlayer;
multiData.msg.buf [2] = (char) GetTeam (nPlayer);
multiData.msg.buf [3] = 0;
MultiSendData (multiData.msg.buf, 4, 0);
}

//-----------------------------------------------------------------------------

static char *szTeamColors [2] = {"blue", "red"};

void SetTeam (int nPlayer, int team)
{
	int	i;

for (i = 0; i < gameData.multi.nPlayers; i++)
	if (gameData.multi.players [i].connected)
		MultiResetObjectTexture (gameData.objs.objects + gameData.multi.players [i].nObject);

if (team  >= 0) {
	if (team)
		netGame.team_vector |= (1 << nPlayer);
	else
		netGame.team_vector &= ~ (1 << nPlayer);
	}
else {
	team = !GetTeam (nPlayer);
	if (team)
		netGame.team_vector |= (1 << nPlayer);
	else
		netGame.team_vector &= ~ (1 << nPlayer);
	NetworkSendNetgameUpdate ();
	}
sprintf (multiData.msg.szMsg, TXT_TEAMCHANGE3, gameData.multi.players [nPlayer].callsign);
if (nPlayer == gameData.multi.nLocalPlayer) {
	HUDInitMessage (TXT_TEAMJOIN, szTeamColors [team]);
	ResetCockpit ();
	}
else
	HUDInitMessage (TXT_TEAMJOIN2, gameData.multi.players [i].callsign, szTeamColors [team]);
}

//-----------------------------------------------------------------------------

void MultiDoSetTeam (char *buf)
{
SetTeam ((int) buf [1], (int) buf [2]);
}

//-----------------------------------------------------------------------------

void StartPlayerDeathSequence (tObject *player);

void SwitchTeam (int nPlayer, int bForce)
{
if ((gameData.app.nGameMode & GM_CAPTURE) && 
	 ((gameData.app.nGameMode & GM_SERIAL) || (gameData.app.nGameMode & GM_MODEM)))
	return;
if (gameStates.app.bHaveExtraGameInfo [1] && (!extraGameInfo [1].bAutoBalanceTeams || bForce)) {
	int t = !GetTeam (nPlayer);
	SetTeam (nPlayer, t);
	MultiSendSetTeam (nPlayer);
	if (!bForce) {
		gameData.multi.players [nPlayer].shields = -1;
		StartPlayerDeathSequence (gameData.objs.objects + gameData.multi.players [nPlayer].nObject);
		}
	}
}

//-----------------------------------------------------------------------------

void ChoseTeam (int nPlayer)
{
	int	h, i, t, teamScore [2];

teamScore [0]  = 
teamScore [1] = 0;
for (h = i = 0; i < gameData.multi.nPlayers; i++) {
	if (t = GetTeam (i))
		h++;
	teamScore [t] = gameData.multi.players [i].score;
	}
i = gameData.multi.nPlayers / 2;
// put tPlayer on red team if red team smaller or weaker than blue team
// and on blue team otherwise
SetTeam (nPlayer, (h < i) || ((h == i) && (teamScore [1] < teamScore [0])) ? 1 : 0);
MultiSendSetTeam (nPlayer);
}

//-----------------------------------------------------------------------------

void AutoBalanceTeams ()
{
if (gameStates.app.bHaveExtraGameInfo && 
	 extraGameInfo [1].bAutoBalanceTeams && 
	 (gameData.app.nGameMode & GM_TEAM) &&
	 NetworkIAmMaster ()) {
		int	h, i, t, teamCount [2], teamScore [2];

	teamCount [0] = 
	teamCount [1] = 
	teamScore [0] = 
	teamScore [1] = 0;
	for (i = 0; i < gameData.multi.nPlayers; i++) {
		t = GetTeam (i);
		teamCount [t]++;
		teamScore [t] = gameData.multi.players [i].score;
		}
	h = teamCount [0] - teamCount [1];
	// don't change teams if 1 tPlayer or less difference
	if ((h  >= -1) && (h <= 1))
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
	for (i = 0; i < gameData.multi.nPlayers; i++) {
		if (GetTeam (i) != t)
			continue;
		if ((gameData.app.nGameMode & GM_CAPTURE) && (gameData.multi.players [i].flags & PLAYER_FLAGS_FLAG))
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

memset (multiData.kills.matrix, 0, MAX_NUM_NET_PLAYERS*MAX_NUM_NET_PLAYERS*2); // Clear kill matrix
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++) {
	multiData.kills.nSorted [i] = i;
	gameData.multi.players [i].netKilledTotal = 0;
	gameData.multi.players [i].netKillsTotal = 0;
	gameData.multi.players [i].flags = 0;
	gameData.multi.players [i].nKillGoalCount = 0;
	}
for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++) {
	multiData.robots.controlled [i] = -1;
	multiData.robots.agitation [i] = 0;
	multiData.robots.fired [i] = 0;
	}
multiData.kills.nTeam [0] = multiData.kills.nTeam [1] = 0;
gameStates.app.bEndLevelSequence = 0;
gameStates.app.bPlayerIsDead = 0;
multiData.nWhoKilledCtrlcen = -1;  // -1 = noone
//do we draw the kill list on the HUD?
multiData.kills.bShowList = 1;
multiData.bShowReticleName = 1;
multiData.kills.xShowListTimer = 0;
multiData.bIsGuided = 0;
multiData.create.nLoc = 0;       // pointer into previous array
multiData.laser.bFired = 0;  // How many times we shot
multiData.msg.bSending = 0;
multiData.msg.bDefining = 0;
multiData.msg.nIndex = 0;
multiData.msg.nReceiver = -1;
multiData.bGotoSecret = 0;
multiData.menu.bInvoked = 0;
multiData.menu.bLeave = 0;
multiData.bQuitGame = 0;
GameDisableCheats ();
gameStates.app.bPlayerExploded = 0;
gameData.objs.deadPlayerCamera = 0;
}

//-----------------------------------------------------------------------------

void MultiMakePlayerGhost (int playernum)
{
	tObject *objP;

if ((playernum == gameData.multi.nLocalPlayer) || 
		(playernum  >= MAX_NUM_NET_PLAYERS) || 
		(playernum < 0))	{
	Int3 (); // Non-terminal, see Rob
	return;
	}
objP = gameData.objs.objects + gameData.multi.players [playernum].nObject;
objP->nType = OBJ_GHOST;
objP->renderType = RT_NONE;
objP->movementType = MT_NONE;
MultiResetPlayerObject (objP);
if (gameData.app.nGameMode & GM_MULTI_ROBOTS)
	MultiStripRobots (playernum);
}

//-----------------------------------------------------------------------------

void MultiMakeGhostPlayer (int playernum)
{
	tObject *objP;

if ((playernum == gameData.multi.nLocalPlayer) || (playernum  >= MAX_NUM_NET_PLAYERS)) {
	Int3 (); // Non-terminal, see rob
	return;
	}
objP = gameData.objs.objects + gameData.multi.players [playernum].nObject;
objP->nType = OBJ_PLAYER;
objP->movementType = MT_PHYSICS;
MultiResetPlayerObject (objP);
}

//-----------------------------------------------------------------------------

int MultiGetKillList (int *plist)
{
	// Returns the number of active net players and their
	// sorted order of kills
	int i;
	int n = 0;

for (i = 0; i < gameData.multi.nPlayers; i++)
	plist [n++] = multiData.kills.nSorted [i];
if (n == 0)
	Int3 (); // SEE ROB OR MATT
return  n;
}

//-----------------------------------------------------------------------------
	// Sort the kills list each time a new kill is added

void MultiSortKillList (void)
{

	int kills [MAX_NUM_NET_PLAYERS];
	int i;
	int changed = 1;

for (i = 0; i < MAX_NUM_NET_PLAYERS; i++) {
	if (gameData.app.nGameMode & GM_MULTI_COOP)
		kills [i] = gameData.multi.players [i].score;
	else
	if (multiData.kills.bShowList == 2) {
		if (gameData.multi.players [i].netKilledTotal+gameData.multi.players [i].netKillsTotal)
			kills [i] = (int) ((double) ((double)gameData.multi.players [i].netKillsTotal/ ((double)gameData.multi.players [i].netKilledTotal+ (double)gameData.multi.players [i].netKillsTotal))*100.0);
		else
			kills [i] = -1;  // always draw the ones without any ratio last
		}
	else
		kills [i] = gameData.multi.players [i].netKillsTotal;
	}
while (changed) {
	changed = 0;
	for (i = 0; i < gameData.multi.nPlayers-1; i++) {
		if (kills [multiData.kills.nSorted [i]] < kills [multiData.kills.nSorted [i+1]]) {
			changed = multiData.kills.nSorted [i];
			multiData.kills.nSorted [i] = multiData.kills.nSorted [i+1];
			multiData.kills.nSorted [i+1] = changed;
			changed = 1;
			}
		}
	}
}

//-----------------------------------------------------------------------------

extern tObject *objP_find_first_ofType (int);
char bMultiSuicide = 0;

void MultiComputeKill (int nKiller, int nKilled)
{
	// Figure out the results of a network kills and add it to the
	// appropriate tPlayer's tally.

	int		nKilledPlayer, killedType, t0, t1;
	int		nKillerPlayer, killerType, nKillerId;
	int		nKillGoal;
	char		szKilled [(CALLSIGN_LEN*2)+4];
	char		szKiller [(CALLSIGN_LEN*2)+4];
	tPlayer	*pKiller, *pKilled;
	tObject	*objP;

nKMatrixKillsChanged = 1;
bMultiSuicide = 0;

// Both tObject numbers are localized already!
if ((nKilled < 0) || (nKilled > gameData.objs.nLastObject) || 
	 (nKiller < 0) || (nKiller > gameData.objs.nLastObject)) {
	Int3 (); // See Rob, illegal value passed to computeKill;
	return;
	}
objP = gameData.objs.objects + nKilled;
killedType = objP->nType;
nKilledPlayer = objP->id;
objP = gameData.objs.objects + nKiller;
killerType = objP->nType;
nKillerId = objP->id;
if ((killedType != OBJ_PLAYER) && (killedType != OBJ_GHOST)) {
	Int3 (); // computeKill passed non-tPlayer tObject!
	return;
	}
pKilled = gameData.multi.players + nKilledPlayer;
multiData.kills.pFlags [nKilledPlayer] = 1;
Assert ((nKilledPlayer >= 0) && (nKilledPlayer < gameData.multi.nPlayers));
if (gameData.app.nGameMode & GM_TEAM)
	sprintf (szKilled, "%s (%s)", pKilled->callsign, netGame.team_name [GetTeam (nKilledPlayer)]);
else
	sprintf (szKilled, "%s", pKilled->callsign);
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordMultiDeath (nKilledPlayer);
DigiPlaySample (SOUND_HUD_KILL, F3_0);
if (gameData.reactor.bDestroyed)
	pKilled->connected = 3;
if (killerType == OBJ_CNTRLCEN) {
	pKilled->netKilledTotal++;
	pKilled->netKillsTotal--;
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordMultiKill (nKilledPlayer, -1);
	if (nKilledPlayer == gameData.multi.nLocalPlayer) {
		HUDInitMessage ("%s %s.", TXT_YOU_WERE, TXT_KILLED_BY_NONPLAY);
		MultiAddLifetimeKilled ();
		}
	else
		HUDInitMessage ("%s %s %s.", szKilled, TXT_WAS, TXT_KILLED_BY_NONPLAY);
	return;
	}
else if ((killerType != OBJ_PLAYER) && (killerType != OBJ_GHOST)) {
	if ((nKillerId == SMALLMINE_ID) && (killerType != OBJ_ROBOT)) {
		if (nKilledPlayer == gameData.multi.nLocalPlayer)
			HUDInitMessage (TXT_MINEKILL);
		else
			HUDInitMessage (TXT_MINEKILL2, szKilled);
		}
	else {
		if (nKilledPlayer == gameData.multi.nLocalPlayer) {
			HUDInitMessage ("%s %s.", TXT_YOU_WERE, TXT_KILLED_BY_ROBOT);
			MultiAddLifetimeKilled ();
			}
		else
			HUDInitMessage ("%s %s %s.", szKilled, TXT_WAS, TXT_KILLED_BY_ROBOT);
		}
	pKilled->netKilledTotal++;
	return;
	}
nKillerPlayer = gameData.objs.objects [nKiller].id;
pKiller = gameData.multi.players + nKillerPlayer;
if (gameData.app.nGameMode & GM_TEAM)
	sprintf (szKiller, "%s (%s)", pKiller->callsign, netGame.team_name [GetTeam (nKillerPlayer)]);
else
	sprintf (szKiller, "%s", pKiller->callsign);
// Beyond this point, it was definitely a tPlayer-tPlayer kill situation
#ifdef _DEBUG
if ((nKillerPlayer < 0) || (nKillerPlayer  >= gameData.multi.nPlayers))
	Int3 (); // See rob, tracking down bug with kill HUD messages
if ((nKilledPlayer < 0) || (nKilledPlayer  >= gameData.multi.nPlayers))
	Int3 (); // See rob, tracking down bug with kill HUD messages
#endif
t0 = GetTeam (nKilledPlayer);
t1 = GetTeam (nKillerPlayer);
if (nKillerPlayer == nKilledPlayer) {
	if (!(gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY))) {
		if (gameData.app.nGameMode & GM_TEAM)
			multiData.kills.nTeam [GetTeam (nKilledPlayer)] -= 1;
		gameData.multi.players [nKilledPlayer].netKilledTotal += 1;
		gameData.multi.players [nKilledPlayer].netKillsTotal -= 1;
		if (gameData.demo.nState == ND_STATE_RECORDING)
			NDRecordMultiKill (nKilledPlayer, -1);
		}
	multiData.kills.matrix [nKilledPlayer][nKilledPlayer]++; // # of suicides
	if (nKillerPlayer == gameData.multi.nLocalPlayer) {
		HUDInitMessage ("%s %s %s!", TXT_YOU, TXT_KILLED, TXT_YOURSELF);
		bMultiSuicide = 1;
		MultiAddLifetimeKilled ();
		}
	else
		HUDInitMessage ("%s %s", szKilled, TXT_SUICIDE);
	}
else {
	if (gameData.app.nGameMode & GM_HOARD) {
		if (gameData.app.nGameMode & GM_TEAM) {
			if ((nKilledPlayer == gameData.multi.nLocalPlayer) && (t0 == t1))
				bMultiSuicide = 1;
			}
		}
	else if (gameData.app.nGameMode & GM_ENTROPY) {
		if (t0 == t1) {
			if (nKilledPlayer == gameData.multi.nLocalPlayer)
				bMultiSuicide = 1;
			}
		else {
			pKiller->secondaryAmmo [SMART_MINE_INDEX] += extraGameInfo [1].entropy.nBumpVirusCapacity;
			if (extraGameInfo [1].entropy.nMaxVirusCapacity)
				if (gameData.multi.players [nKillerPlayer].secondaryAmmo [SMART_MINE_INDEX] > extraGameInfo [1].entropy.nMaxVirusCapacity)
					gameData.multi.players [nKillerPlayer].secondaryAmmo [SMART_MINE_INDEX] = extraGameInfo [1].entropy.nMaxVirusCapacity;
			}
		}
	else if (gameData.app.nGameMode & GM_TEAM) {
		if (t0 == t1) {
			multiData.kills.nTeam [t0] -= 1;
			pKiller->netKillsTotal -= 1;
			pKiller->nKillGoalCount -= 1;
			}
		else {
			multiData.kills.nTeam [t1] += 1;
			pKiller->netKillsTotal += 1;
			pKiller->nKillGoalCount += 1;
			}
		}
	else {
		pKiller->netKillsTotal += 1;
		pKiller->nKillGoalCount += 1;
		}
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordMultiKill (nKillerPlayer, 1);
	multiData.kills.matrix [nKillerPlayer][nKilledPlayer] += 1;
	pKilled->netKilledTotal += 1;
	if (nKillerPlayer == gameData.multi.nLocalPlayer) {
		HUDInitMessage ("%s %s %s!", TXT_YOU, TXT_KILLED, szKilled);
		MultiAddLifetimeKills ();
		if ((gameData.app.nGameMode & GM_MULTI_COOP) && (gameData.multi.players [gameData.multi.nLocalPlayer].score  >= 1000))
			AddPointsToScore (-1000);
		}
	else if (nKilledPlayer == gameData.multi.nLocalPlayer) {
		HUDInitMessage ("%s %s %s!", szKiller, TXT_KILLED, TXT_YOU);
		MultiAddLifetimeKilled ();
		if (gameData.app.nGameMode & GM_HOARD) {
			if (gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [PROXIMITY_INDEX]>3)
				MultiSendPlayByPlay (1, nKillerPlayer, gameData.multi.nLocalPlayer);
			else if (gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [PROXIMITY_INDEX]>0)
				MultiSendPlayByPlay (0, nKillerPlayer, gameData.multi.nLocalPlayer);
			}
		}
	else
		HUDInitMessage ("%s %s %s!", szKiller, TXT_KILLED, szKilled);
	}
nKillGoal = netGame.KillGoal * ((gameData.app.nGameMode & GM_ENTROPY) ? 3 : 5);
if (netGame.KillGoal > 0) {
	if (pKiller->nKillGoalCount >= nKillGoal) {
		if (nKillerPlayer == gameData.multi.nLocalPlayer) {
			HUDInitMessage (TXT_REACH_KILLGOAL);
			gameData.multi.players [gameData.multi.nLocalPlayer].shields = i2f (200);
			}
		else
			HUDInitMessage (TXT_REACH_KILLGOAL2, pKiller->callsign);
		HUDInitMessage (TXT_CTRLCEN_DEAD);
		NetDestroyReactor (ObjFindFirstOfType (OBJ_CNTRLCEN));
		}
	}
MultiSortKillList ();
MultiShowPlayerList ();
pKilled->flags&= (~ (PLAYER_FLAGS_HEADLIGHT_ON));  // clear the nKilled guys flags/headlights
}

//-----------------------------------------------------------------------------

void MultiDoFrame (void)
{
	static int lasttime = 0;
	int i;

if (!(gameData.app.nGameMode & GM_MULTI)) {
	Int3 ();
	return;
	}

if ((gameData.app.nGameMode & GM_NETWORK) && netGame.PlayTimeAllowed && lasttime!=f2i (ThisLevelTime))	{
	for (i = 0;i<gameData.multi.nPlayers;i++)
		if (gameData.multi.players [i].connected) {
			if (i == gameData.multi.nLocalPlayer) {
				MultiSendHeartBeat ();
				lasttime = f2i (ThisLevelTime);
				}
			break;
			}
		}
MultiSendMessage (); // Send any waiting messages
if (!multiData.menu.bInvoked)
	multiData.menu.bLeave = 0;
if (gameData.app.nGameMode & GM_MULTI_ROBOTS) {
	MultiCheckRobotTimeout ();
	}
if ((gameData.app.nGameMode & GM_SERIAL) || (gameData.app.nGameMode & GM_MODEM))
	com_do_frame ();
else
	NetworkDoFrame (0, 1);
if (multiData.bQuitGame && !(multiData.menu.bInvoked || gameStates.menus.nInMenu)) {
	multiData.bQuitGame = 0;
	longjmp (gameExitPoint, 0);
	}
}

//-----------------------------------------------------------------------------

void MultiSendData (char *buf, int len, int repeat)
{
Assert (len == multiMessageLengths [(int)buf [0]]);
Assert (buf [0] <= MULTI_MAX_TYPE);
//      Assert (buf [0]  >= 0);

if (gameData.app.nGameMode & GM_NETWORK)
	Assert (buf [0] > 0);
if ((gameData.app.nGameMode & GM_SERIAL) || (gameData.app.nGameMode & GM_MODEM))
	com_send_data (buf, len, repeat);
else if (gameData.app.nGameMode & GM_NETWORK)
	NetworkSendData ((ubyte *) buf, len, repeat);
}

//-----------------------------------------------------------------------------

void MultiLeaveGame (void)
{
	fix	shields;

if (!(gameData.app.nGameMode & GM_MULTI))
	return;
if (gameData.app.nGameMode & GM_NETWORK) {
	multiData.create.nLoc = 0;
	AdjustMineSpawn ();
	MultiCapObjects ();
	shields = gameData.multi.players [gameData.multi.nLocalPlayer].shields;
	gameData.multi.players [gameData.multi.nLocalPlayer].shields = -1;
	DropPlayerEggs (gameData.objs.console);
	gameData.multi.players [gameData.multi.nLocalPlayer].shields = shields;
	MultiSendPosition (gameData.multi.players [gameData.multi.nLocalPlayer].nObject);
	MultiSendPlayerExplode (MULTI_PLAYER_DROP);
	}
MultiSendQuit (MULTI_QUIT);
if ((gameData.app.nGameMode & GM_SERIAL) || (gameData.app.nGameMode & GM_MODEM))
	serial_leave_game ();
if (gameData.app.nGameMode & GM_NETWORK)
	NetworkLeaveGame ();
gameData.app.nGameMode |= GM_GAME_OVER;
if (gameStates.app.nFunctionMode != FMODE_EXIT)
	SetFunctionMode (FMODE_MENU);
}

//-----------------------------------------------------------------------------

void MultiShowPlayerList ()
{
if (!(gameData.app.nGameMode & GM_MULTI) || (gameData.app.nGameMode & GM_MULTI_COOP))
	return;
if (multiData.kills.bShowList)
	return;
multiData.kills.xShowListTimer = F1_0*5; // 5 second timer
multiData.kills.bShowList = 1;
}

//-----------------------------------------------------------------------------

int MultiEndLevel (int *secret)
{
	int result = 0;

if ((gameData.app.nGameMode & GM_SERIAL) || (gameData.app.nGameMode & GM_MODEM))
	com_endlevel (secret);          // an opportunity to re-sync or whatever
else if (gameData.app.nGameMode & GM_NETWORK)
	result = NetworkEndLevel (secret);
return result;
}

//
// Part 2 : functions that act on network/serial messages and change 
//          the state of the game in some way.
//

//-----------------------------------------------------------------------------

int MultiMenuPoll (void)
{
	fix xOldShields;
	int bWasFuelCenAlive;
	int bPlayerWasDead;

bWasFuelCenAlive = gameData.reactor.bDestroyed;
// Special polling function for in-game menus for multiplayer and serial
if (!((gameData.app.nGameMode & GM_MULTI) && (gameStates.app.nFunctionMode == FMODE_GAME)))
	return (0);
if (multiData.menu.bLeave)
	return -1;
xOldShields = gameData.multi.players [gameData.multi.nLocalPlayer].shields;
bPlayerWasDead = gameStates.app.bPlayerIsDead;
if (!gameOpts->menus.nStyle) {
	multiData.menu.bInvoked++; // Track level of menu nesting
	GameLoop (0, 0);
	multiData.menu.bInvoked--;
	timer_delay (f0_1);   // delay 100 milliseconds
	}
if (gameStates.app.bEndLevelSequence || 
		(gameData.reactor.bDestroyed && !bWasFuelCenAlive) || 
		(gameStates.app.bPlayerIsDead != bPlayerWasDead) || 
		(gameData.multi.players [gameData.multi.nLocalPlayer].shields < xOldShields))	{
	multiData.menu.bLeave = 1;
	return -1;
	}
if ((gameData.reactor.bDestroyed) && (gameData.reactor.countdown.nSecsLeft < 10)) {
	multiData.menu.bLeave = 1;
	return -1;
	}
return 0;
}

//-----------------------------------------------------------------------------
// Do any miscellaneous stuff for a new network tPlayer after death

void MultiDoDeath (int nObject)
{
if (!(gameData.app.nGameMode & GM_MULTI_COOP))
	gameData.multi.players [gameData.multi.nLocalPlayer].flags |= (PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_GOLD_KEY);
}

//-----------------------------------------------------------------------------

void MultiDoFire (char *buf)
{
char nPlayer = buf [1];
ubyte weapon = (ubyte) buf [2];
sbyte flags = buf [4];
multiData.laser.nTrack = GET_INTEL_SHORT (buf + 6);
Assert (nPlayer < gameData.multi.nPlayers);
if (gameData.objs.objects [gameData.multi.players [nPlayer].nObject].nType == OBJ_GHOST)
	MultiMakeGhostPlayer (nPlayer);
if (weapon == FLARE_ADJUST)
	LaserPlayerFire (gameData.objs.objects + gameData.multi.players [nPlayer].nObject, 
						  FLARE_ID, 6, 1, 0);
else if (weapon >= MISSILE_ADJUST) {
	int h = weapon - MISSILE_ADJUST;
	ubyte weapon_id = secondaryWeaponToWeaponInfo [h];
	int weapon_gun = secondaryWeaponToGunNum [h] + (flags & 1);
	tPlayer *playerP = gameData.multi.players + nPlayer;
	if (h == GUIDED_INDEX)
		multiData.bIsGuided = 1;
	if (playerP->secondaryAmmo [h] > 0)
		playerP->secondaryAmmo [h]--;
	LaserPlayerFire (gameData.objs.objects + gameData.multi.players [nPlayer].nObject, 
						  weapon_id, weapon_gun, 1, 0);
	}
else {
	fix save_charge = gameData.app.fusion.xCharge;
	if (weapon == FUSION_INDEX)
		gameData.app.fusion.xCharge = flags << 12;
	if (weapon == LASER_ID) {
		if (flags & LASER_QUAD)
			gameData.multi.players [nPlayer].flags |= PLAYER_FLAGS_QUAD_LASERS;
		else
			gameData.multi.players [nPlayer].flags &= ~PLAYER_FLAGS_QUAD_LASERS;
		}
	LaserFireObject ((short) gameData.multi.players [nPlayer].nObject, weapon, (int)buf [3], flags, (int)buf [5]);
	if (weapon == FUSION_INDEX)
		gameData.app.fusion.xCharge = save_charge;
	}
}

//-----------------------------------------------------------------------------
// This routine does only tPlayer positions, mode game only

void MultiDoPosition (char *buf)
{
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	shortpos sp;
#endif
int nPlayer = (gameData.multi.nLocalPlayer+1)%2;
Assert (&gameData.objs.objects [gameData.multi.players [nPlayer].nObject] != gameData.objs.console);
if (gameData.app.nGameMode & GM_NETWORK) {
	Int3 (); // Get Jason, what the hell are we doing here?
	return;
	}
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
ExtractShortPos (&gameData.objs.objects [gameData.multi.players [nPlayer].nObject], (shortpos *) (buf+1), 0);
#else
memcpy ((ubyte *) (sp.bytemat), (ubyte *) (buf + 1), 9);
memcpy ((ubyte *)& (sp.xo), (ubyte *) (buf + 10), 14);
ExtractShortPos (&gameData.objs.objects [gameData.multi.players [nPlayer].nObject], &sp, 1);
#endif
if (gameData.objs.objects [gameData.multi.players [nPlayer].nObject].movementType == MT_PHYSICS)
	set_thrust_from_velocity (&gameData.objs.objects [gameData.multi.players [nPlayer].nObject]);
}

//-----------------------------------------------------------------------------

void MultiDoReappear (char *buf)
{
	short nObject = GET_INTEL_SHORT (buf + 1);
	tObject *objP = gameData.objs.objects + nObject;

Assert (nObject  >= 0);
MultiMakeGhostPlayer (objP->id);
CreatePlayerAppearanceEffect (objP);
multiData.kills.pFlags [objP->id] = 0;
}

//-----------------------------------------------------------------------------
// Only call this for players, not robots.  nPlayer is tPlayer number, not
// Object number.

void MultiDoPlayerExplode (char *buf)
{
	tObject	*objP;
	tPlayer	*playerP;
	int		count, nPlayer, i;
	fix		shields;
	short		s;
	char		nRemoteCreated;

nPlayer = buf [1];
#ifdef NDEBUG
if ((nPlayer < 0) || (nPlayer >= gameData.multi.nPlayers))
	return;
#else
Assert (nPlayer >= 0);
Assert (nPlayer < gameData.multi.nPlayers);
#endif
// If we are in the process of sending gameData.objs.objects to a new tPlayer, reset that process
if (networkData.bSendObjects)
	networkData.nSendObjNum = -1;
// Stuff the gameData.multi.players structure to prepare for the explosion
playerP = gameData.multi.players + nPlayer;
count = 2;
playerP->primaryWeaponFlags = GET_INTEL_SHORT (buf + count); 
count += 2;
playerP->secondaryWeaponFlags = GET_INTEL_SHORT (buf + count); 
count += 2;
playerP->laserLevel = buf [count++];  
playerP->secondaryAmmo [HOMING_INDEX] = buf [count++];                
playerP->secondaryAmmo [CONCUSSION_INDEX] = buf [count++];
playerP->secondaryAmmo [SMART_INDEX] = buf [count++];         
playerP->secondaryAmmo [MEGA_INDEX] = buf [count++];          
playerP->secondaryAmmo [PROXIMITY_INDEX] = buf [count++]; 
playerP->secondaryAmmo [FLASHMSL_INDEX] = buf [count++]; 
playerP->secondaryAmmo [GUIDED_INDEX] = buf [count++]; 
playerP->secondaryAmmo [SMART_MINE_INDEX] = buf [count++]; 
playerP->secondaryAmmo [SMISSILE4_INDEX] = buf [count++]; 
playerP->secondaryAmmo [SMISSILE5_INDEX] = buf [count++]; 
playerP->primaryAmmo [VULCAN_INDEX] = GET_INTEL_SHORT (buf + count); 
count += 2;
playerP->primaryAmmo [GAUSS_INDEX] = GET_INTEL_SHORT (buf + count); 
count += 2;
playerP->flags = GET_INTEL_INT (buf + count);               
count += 4;
MultiAdjustRemoteCap (nPlayer);
objP = gameData.objs.objects + playerP->nObject;
nRemoteCreated = buf [count++]; // How many did the other guy create?
multiData.create.nLoc = 0;
shields = playerP->shields;
playerP->shields = -1;
DropPlayerEggs (objP);
playerP->shields = shields;
// Create mapping from remote to local numbering system
for (i = 0; i < nRemoteCreated; i++) {
	s = GET_INTEL_SHORT (buf + count);
	if ((i < multiData.create.nLoc) && (s > 0))
		MapObjnumLocalToRemote ((short) multiData.create.nObjNums [i], s, nPlayer);
	count += 2;
	}
for (i = nRemoteCreated; i < multiData.create.nLoc; i++)
	gameData.objs.objects [multiData.create.nObjNums [i]].flags |= OF_SHOULD_BE_DEAD;
if (buf [0] == MULTI_PLAYER_EXPLODE) {
	KillPlayerSmoke (nPlayer);
	ExplodeBadassPlayer (objP);
	objP->flags &= ~OF_SHOULD_BE_DEAD;              //don't really kill tPlayer
	MultiMakePlayerGhost (nPlayer);
	}
else
	CreatePlayerAppearanceEffect (objP);
playerP->flags &= ~(PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE | PLAYER_FLAGS_FLAG);
playerP->cloakTime = 0;
}

//-----------------------------------------------------------------------------

void MultiDoKill (char *buf)
{
	int nKiller, nKilled;

int nPlayer = (int) (buf [1]);
if ((nPlayer < 0) || (nPlayer >= gameData.multi.nPlayers)) {
	Int3 (); // Invalid player number nKilled
	return;
	}
nKilled = gameData.multi.players [nPlayer].nObject;
gameData.multi.players [nPlayer].shields = -1;
nKiller = GET_INTEL_SHORT (buf + 2);
if (nKiller > 0)
	nKiller = ObjnumRemoteToLocal (nKiller, (sbyte) buf [4]);
MultiComputeKill (nKiller, nKilled);
}

//-----------------------------------------------------------------------------
// Changed by MK on 10/20/94 to send NULL as tObject to NetDestroyReactor if it got -1
// which means not a controlcen tObject, but contained in another tObject

void MultiDoDestroyReactor (char *buf)
{
short nObject = GET_INTEL_SHORT (buf + 1);
sbyte who = buf [3];
if (gameData.reactor.bDestroyed != 1) {
	if ((who < gameData.multi.nPlayers) && (who != gameData.multi.nLocalPlayer))
		HUDInitMessage ("%s %s", gameData.multi.players [who].callsign, TXT_HAS_DEST_CONTROL);
	else if (who == gameData.multi.nLocalPlayer)
		HUDInitMessage (TXT_YOU_DEST_CONTROL);
	else
		HUDInitMessage (TXT_CONTROL_DESTROYED);
	if (nObject != -1)
		NetDestroyReactor (gameData.objs.objects + nObject);
	else
		NetDestroyReactor (NULL);
	}
}

//-----------------------------------------------------------------------------

void MultiDoEscape (char *buf)
{
	int nObject;

nObject = gameData.multi.players [(int)buf [1]].nObject;
DigiPlaySample (SOUND_HUD_MESSAGE, F1_0);
DigiKillSoundLinkedToObject (nObject);
if (buf [2] == 0) {
	HUDInitMessage ("%s %s", gameData.multi.players [(int)buf [1]].callsign, TXT_HAS_ESCAPED);
	if (gameData.app.nGameMode & GM_NETWORK)
		gameData.multi.players [(int)buf [1]].connected = CONNECT_ESCAPE_TUNNEL;
	if (!multiData.bGotoSecret)
		multiData.bGotoSecret = 2;
	}
else if (buf [2] == 1) {
	HUDInitMessage ("%s %s", gameData.multi.players [(int)buf [1]].callsign, TXT_HAS_FOUND_SECRET);
	if (gameData.app.nGameMode & GM_NETWORK)
		gameData.multi.players [(int)buf [1]].connected = CONNECT_FOUND_SECRET;
	if (!multiData.bGotoSecret)
		multiData.bGotoSecret = 1;
	}
CreatePlayerAppearanceEffect (&gameData.objs.objects [nObject]);
MultiMakePlayerGhost (buf [1]);
}

//-----------------------------------------------------------------------------

void MultiDoRemObj (char *buf)
{
	short nObject; // which tObject to remove
	short nLocalObj;
	sbyte obj_owner; // which remote list is it entered in
	int	id;

nObject = GET_INTEL_SHORT (buf + 1);
obj_owner = buf [3];

Assert (nObject  >= 0);
if (nObject < 1)
	return;
nLocalObj = ObjnumRemoteToLocal (nObject, obj_owner); // translate to local nObject
if (nLocalObj < 0)
	return;
if ((gameData.objs.objects [nLocalObj].nType != OBJ_POWERUP) && (gameData.objs.objects [nLocalObj].nType != OBJ_HOSTAGE))
	return;
if (networkData.bSendObjects && NetworkObjnumIsPast (nLocalObj))
	networkData.nSendObjNum = -1;
if (gameData.objs.objects [nLocalObj].nType == OBJ_POWERUP)
	if (gameData.app.nGameMode & GM_NETWORK) {
		id = gameData.objs.objects [nLocalObj].id;
		if (gameData.multi.powerupsInMine [id] > 0)
			gameData.multi.powerupsInMine [id]--;
		if (MultiPowerupIs4Pack (id)) {
			if (gameData.multi.powerupsInMine [--id] - 4 < 0)
				gameData.multi.powerupsInMine [id] = 0;
			else
				gameData.multi.powerupsInMine [id] -= 4;
			}
		}
gameData.objs.objects [nLocalObj].flags |= OF_SHOULD_BE_DEAD; // quick and painless
}

//-----------------------------------------------------------------------------

void MultiDoQuit (char *buf)
{
if (gameData.app.nGameMode & GM_NETWORK) {
	int i, n = 0;

	DigiPlaySample (SOUND_HUD_MESSAGE, F1_0);
	HUDInitMessage ("%s %s", gameData.multi.players [(int)buf [1]].callsign, TXT_HAS_LEFT_THE_GAME);
	NetworkDisconnectPlayer (buf [1]);
	if (multiData.menu.bInvoked || gameStates.menus.nInMenu)
		return;
	for (i = 0; i < gameData.multi.nPlayers; i++)
		if (gameData.multi.players [i].connected) 
			n++;
	if (n == 1)
		if (1 || IsCoopGame) {
			char szMsg [100];
			
			sprintf (szMsg, "%c%c%s", 1, GrFindClosestColor (gamePalette, 200, 0, 0), TXT_ONLY_PLAYER);
			HUDInitMessage (szMsg);
			}
		else
			ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_ONLY_PLAYER);
	}

if ((gameData.app.nGameMode & GM_SERIAL) || (gameData.app.nGameMode & GM_MODEM)) {
	SetFunctionMode (FMODE_MENU);
	multiData.bQuitGame = 1;
	multiData.menu.bLeave = 1;
	ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_OPPONENT_LEFT);
	SetFunctionMode (FMODE_GAME);
	MultiResetStuff ();
	}
return;
}

//-----------------------------------------------------------------------------

void MultiDoInvul (char *buf)
{
int nPlayer = (int) (buf [1]);
Assert (nPlayer < gameData.multi.nPlayers);
gameData.multi.players [nPlayer].flags |= PLAYER_FLAGS_INVULNERABLE;
gameData.multi.players [nPlayer].cloakTime = gameData.time.xGame;
if (gameData.app.nGameMode & GM_MULTI_ROBOTS)
	MultiStripRobots (nPlayer);
}

//-----------------------------------------------------------------------------

void MultiDoDeInvul (char *buf)
{
int nPlayer = (int) (buf [1]);
Assert (nPlayer < gameData.multi.nPlayers);
gameData.multi.players [nPlayer].flags &= ~PLAYER_FLAGS_INVULNERABLE;
}

//-----------------------------------------------------------------------------

void MultiDoCloak (char *buf)
{
int nPlayer = (int) (buf [1]);
Assert (nPlayer < gameData.multi.nPlayers);
gameData.multi.players [nPlayer].flags |= PLAYER_FLAGS_CLOAKED;
gameData.multi.players [nPlayer].cloakTime = gameData.time.xGame;
AIDoCloakStuff ();
if (gameData.app.nGameMode & GM_MULTI_ROBOTS)
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
	sbyte tSide;
	tSegment *segP;
	wall *wallP;
	ubyte flag;

nSegment = GET_INTEL_SHORT (buf + 1);
tSide = buf [3];
flag = buf [4];

if ((nSegment < 0) || (nSegment > gameData.segs.nLastSegment) || (tSide < 0) || (tSide > 5)) {
	Int3 ();
	return;
	}
segP = gameData.segs.segments + nSegment;
if (!IS_WALL (WallNumP (segP, tSide))) {  //Opening door on illegal wall
	Int3 ();
	return;
	}
wallP = gameData.walls.walls + WallNumP (segP, tSide);
if (wallP->nType == WALL_BLASTABLE) {
	if (!(wallP->flags & WALL_BLASTED))
		WallDestroy (segP, tSide);
	return;
	}
else if (wallP->state != WALL_DOOR_OPENING) {
	WallOpenDoor (segP, tSide);
	wallP->flags = flag;
	}
else
	wallP->flags = flag;
}

//-----------------------------------------------------------------------------

void MultiDoCreateExplosion (char *buf)
{
int nPlayer = buf [1];
CreateSmallFireballOnObject (&gameData.objs.objects [gameData.multi.players [nPlayer].nObject], F1_0, 1);
}

//-----------------------------------------------------------------------------

void MultiDoCtrlcenFire (char *buf)
{
	vmsVector to_target;
	char gun_num;
	short nObject;
	int count = 1;

memcpy (&to_target, buf + count, 12);          
count += 12;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)  // swap the vector to_target
to_target.p.x = (fix)INTEL_INT ((int)to_target.p.x);
to_target.p.y = (fix)INTEL_INT ((int)to_target.p.y);
to_target.p.z = (fix)INTEL_INT ((int)to_target.p.z);
#endif
gun_num = buf [count++];                       
nObject = GET_INTEL_SHORT (buf + count);      
CreateNewLaserEasy (&to_target, &Gun_pos [(int)gun_num], nObject, CONTROLCEN_WEAPON_NUM, 1);
}

//-----------------------------------------------------------------------------

void MultiDoCreatePowerup (char *buf)
{
	short nSegment;
	short nObject;
	int nMyObj;
	char nPlayer;
	int count = 1;
	vmsVector vNewPos;
	char powerupType;

if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed)
	return;
nPlayer = buf [count++];
powerupType = buf [count++];
#if 0
if ((gameData.app.nGameMode & GM_NETWORK) &&
	 (gameData.multi.powerupsInMine [(int)powerupType] + PowerupsOnShips (powerupType) >= 
	  gameData.multi.maxPowerupsAllowed [powerupType]))
	return;
#endif
nSegment = GET_INTEL_SHORT (buf + count); 
count += 2;
nObject = GET_INTEL_SHORT (buf + count); 
count += 2;
if ((nSegment < 0) || (nSegment > gameData.segs.nLastSegment)) {
	Int3 ();
	return;
	}
memcpy (&vNewPos, buf+count, sizeof (vmsVector)); 
count += sizeof (vmsVector);
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
INTEL_VECTOR (&vNewPos);
#endif
multiData.create.nLoc = 0;
nMyObj = CallObjectCreateEgg (gameData.objs.objects + gameData.multi.players [nPlayer].nObject, 1, OBJ_POWERUP, powerupType);
if (nMyObj < 0)
	return;
if (networkData.bSendObjects && NetworkObjnumIsPast (nMyObj))
	networkData.nSendObjNum = -1;
gameData.objs.objects [nMyObj].position.vPos = vNewPos;
VmVecZero (&gameData.objs.objects [nMyObj].mType.physInfo.velocity);
RelinkObject (nMyObj, nSegment);
MapObjnumLocalToRemote (nMyObj, nObject, nPlayer);
ObjectCreateExplosion (nSegment, &vNewPos, i2f (5), VCLIP_POWERUP_DISAPPEARANCE);
#if 0
if (gameData.app.nGameMode & GM_NETWORK)
	gameData.multi.powerupsInMine [(int) powerupType]++;
#endif
}

//-----------------------------------------------------------------------------

void MultiDoPlaySound (char *buf)
{
	int nPlayer = (int) (buf [1]);
	short nSound = (int) (buf [2]);
	fix volume = (int) (buf [3]) << 12;

if (!gameData.multi.players [nPlayer].connected)
	return;
Assert (gameData.multi.players [nPlayer].nObject  >= 0);
Assert (gameData.multi.players [nPlayer].nObject <= gameData.objs.nLastObject);
DigiLinkSoundToObject (nSound, (short) gameData.multi.players [nPlayer].nObject, 0, volume);
}

//-----------------------------------------------------------------------------

void MultiDoScore (char *buf)
{
	int nPlayer = (int) (buf [1]);

if ((nPlayer < 0) || (nPlayer  >= gameData.multi.nPlayers)) {
	Int3 (); // Non-terminal, see rob
	return;
	}
if (gameData.demo.nState == ND_STATE_RECORDING) {
	int score = GET_INTEL_INT (buf + 2);
	NDRecordMultiScore (nPlayer, score);
	}
gameData.multi.players [nPlayer].score = GET_INTEL_INT (buf + 2);
MultiSortKillList ();
}

//-----------------------------------------------------------------------------

void MultiDoTrigger (char *buf)
{
	int nPlayer = (int) (buf [1]);
	int tTrigger = (int) ((ubyte) buf [2]);
	short nObject;

if ((nPlayer < 0) || (nPlayer  >= gameData.multi.nPlayers) || (nPlayer == gameData.multi.nLocalPlayer)) {
	Int3 (); // Got tTrigger from illegal playernum
	return;
	}
if ((tTrigger < 0) || (tTrigger  >= gameData.trigs.nTriggers)) {
	Int3 (); // Illegal tTrigger number in multiplayer
	return;
	}
if (gameStates.multi.nGameType == UDP_GAME)
	nObject = GET_INTEL_SHORT (buf + 3);
else
	nObject = gameData.multi.players [nPlayer].nObject;
CheckTriggerSub (nObject, gameData.trigs.triggers, gameData.trigs.nTriggers, tTrigger, nPlayer, 0, 0);
}

//-----------------------------------------------------------------------------

void MultiDoShields (char *buf)
{
	char	nPlayer = (int) (buf [1]);
	int	shields = GET_INTEL_INT (buf+2);

if ((nPlayer < 0) || (nPlayer  >= gameData.multi.nPlayers) || (nPlayer == gameData.multi.nLocalPlayer)) {
	Int3 (); // Got tTrigger from illegal playernum
	return;
	}
gameData.multi.players [nPlayer].shields  = 
gameData.objs.objects [gameData.multi.players [nPlayer].nObject].shields = shields;
}

//-----------------------------------------------------------------------------

void MultiDoObjTrigger (char *buf)
{
	int nPlayer = (int) (buf [1]);
	int tTrigger = (int) ((ubyte) buf [2]);

if ((nPlayer < 0) || (nPlayer  >= gameData.multi.nPlayers) || (nPlayer == gameData.multi.nLocalPlayer)) {
	Int3 (); // Got tTrigger from illegal playernum
	return;
	}
if ((tTrigger < 0) || (tTrigger  >= gameData.trigs.nObjTriggers)) {
	Int3 (); // Illegal tTrigger number in multiplayer
	return;
	}
CheckTriggerSub (gameData.multi.players [nPlayer].nObject, gameData.trigs.objTriggers, 
					  gameData.trigs.nObjTriggers, tTrigger, nPlayer, 0, 1);
}

//-----------------------------------------------------------------------------

void MultiDoDropMarker (char *buf)
{
	int nPlayer = (int) (buf [1]);
	int msgNum = (int) (buf [2]);
	vmsVector position;

if (nPlayer == gameData.multi.nLocalPlayer)  // my marker? don't set it down cuz it might screw up the orientation
	return;
position.p.x = GET_INTEL_INT (buf + 3);
position.p.y = GET_INTEL_INT (buf + 7);
position.p.z = GET_INTEL_INT (buf + 11);
memcpy (gameData.marker.szMessage + 2 * nPlayer + msgNum, buf + 15, 40);
gameData.marker.point [(nPlayer*2)+msgNum] = position;
if (gameData.marker.objects [(nPlayer*2)+msgNum] !=-1 && gameData.objs.objects [gameData.marker.objects [(nPlayer*2)+msgNum]].nType!=OBJ_NONE && gameData.marker.objects [(nPlayer*2)+msgNum] !=0)
	ReleaseObject (gameData.marker.objects [(nPlayer*2)+msgNum]);
gameData.marker.objects [(nPlayer*2)+msgNum] = 
	DropMarkerObject (
		&position, 
		gameData.objs.objects [gameData.multi.players [gameData.multi.nLocalPlayer].nObject].position.nSegment, 
		&gameData.objs.objects [gameData.multi.players [gameData.multi.nLocalPlayer].nObject].position.mOrient, 
		(ubyte) ((nPlayer*2)+msgNum));
strcpy (gameData.marker.nOwner [(nPlayer*2)+msgNum], gameData.multi.players [nPlayer].callsign);
}

//-----------------------------------------------------------------------------
// Update hit point status of a door

void MultiDoHostageDoorStatus (char *buf)
{
	wall	*wallP;
	fix	hps;
	short	wallnum;

wallnum = GET_INTEL_SHORT (buf + 1); 
if ((!IS_WALL (wallnum)) || (wallnum > gameData.walls.nWalls))
	return;
hps = GET_INTEL_INT (buf + 3); 
if (hps < 0)
	return;
wallP = gameData.walls.walls + wallnum;
if (wallP->nType != WALL_BLASTABLE)
	return;
if (hps < wallP->hps)
	WallDamage (gameData.segs.segments + wallP->nSegment, (short) wallP->nSide, wallP->hps - hps);
}

//-----------------------------------------------------------------------------

void MultiDoSaveGame (char *buf)
{
	char desc [25];

ubyte slot = (ubyte) buf [1];
uint id = GET_INTEL_INT (buf + 2);
memcpy (desc, buf + 6, 20); 
MultiSaveGame (slot, id, desc);
}

//-----------------------------------------------------------------------------

void MultiDoRestoreGame (char *buf)
{
ubyte slot = (ubyte) buf [1];  
uint id = GET_INTEL_INT (buf + 2);
MultiRestoreGame (slot, id);
}

//-----------------------------------------------------------------------------

void MultiDoReqPlayer (char *buf)
{
tNetPlayerStats ps;
ubyte player_n;
// Send my tNetPlayerStats to everyone!
player_n = *(ubyte *) (buf+1);
if ((player_n == gameData.multi.nLocalPlayer) || (player_n == 255)) {
	extract_netplayer_stats (&ps, gameData.multi.players + gameData.multi.nLocalPlayer);
	ps.local_player = gameData.multi.nLocalPlayer;
	ps.messageType = MULTI_SEND_PLAYER;            // SET
	MultiSendData ((char *)&ps, sizeof (tNetPlayerStats), 0);
	}
}

//-----------------------------------------------------------------------------
// Got a tPlayer packet from someone!!!

void MultiDoSendPlayer (char *buf)
{
tNetPlayerStats *p = (tNetPlayerStats *)buf;
Assert (p->local_player <= gameData.multi.nPlayers);
use_netplayer_stats (gameData.multi.players  + p->local_player, p);
}

//-----------------------------------------------------------------------------
// A generic, emergency function to solve proszPlayerms that crop up
// when a tPlayer exits quick-out from the game because of a
// serial connection loss.  Fixes several weird bugs!

void MultiResetStuff (void)
{
DeadPlayerEnd ();
gameData.multi.players [gameData.multi.nLocalPlayer].homingObjectDist = -F1_0; // Turn off homing sound.
gameData.objs.deadPlayerCamera = 0;
gameStates.app.bEndLevelSequence = 0;
ResetRearView ();
}

//-----------------------------------------------------------------------------

void MultiResetPlayerObject (tObject *objP)
{
	int i;

//Init physics for a non-console tPlayer
Assert (objP >= gameData.objs.objects);
Assert (objP <= gameData.objs.objects+gameData.objs.nLastObject);
Assert ((objP->nType == OBJ_PLAYER) || (objP->nType == OBJ_GHOST));
VmVecZero (&objP->mType.physInfo.velocity);
VmVecZero (&objP->mType.physInfo.thrust);
VmVecZero (&objP->mType.physInfo.rotVel);
VmVecZero (&objP->mType.physInfo.rotThrust);
objP->mType.physInfo.brakes = objP->mType.physInfo.turnRoll = 0;
objP->mType.physInfo.mass = gameData.pig.ship.player->mass;
objP->mType.physInfo.drag = gameData.pig.ship.player->drag;
//      objP->mType.physInfo.flags &= ~ (PF_TURNROLL | PF_LEVELLING | PF_WIGGLE | PF_USES_THRUST);
objP->mType.physInfo.flags &= ~ (PF_TURNROLL | PF_LEVELLING | PF_WIGGLE);
//Init render info
objP->renderType = RT_POLYOBJ;
objP->rType.polyObjInfo.nModel = gameData.pig.ship.player->nModel;               //what model is this?
objP->rType.polyObjInfo.nSubObjFlags = 0;         //zero the flags
for (i = 0;i<MAX_SUBMODELS;i++)
	VmAngVecZero (&objP->rType.polyObjInfo.animAngles [i]);
//reset textures for this, if not tPlayer 0
MultiResetObjectTexture (objP);
// Clear misc
objP->flags = 0;
if (objP->nType == OBJ_GHOST)
	objP->renderType = RT_NONE;
}

//-----------------------------------------------------------------------------

void MultiResetObjectTexture (tObject *objP)
{
	int id, i;

if (gameData.app.nGameMode & GM_TEAM)
	id = GetTeam (objP->id);
else
	id = objP->id;
if (id == 0)
	objP->rType.polyObjInfo.nAltTextures = 0;
else {
	Assert (N_PLAYER_SHIP_TEXTURES == gameData.models.polyModels [objP->rType.polyObjInfo.nModel].nTextures);
	for (i = 0;i<N_PLAYER_SHIP_TEXTURES;i++)
		multi_player_textures [id-1][i] = gameData.pig.tex.objBmIndex [gameData.pig.tex.pObjBmIndex [gameData.models.polyModels [objP->rType.polyObjInfo.nModel].nFirstTexture+i]];
	multi_player_textures [id-1][4] = gameData.pig.tex.objBmIndex [gameData.pig.tex.pObjBmIndex [gameData.pig.tex.nFirstMultiBitmap+ (id-1)*2]];
	multi_player_textures [id-1][5] = gameData.pig.tex.objBmIndex [gameData.pig.tex.pObjBmIndex [gameData.pig.tex.nFirstMultiBitmap+ (id-1)*2+1]];
	objP->rType.polyObjInfo.nAltTextures = id;
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
	int nType, sub_len, bytes_processed = 0;

while (bytes_processed < len) {
	nType = buf [bytes_processed];
	if ((nType < 0) || (nType > MULTI_MAX_TYPE))
		return;
	sub_len = multiMessageLengths [nType];
	Assert (sub_len > 0);
	if ((bytes_processed+sub_len) > len) {
		Int3 ();
		return;
		}
	MultiProcessData (buf + bytes_processed, sub_len);
	bytes_processed += sub_len;
	}
}

//
// Part 2 : Functions that send communication messages to inform the other
//          players of something we did.
//

//-----------------------------------------------------------------------------

void MultiSendFire (void)
{
if (!multiData.laser.bFired)
	return;
multiData.msg.buf [0] = (char)MULTI_FIRE;
multiData.msg.buf [1] = (char)gameData.multi.nLocalPlayer;
multiData.msg.buf [2] = (char)multiData.laser.nGun;
multiData.msg.buf [3] = (char)multiData.laser.nLevel;
multiData.msg.buf [4] = (char)multiData.laser.nFlags;
multiData.msg.buf [5] = (char)multiData.laser.bFired;
PUT_INTEL_SHORT (multiData.msg.buf+6, multiData.laser.nTrack);
MultiSendData (multiData.msg.buf, 8, 0);
multiData.laser.bFired = 0;
}

//-----------------------------------------------------------------------------

void MultiSendDestroyReactor (int nObject, int tPlayer)
{
if (tPlayer == gameData.multi.nLocalPlayer)
	HUDInitMessage (TXT_YOU_DEST_CONTROL);
else if ((tPlayer > 0) && (tPlayer < gameData.multi.nPlayers))
	HUDInitMessage ("%s %s", gameData.multi.players [tPlayer].callsign, TXT_HAS_DEST_CONTROL);
else
	HUDInitMessage (TXT_CONTROL_DESTROYED);
multiData.msg.buf [0] = (char)MULTI_CONTROLCEN;
PUT_INTEL_SHORT (multiData.msg.buf+1, nObject);
multiData.msg.buf [3] = tPlayer;
MultiSendData (multiData.msg.buf, 4, 2);
}

//-----------------------------------------------------------------------------

void MultiSendDropMarker (int tPlayer, vmsVector position, char messagenum, char text [])
{
if (tPlayer < gameData.multi.nPlayers) {
	multiData.msg.buf [0] = (char)MULTI_MARKER;
	multiData.msg.buf [1] = (char)tPlayer;
	multiData.msg.buf [2] = messagenum;
	PUT_INTEL_INT (multiData.msg.buf+3, position.p.x);
	PUT_INTEL_INT (multiData.msg.buf+7, position.p.y);
	PUT_INTEL_INT (multiData.msg.buf+11, position.p.z);
	memcpy (multiData.msg.buf + 15, text, 40);
	MultiSendData (multiData.msg.buf, 55, 1);
	}
}

//-----------------------------------------------------------------------------

void MultiSendEndLevelStart (int secret)
{
multiData.msg.buf [0] = (char)MULTI_ENDLEVEL_START;
multiData.msg.buf [1] = gameData.multi.nLocalPlayer;
multiData.msg.buf [2] = (char)secret;
if ((secret) && !multiData.bGotoSecret)
	multiData.bGotoSecret = 1;
else if (!multiData.bGotoSecret)
	multiData.bGotoSecret = 2;
MultiSendData (multiData.msg.buf, 3, 1);
if (gameData.app.nGameMode & GM_NETWORK) {
	gameData.multi.players [gameData.multi.nLocalPlayer].connected = 5;
	NetworkSendEndLevelPacket ();
	}
}

//-----------------------------------------------------------------------------

void MultiSendPlayerExplode (char nType)
{
	int count = 0;
	int i;

Assert ((nType == MULTI_PLAYER_DROP) || (nType == MULTI_PLAYER_EXPLODE));
MultiSendPosition (gameData.multi.players [gameData.multi.nLocalPlayer].nObject);
if (networkData.bSendObjects)
	networkData.nSendObjNum = -1;
multiData.msg.buf [count++] = nType;
multiData.msg.buf [count++] = gameData.multi.nLocalPlayer;
PUT_INTEL_SHORT (multiData.msg.buf+count, gameData.multi.players [gameData.multi.nLocalPlayer].primaryWeaponFlags);
count += 2;
PUT_INTEL_SHORT (multiData.msg.buf+count, gameData.multi.players [gameData.multi.nLocalPlayer].secondaryWeaponFlags);
count += 2;
multiData.msg.buf [count++] = (char)gameData.multi.players [gameData.multi.nLocalPlayer].laserLevel;
multiData.msg.buf [count++] = (char)gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [HOMING_INDEX];
multiData.msg.buf [count++] = (char)gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [CONCUSSION_INDEX];
multiData.msg.buf [count++] = (char)gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [SMART_INDEX];
multiData.msg.buf [count++] = (char)gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [MEGA_INDEX];
multiData.msg.buf [count++] = (char)gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [PROXIMITY_INDEX];
multiData.msg.buf [count++] = (char)gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [FLASHMSL_INDEX];
multiData.msg.buf [count++] = (char)gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [GUIDED_INDEX];
multiData.msg.buf [count++] = (char)gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [SMART_MINE_INDEX];
multiData.msg.buf [count++] = (char)gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [SMISSILE4_INDEX];
multiData.msg.buf [count++] = (char)gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [SMISSILE5_INDEX];
PUT_INTEL_SHORT (multiData.msg.buf+count, gameData.multi.players [gameData.multi.nLocalPlayer].primaryAmmo [VULCAN_INDEX]);
count += 2;
PUT_INTEL_SHORT (multiData.msg.buf+count, gameData.multi.players [gameData.multi.nLocalPlayer].primaryAmmo [GAUSS_INDEX]);
count += 2;
PUT_INTEL_INT (multiData.msg.buf+count, gameData.multi.players [gameData.multi.nLocalPlayer].flags);
count += 4;
multiData.msg.buf [count++] = multiData.create.nLoc;
Assert (multiData.create.nLoc <= MAX_NET_CREATE_OBJECTS);
memset (multiData.msg.buf+count, -1, MAX_NET_CREATE_OBJECTS*sizeof (short));
for (i = 0; i < multiData.create.nLoc; i++) {
	if (multiData.create.nObjNums [i] <= 0) {
		Int3 (); // Illegal value in created egg tObject numbers
		count += 2;
		continue;
		}
	PUT_INTEL_SHORT (multiData.msg.buf+count, multiData.create.nObjNums [i]); 
	count += 2;
	// We created these objs so our local number = the network number
	MapObjnumLocalToLocal ((short)multiData.create.nObjNums [i]);
	}
multiData.create.nLoc = 0;
if (count > multiMessageLengths [MULTI_PLAYER_EXPLODE])
	Int3 (); // See Rob
MultiSendData (multiData.msg.buf, multiMessageLengths [MULTI_PLAYER_EXPLODE], 2);
if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED)
	MultiSendDeCloak ();
if (gameData.app.nGameMode & GM_MULTI_ROBOTS)
	MultiStripRobots (gameData.multi.nLocalPlayer);
}

//-----------------------------------------------------------------------------

extern ubyte secondaryWeaponToPowerup [];
extern ubyte primaryWeaponToPowerup [];

// put a lid on how many gameData.objs.objects will be spewed by an exploding tPlayer
// to prevent rampant powerups in netgames

void MultiCapObjects ()
{
	char nType, flagtype;
	int nIndex;

if (!(gameData.app.nGameMode & GM_NETWORK))
	return;
for (nIndex = 0; nIndex < MAX_PRIMARY_WEAPONS; nIndex++) {
	nType = primaryWeaponToPowerup [nIndex];
	if (gameData.multi.powerupsInMine [(int)nType] >= gameData.multi.maxPowerupsAllowed [(int)nType])
		if (gameData.multi.players [gameData.multi.nLocalPlayer].primaryWeaponFlags & (1 << nIndex))
			gameData.multi.players [gameData.multi.nLocalPlayer].primaryWeaponFlags &= (~(1 << nIndex));
	}
// Don't do the adjustment stuff for Hoard mode
if (!(gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)))
	gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [PROXIMITY_INDEX] /= 4;
if (gameData.app.nGameMode & GM_ENTROPY)
	gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [SMART_MINE_INDEX] = 0;
else
	gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [SMART_MINE_INDEX] /= 4;

for (nIndex = 0; nIndex < MAX_SECONDARY_WEAPONS; nIndex++) {
	if (gameData.app.nGameMode & GM_HOARD)
		if (nIndex == PROXIMITY_INDEX)
			continue;
	else if (gameData.app.nGameMode & GM_ENTROPY)
		if ((nIndex == PROXIMITY_INDEX) || (nIndex == SMART_MINE_INDEX))
			continue;
	nType = secondaryWeaponToPowerup [nIndex];
	if ((gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [nIndex] +
		  gameData.multi.powerupsInMine [(int)nType]) > 
		  gameData.multi.maxPowerupsAllowed [(int)nType]) {
		if (gameData.multi.maxPowerupsAllowed [(int)nType] - gameData.multi.powerupsInMine [(int)nType] < 0)
			gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [nIndex] = 0;
		else
			gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [nIndex] = 
				(gameData.multi.maxPowerupsAllowed [(int)nType] - gameData.multi.powerupsInMine [(int)nType]);
		}
	}

if (!(gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)))
	gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [2] *= 4;
gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [7] *= 4;

if (gameData.multi.players [gameData.multi.nLocalPlayer].laserLevel > MAX_LASER_LEVEL)
	if (gameData.multi.powerupsInMine [POW_SUPERLASER] + 1 > gameData.multi.maxPowerupsAllowed [POW_SUPERLASER])
		gameData.multi.players [gameData.multi.nLocalPlayer].laserLevel = 0;

if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_QUAD_LASERS)
	if (gameData.multi.powerupsInMine [POW_QUADLASER] + 1 > gameData.multi.maxPowerupsAllowed [POW_QUADLASER])
		gameData.multi.players [gameData.multi.nLocalPlayer].flags &= (~PLAYER_FLAGS_QUAD_LASERS);

if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED)
	if (gameData.multi.powerupsInMine [POW_CLOAK] + 1 > gameData.multi.maxPowerupsAllowed [POW_CLOAK])
		gameData.multi.players [gameData.multi.nLocalPlayer].flags &= (~PLAYER_FLAGS_CLOAKED);

if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_MAP_ALL)
	if (gameData.multi.powerupsInMine [POW_FULL_MAP] + 1 > gameData.multi.maxPowerupsAllowed [POW_FULL_MAP])
		gameData.multi.players [gameData.multi.nLocalPlayer].flags &= (~PLAYER_FLAGS_MAP_ALL);

if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_AFTERBURNER)
	if (gameData.multi.powerupsInMine [POW_AFTERBURNER] + 1 > gameData.multi.maxPowerupsAllowed [POW_AFTERBURNER])
		gameData.multi.players [gameData.multi.nLocalPlayer].flags &= (~PLAYER_FLAGS_AFTERBURNER);

if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_AMMO_RACK)
	if (gameData.multi.powerupsInMine [POW_AMMORACK] + 1 > gameData.multi.maxPowerupsAllowed [POW_AMMORACK])
		gameData.multi.players [gameData.multi.nLocalPlayer].flags &= (~PLAYER_FLAGS_AMMO_RACK);

if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CONVERTER)
	if (gameData.multi.powerupsInMine [POW_CONVERTER] + 1 > gameData.multi.maxPowerupsAllowed [POW_CONVERTER])
		gameData.multi.players [gameData.multi.nLocalPlayer].flags &= (~PLAYER_FLAGS_CONVERTER);

if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_HEADLIGHT)
	if (gameData.multi.powerupsInMine [POW_HEADLIGHT] + 1 > gameData.multi.maxPowerupsAllowed [POW_HEADLIGHT])
		gameData.multi.players [gameData.multi.nLocalPlayer].flags &= (~PLAYER_FLAGS_HEADLIGHT);

if (gameData.app.nGameMode & GM_CAPTURE) {
	if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_FLAG)	{
		if (GetTeam (gameData.multi.nLocalPlayer) == TEAM_RED)
			flagtype = POW_BLUEFLAG;
		else
			flagtype = POW_REDFLAG;
		if (gameData.multi.powerupsInMine [(int)flagtype] + 1 > gameData.multi.maxPowerupsAllowed [(int)flagtype])
			gameData.multi.players [gameData.multi.nLocalPlayer].flags &= (~PLAYER_FLAGS_FLAG);
		}
	}
}

//-----------------------------------------------------------------------------
// adds players inventory to multi cap

void MultiAdjustCapForPlayer (int nPlayer)
{
	char nType;
	int nIndex;

if (!(gameData.app.nGameMode & GM_NETWORK))
	return;
for (nIndex = 0; nIndex < MAX_PRIMARY_WEAPONS; nIndex++) {
	nType = primaryWeaponToPowerup [nIndex];
	if (gameData.multi.players [nPlayer].primaryWeaponFlags & (1 << nIndex))
	    gameData.multi.maxPowerupsAllowed [(int)nType]++;
	}
for (nIndex = 0; nIndex < MAX_SECONDARY_WEAPONS; nIndex++) {
	nType = secondaryWeaponToPowerup [nIndex];
	gameData.multi.maxPowerupsAllowed [(int)nType] += gameData.multi.players [nPlayer].secondaryAmmo [nIndex];
	}
if (gameData.multi.players [nPlayer].laserLevel > MAX_LASER_LEVEL)
	gameData.multi.maxPowerupsAllowed [POW_SUPERLASER]++;
if (gameData.multi.players [nPlayer].flags & PLAYER_FLAGS_QUAD_LASERS)
	gameData.multi.maxPowerupsAllowed [POW_QUADLASER]++;
if (gameData.multi.players [nPlayer].flags & PLAYER_FLAGS_CLOAKED)
	gameData.multi.maxPowerupsAllowed [POW_CLOAK]++;
if (gameData.multi.players [nPlayer].flags & PLAYER_FLAGS_MAP_ALL)
	gameData.multi.maxPowerupsAllowed [POW_FULL_MAP]++;
if (gameData.multi.players [nPlayer].flags & PLAYER_FLAGS_AFTERBURNER)
	gameData.multi.maxPowerupsAllowed [POW_AFTERBURNER]++;
if (gameData.multi.players [nPlayer].flags & PLAYER_FLAGS_AMMO_RACK)
	gameData.multi.maxPowerupsAllowed [POW_AMMORACK]++;
if (gameData.multi.players [nPlayer].flags & PLAYER_FLAGS_CONVERTER)
	gameData.multi.maxPowerupsAllowed [POW_CONVERTER]++;
if (gameData.multi.players [nPlayer].flags & PLAYER_FLAGS_HEADLIGHT)
	gameData.multi.maxPowerupsAllowed [POW_HEADLIGHT]++;
}

//-----------------------------------------------------------------------------

void MultiAdjustRemoteCap (int nPlayer)
{
	char nType;
	int nIndex;

if (!(gameData.app.nGameMode & GM_NETWORK) || (gameStates.multi.nGameType == UDP_GAME))
	return;
for (nIndex = 0; nIndex < MAX_PRIMARY_WEAPONS; nIndex++) {
	nType = primaryWeaponToPowerup [nIndex];
	if (gameData.multi.players [nPlayer].primaryWeaponFlags & (1 << nIndex))
		   gameData.multi.powerupsInMine [(int)nType]++;
	}
for (nIndex = 0; nIndex < MAX_SECONDARY_WEAPONS; nIndex++) {
	nType = secondaryWeaponToPowerup [nIndex];
	if ((gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)) && nIndex == 2)
		continue;
	if (nIndex == 2 || nIndex == 7) // PROX or SMARTMINES? Those bastards...
		gameData.multi.powerupsInMine [(int)nType] += (gameData.multi.players [nPlayer].secondaryAmmo [nIndex]/4);
	else
		gameData.multi.powerupsInMine [(int)nType] += gameData.multi.players [nPlayer].secondaryAmmo [nIndex];
	}
if (gameData.multi.players [nPlayer].laserLevel > MAX_LASER_LEVEL)
	gameData.multi.powerupsInMine [POW_SUPERLASER]++;
if (gameData.multi.players [nPlayer].flags & PLAYER_FLAGS_QUAD_LASERS)
	gameData.multi.powerupsInMine [POW_QUADLASER]++;
if (gameData.multi.players [nPlayer].flags & PLAYER_FLAGS_CLOAKED)
	gameData.multi.powerupsInMine [POW_CLOAK]++;
if (gameData.multi.players [nPlayer].flags & PLAYER_FLAGS_MAP_ALL)
	gameData.multi.powerupsInMine [POW_FULL_MAP]++;
if (gameData.multi.players [nPlayer].flags & PLAYER_FLAGS_AFTERBURNER)
	gameData.multi.powerupsInMine [POW_AFTERBURNER]++;
if (gameData.multi.players [nPlayer].flags & PLAYER_FLAGS_AMMO_RACK)
	gameData.multi.powerupsInMine [POW_AMMORACK]++;
if (gameData.multi.players [nPlayer].flags & PLAYER_FLAGS_CONVERTER)
	gameData.multi.powerupsInMine [POW_CONVERTER]++;
if (gameData.multi.players [nPlayer].flags & PLAYER_FLAGS_HEADLIGHT)
	gameData.multi.powerupsInMine [POW_HEADLIGHT]++;
}

//-----------------------------------------------------------------------------

void MultiSendReappear ()
{
multiData.msg.buf [0] = (char) MULTI_REAPPEAR;
PUT_INTEL_SHORT (multiData.msg.buf + 1, gameData.multi.players [gameData.multi.nLocalPlayer].nObject);
MultiSendData (multiData.msg.buf, 3, 2);
multiData.kills.pFlags [gameData.multi.nLocalPlayer] = 0;
}

//-----------------------------------------------------------------------------

void MultiSendPosition (int nObject)
{
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	shortpos sp;
#endif
	int count = 0;

if (gameData.app.nGameMode & GM_NETWORK)
	return;
multiData.msg.buf [count++] = (char)MULTI_POSITION;
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
CreateShortPos ((shortpos *) (multiData.msg.buf+count), gameData.objs.objects+nObject, 0);
count += sizeof (shortpos);
#else
CreateShortPos (&sp, gameData.objs.objects+nObject, 1);
memcpy (& (multiData.msg.buf [count]), (ubyte *) (sp.bytemat), 9);
count += 9;
memcpy (& (multiData.msg.buf [count]), (ubyte *)& (sp.xo), 14);
count += 14;
#endif
MultiSendData (multiData.msg.buf, count, 0);
}

//-----------------------------------------------------------------------------
// I died, tell the world.

void MultiSendKill (int nObject)
{
	int nKillerObj;

Assert (gameData.objs.objects [nObject].id == gameData.multi.nLocalPlayer);
nKillerObj = gameData.multi.players [gameData.multi.nLocalPlayer].nKillerObj;
MultiComputeKill (nKillerObj, nObject);
multiData.msg.buf [0] = (char) MULTI_KILL;     
multiData.msg.buf [1] = gameData.multi.nLocalPlayer;           
if (nKillerObj > -1) {
	// do it with variable player since INTEL_SHORT won't work on return val from function.
	short s = (short) ObjnumLocalToRemote (nKillerObj, (sbyte *) &multiData.msg.buf [4]);
	PUT_INTEL_SHORT (multiData.msg.buf + 2, s);
	}
else {
	PUT_INTEL_SHORT (multiData.msg.buf + 2, -1);
	multiData.msg.buf [4] = (char) -1;
	}
MultiSendData (multiData.msg.buf, 5, 1);
if (gameData.app.nGameMode & GM_MULTI_ROBOTS)
	MultiStripRobots (gameData.multi.nLocalPlayer);
}

//-----------------------------------------------------------------------------

void MultiDoSyncKills (char *buf)
{
	int	i, nPlayer = (int) buf [1];
	short	*bufP = (short *) (buf + 2);

for (i = 0, bufP = (short *) (multiData.msg.buf + 2); i < MAX_NUM_NET_PLAYERS; i++, bufP++)
	multiData.kills.matrix [gameData.multi.nLocalPlayer][i] = GET_INTEL_SHORT (bufP);
}

//-----------------------------------------------------------------------------

void MultiSendSyncKills (void)
{
	int		i;
	short		*bufP;

multiData.msg.buf [0] = (char) MULTI_SYNC_KILLS;
multiData.msg.buf [1] = gameData.multi.nLocalPlayer;           
for (i = 0, bufP = (short *) (multiData.msg.buf + 2); i < MAX_NUM_NET_PLAYERS; i++, bufP++)
	PUT_INTEL_SHORT (bufP, multiData.kills.matrix [gameData.multi.nLocalPlayer] + i);
MultiSendData (multiData.msg.buf, 2 + MAX_NUM_NET_PLAYERS * sizeof (short), 0);
}

//-----------------------------------------------------------------------------

void MultiSyncKills (void)
{
	static	time_t	t0;

if (IsMultiGame && (gameStates.multi.nGameType == UDP_GAME) && (gameStates.app.nSDLTicks - t0 > 3000)) {
	t0 = gameStates.app.nSDLTicks;
	MultiSendSyncKills ();
	}
}

//-----------------------------------------------------------------------------
// Tell the other guy to remove an tObject from his list

void MultiSendRemObj (int nObject)
{
	sbyte obj_owner;
	short nRemoteObj;
	int	id;

if ((gameData.objs.objects [nObject].nType == OBJ_POWERUP) && (gameData.app.nGameMode & GM_NETWORK)) {
	id = gameData.objs.objects [nObject].id;
	if (gameData.multi.powerupsInMine [id] > 0) {
		gameData.multi.powerupsInMine [id]--;
		if (MultiPowerupIs4Pack (id)) {
			if (gameData.multi.powerupsInMine [--id] - 4 < 0)
				gameData.multi.powerupsInMine [id] = 0;
			else
				gameData.multi.powerupsInMine [id] -= 4;
			}
		}
	}
multiData.msg.buf [0] = (char) MULTI_REMOVE_OBJECT;
nRemoteObj = ObjnumLocalToRemote ((short)nObject, &obj_owner);
PUT_INTEL_SHORT (multiData.msg.buf+1, nRemoteObj); // Map to network objnums
multiData.msg.buf [3] = obj_owner;
MultiSendData (multiData.msg.buf, 4, 0);
if (networkData.bSendObjects && NetworkObjnumIsPast (nObject))
	networkData.nSendObjNum = -1;
}

//-----------------------------------------------------------------------------
// I am quitting the game, tell the other guy the bad news.

void MultiSendQuit (int why)
{
Assert (why == MULTI_QUIT);
multiData.msg.buf [0] = (char)why;
multiData.msg.buf [1] = gameData.multi.nLocalPlayer;
MultiSendData (multiData.msg.buf, 2, 1);
}

//-----------------------------------------------------------------------------
// Broadcast a change in our pflags (made to support cloaking)

void MultiSendInvul (void)
{
multiData.msg.buf [0] = MULTI_INVUL;
multiData.msg.buf [1] = (char)gameData.multi.nLocalPlayer;
MultiSendData (multiData.msg.buf, 2, 1);
if (gameData.app.nGameMode & GM_MULTI_ROBOTS)
	MultiStripRobots (gameData.multi.nLocalPlayer);
}

//-----------------------------------------------------------------------------
// Broadcast a change in our pflags (made to support cloaking)

void MultiSendDeInvul (void)
{
multiData.msg.buf [0] = MULTI_DEINVUL;
multiData.msg.buf [1] = (char)gameData.multi.nLocalPlayer;
MultiSendData (multiData.msg.buf, 2, 1);
}

//-----------------------------------------------------------------------------
// Broadcast a change in our pflags (made to support cloaking)

void MultiSendCloak (void)
{
multiData.msg.buf [0] = MULTI_CLOAK;
multiData.msg.buf [1] = (char)gameData.multi.nLocalPlayer;
MultiSendData (multiData.msg.buf, 2, 1);
if (gameData.app.nGameMode & GM_MULTI_ROBOTS)
	MultiStripRobots (gameData.multi.nLocalPlayer);
}

//-----------------------------------------------------------------------------
// Broadcast a change in our pflags (made to support cloaking)

void MultiSendDeCloak (void)
{
multiData.msg.buf [0] = MULTI_DECLOAK;
multiData.msg.buf [1] = (char)gameData.multi.nLocalPlayer;
MultiSendData (multiData.msg.buf, 2, 1);
}

//-----------------------------------------------------------------------------
// When we open a door make sure everyone else opens that door

void MultiSendDoorOpen (int nSegment, int tSide, ubyte flag)
{
multiData.msg.buf [0] = MULTI_DOOR_OPEN;
PUT_INTEL_SHORT (multiData.msg.buf+1, nSegment);
multiData.msg.buf [3] = (sbyte)tSide;
multiData.msg.buf [4] = flag;
MultiSendData (multiData.msg.buf, 5, 2);
}

//-----------------------------------------------------------------------------
// For sending doors only to a specific person (usually when they're joining)

extern void NetworkSendNakedPacket (char *, short, int);

void MultiSendDoorOpenSpecific (int nPlayer, int nSegment, int tSide, ubyte flag)
{
Assert (gameData.app.nGameMode & GM_NETWORK);
multiData.msg.buf [0] = MULTI_DOOR_OPEN;
PUT_INTEL_SHORT (multiData.msg.buf+1, nSegment);
multiData.msg.buf [3] = (sbyte)tSide;
multiData.msg.buf [4] = flag;
NetworkSendNakedPacket (multiData.msg.buf, 5, nPlayer);
}

//
// Part 3 : Functions that change or prepare the game for multiplayer use.
//          Not including functions needed to synchronize or start the
//          particular nType of multiplayer game.  Includes preparing the
//                      mines, tPlayer structures, etc.

//-----------------------------------------------------------------------------

void MultiSendCreateExplosion (int nPlayer)
{
// Send all data needed to create a remote explosion
multiData.msg.buf [0] = MULTI_CREATE_EXPLOSION;       
multiData.msg.buf [1] = (sbyte)nPlayer;                  
MultiSendData (multiData.msg.buf, 2, 0);
}

//-----------------------------------------------------------------------------

void MultiSendCtrlcenFire (vmsVector *to_goal, int nBestGun, int nObject)
{
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	vmsVector swapped_vec;
#endif
	int count = 0;

multiData.msg.buf [count++] = MULTI_CONTROLCEN_FIRE;                
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
memcpy (multiData.msg.buf+count, to_goal, 12);                    
count += 12;
#else
swapped_vec.p.x = (fix)INTEL_INT ((int)to_goal->p.x);
swapped_vec.p.y = (fix)INTEL_INT ((int)to_goal->p.y);
swapped_vec.p.z = (fix)INTEL_INT ((int)to_goal->p.z);
memcpy (multiData.msg.buf+count, &swapped_vec, 12);				
count += 12;
#endif
multiData.msg.buf [count++] = (char)nBestGun;                   
PUT_INTEL_SHORT (multiData.msg.buf+count, nObject);     
count += 2;
MultiSendData (multiData.msg.buf, count, 0);
}

//-----------------------------------------------------------------------------

void MultiSendCreatePowerup (int powerupType, int nSegment, int nObject, vmsVector *pos)
{
	// Create a powerup on a remote machine, used for remote
	// placement of used powerups like missiles and cloaking
	// powerups.

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	vmsVector swapped_vec;
#endif
	int count = 0;

#if 0
if (gameData.app.nGameMode & GM_NETWORK)
	gameData.multi.powerupsInMine [powerupType]++;
#endif
multiData.msg.buf [count++] = MULTI_CREATE_POWERUP;         
multiData.msg.buf [count++] = gameData.multi.nLocalPlayer;                                      
multiData.msg.buf [count++] = powerupType;                                 
PUT_INTEL_SHORT (multiData.msg.buf+count, nSegment);     
count += 2;
PUT_INTEL_SHORT (multiData.msg.buf+count, nObject);     
count += 2;
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
memcpy (multiData.msg.buf+count, pos, sizeof (vmsVector));  
count += sizeof (vmsVector);
#else
swapped_vec.p.x = (fix)INTEL_INT ((int)pos->p.x);
swapped_vec.p.y = (fix)INTEL_INT ((int)pos->p.y);
swapped_vec.p.z = (fix)INTEL_INT ((int)pos->p.z);
memcpy (multiData.msg.buf+count, &swapped_vec, 12);				
count += 12;
#endif
MultiSendData (multiData.msg.buf, count, 2);
if (networkData.bSendObjects && NetworkObjnumIsPast (nObject))
	networkData.nSendObjNum = -1;
MapObjnumLocalToLocal (nObject);
}

//-----------------------------------------------------------------------------

void MultiSendPlaySound (int nSound, fix volume)
{
multiData.msg.buf [0] = MULTI_PLAY_SOUND;                     
multiData.msg.buf [1] = gameData.multi.nLocalPlayer;                                  
multiData.msg.buf [2] = (char)nSound;                      
multiData.msg.buf [3] = (char) (volume >> 12); 
MultiSendData (multiData.msg.buf, 4, 0);
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

	Assert (taunt_num  >= 0);
	Assert (taunt_num < 4);
	DigiPlaySample (audio_taunts [taunt_num], F1_0);
	MultiSendPlaySound (audio_taunts [taunt_num], F1_0);
#endif
}

//-----------------------------------------------------------------------------

void MultiSendScore (void)
{
	// Send my current score to all other players so it will remain
	// synced.
	int count = 0;

if (gameData.app.nGameMode & GM_MULTI_COOP) {
	MultiSortKillList ();
	multiData.msg.buf [count++] = MULTI_SCORE;                  
	multiData.msg.buf [count++] = gameData.multi.nLocalPlayer;                           
	PUT_INTEL_INT (multiData.msg.buf+count, gameData.multi.players [gameData.multi.nLocalPlayer].score);  
	count += 4;
	MultiSendData (multiData.msg.buf, count, 0);
	}
}

//-----------------------------------------------------------------------------

void MultiSendSaveGame (ubyte slot, uint id, char * desc)
{
	int count = 0;

multiData.msg.buf [count++] = MULTI_SAVE_GAME;              
multiData.msg.buf [count++] = slot;                        
PUT_INTEL_INT (multiData.msg.buf+count, id);           
count += 4;             // Save id
memcpy (&multiData.msg.buf [count], desc, 20); 
count += 20;
MultiSendData (multiData.msg.buf, count, 2);
}

//-----------------------------------------------------------------------------

void multi_send_restore_game (ubyte slot, uint id)
{
	int count = 0;

multiData.msg.buf [count++] = MULTI_RESTORE_GAME;   
multiData.msg.buf [count++] = slot;                                                 
PUT_INTEL_INT (multiData.msg.buf+count, id);         
count += 4;             // Save id
MultiSendData (multiData.msg.buf, count, 2);
}

//-----------------------------------------------------------------------------

void MultiSendNetPlayerStatsRequest (ubyte player_num)
{
	int count = 0;

multiData.msg.buf [count] = MULTI_REQ_PLAYER;     
count += 1;
multiData.msg.buf [count] = player_num;                   
count += 1;
MultiSendData (multiData.msg.buf, count, 0);
}

//-----------------------------------------------------------------------------

void MultiSendTrigger (int triggernum, int nObject)
{
	// Send an even to tTrigger something in the mine

	int count = 1;

multiData.msg.buf [count] = gameData.multi.nLocalPlayer;                                   
count += 1;
multiData.msg.buf [count] = (ubyte)triggernum;            
count += 1;
if (gameStates.multi.nGameType == UDP_GAME) {
	multiData.msg.buf [0] = MULTI_TRIGGER_EXT;                                
	PUT_INTEL_SHORT (multiData.msg.buf + count, nObject);
	count += 2;
	}	
else
	multiData.msg.buf [0] = MULTI_TRIGGER;                                
MultiSendData (multiData.msg.buf, count, 1);
//MultiSendData (multiData.msg.buf, count, 1); // twice?
}

//-----------------------------------------------------------------------------

void MultiSendObjTrigger (int triggernum)
{
	// Send an even to tTrigger something in the mine

	int count = 0;

multiData.msg.buf [count] = MULTI_OBJ_TRIGGER;                                
count += 1;
multiData.msg.buf [count] = gameData.multi.nLocalPlayer;                                   
count += 1;
multiData.msg.buf [count] = (ubyte)triggernum;            
count += 1;
MultiSendData (multiData.msg.buf, count, 1);
//MultiSendData (multiData.msg.buf, count, 1); // twice?
}

//-----------------------------------------------------------------------------

void MultiSendHostageDoorStatus (int wallnum)
{
	// Tell the other tPlayer what the hit point status of a hostage door
	// should be
	int count = 0;

Assert (gameData.walls.walls [wallnum].nType == WALL_BLASTABLE);
multiData.msg.buf [count] = MULTI_HOSTAGE_DOOR;           
count += 1;
PUT_INTEL_SHORT (multiData.msg.buf+count, wallnum);           
count += 2;
PUT_INTEL_INT (multiData.msg.buf+count, gameData.walls.walls [wallnum].hps);  
count += 4;
MultiSendData (multiData.msg.buf, count, 0);
}

//-----------------------------------------------------------------------------

extern int nConsistencyErrorCount;
int PhallicLimit = 0;
int PhallicMan = -1;

void MultiPrepLevel (void)
{
	// Do any special stuff to the level required for serial games
	// before we begin playing in it.

	// gameData.multi.nLocalPlayer MUST be set before calling this procedure.

	// This function must be called before checksuming the Object array, 
	// since the resulting checksum with depend on the value of gameData.multi.nLocalPlayer
	// at the time this is called.

	int		i, ng = 0, nObject;
	int		cloak_count, inv_count;
	tObject	*objP;

Assert (gameData.app.nGameMode & GM_MULTI);
Assert (gameData.multi.nPlayerPositions > 0);
PhallicLimit = 0;
PhallicMan = -1;
gameStates.render.bDropAfterburnerBlob = 0;
nConsistencyErrorCount = 0;
memset (multiData.kills.pFlags, 0, MAX_NUM_NET_PLAYERS * sizeof (multiData.kills.pFlags [0]));
for (i = 0; i < gameData.multi.nPlayerPositions; i++) {
	objP = gameData.objs.objects + gameData.multi.players [i].nObject;
	if (i != gameData.multi.nLocalPlayer)
		objP->controlType = CT_REMOTE;
	objP->movementType = MT_PHYSICS;
	MultiResetPlayerObject (objP);
	networkData.nLastPacketTime [i] = 0;
	}
for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++) {
	multiData.robots.controlled [i] = -1;
	multiData.robots.agitation [i] = 0;
	multiData.robots.fired [i] = 0;
	}
gameData.objs.viewer = gameData.objs.console = gameData.objs.objects + gameData.multi.players [gameData.multi.nLocalPlayer].nObject;
if (!(gameData.app.nGameMode & GM_MULTI_COOP))
	MultiDeleteExtraObjects (); // Removes monsters from level
if (gameData.app.nGameMode & GM_MULTI_ROBOTS)
	MultiSetRobotAI (); // Set all Robot AI to types we can cope with
if (gameData.app.nGameMode & GM_NETWORK) {
	MultiAdjustCapForPlayer (gameData.multi.nLocalPlayer);
	MultiSendPowerupUpdate ();
	}
ng = 1;
inv_count = 0;
cloak_count = 0;
for (i = 0; i <= gameData.objs.nLastObject; i++) {
	if ((gameData.objs.objects [i].nType == OBJ_HOSTAGE) && !(gameData.app.nGameMode & GM_MULTI_COOP)) {
		nObject = CreateObject (OBJ_POWERUP, POW_SHIELD_BOOST, -1, gameData.objs.objects [i].position.nSegment, 
									  &gameData.objs.objects [i].position.vPos, &vmdIdentityMatrix, 
									  gameData.objs.pwrUp.info [POW_SHIELD_BOOST].size, 
									  CT_POWERUP, MT_PHYSICS, RT_POWERUP, 1);
		ReleaseObject ((short) i);
		if (nObject != -1) {
			gameData.objs.objects [nObject].rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [POW_SHIELD_BOOST].nClipIndex;
			gameData.objs.objects [nObject].rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][gameData.objs.objects [nObject].rType.vClipInfo.nClipIndex].xFrameTime;
			gameData.objs.objects [nObject].rType.vClipInfo.nCurFrame = 0;
			gameData.objs.objects [nObject].mType.physInfo.drag = 512;     //1024;
			gameData.objs.objects [nObject].mType.physInfo.mass = F1_0;
			VmVecZero (&gameData.objs.objects [nObject].mType.physInfo.velocity);
			}
		continue;
		}
	if (gameData.objs.objects [i].nType == OBJ_POWERUP) {
		if (gameData.objs.objects [i].id == POW_EXTRA_LIFE) {
			if (ng && !netGame.DoInvulnerability) {
				gameData.objs.objects [i].id = POW_SHIELD_BOOST;
				gameData.objs.objects [i].rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [gameData.objs.objects [i].id].nClipIndex;
				gameData.objs.objects [i].rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][gameData.objs.objects [i].rType.vClipInfo.nClipIndex].xFrameTime;
				}
			else {
				gameData.objs.objects [i].id = POW_INVULNERABILITY;
				gameData.objs.objects [i].rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [gameData.objs.objects [i].id].nClipIndex;
				gameData.objs.objects [i].rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][gameData.objs.objects [i].rType.vClipInfo.nClipIndex].xFrameTime;
				}
			}

		if (!(gameData.app.nGameMode & GM_MULTI_COOP))
			if ((gameData.objs.objects [i].id  >= POW_KEY_BLUE) && (gameData.objs.objects [i].id <= POW_KEY_GOLD)) {
				gameData.objs.objects [i].id = POW_SHIELD_BOOST;
				gameData.objs.objects [i].rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [gameData.objs.objects [i].id].nClipIndex;
				gameData.objs.objects [i].rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][gameData.objs.objects [i].rType.vClipInfo.nClipIndex].xFrameTime;
				}
		if (gameData.objs.objects [i].id == POW_INVULNERABILITY) {
			if (inv_count  >= 3 || (ng && !netGame.DoInvulnerability)) {
				gameData.objs.objects [i].id = POW_SHIELD_BOOST;
				gameData.objs.objects [i].rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [gameData.objs.objects [i].id].nClipIndex;
				gameData.objs.objects [i].rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][gameData.objs.objects [i].rType.vClipInfo.nClipIndex].xFrameTime;
				} 
			else
				inv_count++;
			}
		if (gameData.objs.objects [i].id == POW_CLOAK) {
			if (cloak_count  >= 3 || (ng && !netGame.DoCloak)) {
				gameData.objs.objects [i].id = POW_SHIELD_BOOST;
				gameData.objs.objects [i].rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [gameData.objs.objects [i].id].nClipIndex;
				gameData.objs.objects [i].rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][gameData.objs.objects [i].rType.vClipInfo.nClipIndex].xFrameTime;
				} 
			else
				cloak_count++;
			}
		if (gameData.objs.objects [i].id == POW_AFTERBURNER && ng && !netGame.DoAfterburner)
			BashToShield (i, "afterburner");
		if (gameData.objs.objects [i].id == POW_FUSION && ng && !netGame.DoFusions)
			BashToShield (i, "fusion");
		if (gameData.objs.objects [i].id == POW_PHOENIX && ng && !netGame.DoPhoenix)
			BashToShield (i, "phoenix");
		if (gameData.objs.objects [i].id == POW_HELIX && ng && !netGame.DoHelix)
			BashToShield (i, "helix");
		if (gameData.objs.objects [i].id == POW_MEGAMSL && ng && !netGame.DoMegas)
			BashToShield (i, "mega");
		if (gameData.objs.objects [i].id == POW_SMARTMSL && ng && !netGame.DoSmarts)
			BashToShield (i, "smartmissile");
		if (gameData.objs.objects [i].id == POW_GAUSS && ng && !netGame.DoGauss)
			BashToShield (i, "gauss");
		if (gameData.objs.objects [i].id == POW_VULCAN && ng && !netGame.DoVulcan)
			BashToShield (i, "vulcan");
		if (gameData.objs.objects [i].id == POW_PLASMA && ng && !netGame.DoPlasma)
			BashToShield (i, "plasma");
		if (gameData.objs.objects [i].id == POW_OMEGA && ng && !netGame.DoOmega)
			BashToShield (i, "omega");
		if (gameData.objs.objects [i].id == POW_SUPERLASER && ng && !netGame.DoSuperLaser)
			BashToShield (i, "superlaser");
		if (gameData.objs.objects [i].id == POW_PROXMINE && ng && !netGame.DoProximity)
			BashToShield (i, "proximity");
		// Special: Make all proximity bombs into shields if in
		// hoard mode because we use the proximity slot in the
		// tPlayer struct to signify how many orbs the tPlayer has.
		if (gameData.objs.objects [i].id == POW_PROXMINE && ng && (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)))
			BashToShield (i, "proximity");
		if (gameData.objs.objects [i].id == POW_SMARTMINE && ng && (gameData.app.nGameMode & GM_ENTROPY))
			BashToShield (i, "smart mine");
		if (gameData.objs.objects [i].id == POW_VULCAN_AMMO && ng && (!netGame.DoVulcan && !netGame.DoGauss))
			BashToShield (i, "vulcan ammo");
		if (gameData.objs.objects [i].id == POW_SPREADFIRE && ng && !netGame.DoSpread)
			BashToShield (i, "spread");
		if (gameData.objs.objects [i].id == POW_SMARTMINE && ng && !netGame.DoSmartMine)
			BashToShield (i, "smartmine");
		if (gameData.objs.objects [i].id == POW_FLASHMSL_1 && ng && !netGame.DoFlash)
			BashToShield (i, "flash");
		if (gameData.objs.objects [i].id == POW_FLASHMSL_4 && ng && !netGame.DoFlash)
			BashToShield (i, "flash");
		if (gameData.objs.objects [i].id == POW_GUIDEDMSL_1 && ng && !netGame.DoGuided)
			BashToShield (i, "guided");
		if (gameData.objs.objects [i].id == POW_GUIDEDMSL_4 && ng && !netGame.DoGuided)
			BashToShield (i, "guided");
		if (gameData.objs.objects [i].id == POW_EARTHSHAKER && ng && !netGame.DoEarthShaker)
			BashToShield (i, "earth");
		if (gameData.objs.objects [i].id == POW_MERCURYMSL_1 && ng && !netGame.DoMercury)
			BashToShield (i, "Mercury");
		if (gameData.objs.objects [i].id == POW_MERCURYMSL_4 && ng && !netGame.DoMercury)
			BashToShield (i, "Mercury");
		if (gameData.objs.objects [i].id == POW_CONVERTER && ng && !netGame.DoConverter)
			BashToShield (i, "Converter");
		if (gameData.objs.objects [i].id == POW_AMMORACK && ng && !netGame.DoAmmoRack)
			BashToShield (i, "Ammo rack");
		if (gameData.objs.objects [i].id == POW_HEADLIGHT && ng && 
			 (!netGame.DoHeadlight || (EGI_FLAG (bDarkness, 0, 0) && !EGI_FLAG (bHeadLights, 0, 0))))
			BashToShield (i, "Headlight");
		if (gameData.objs.objects [i].id == POW_LASER && ng && !netGame.DoLaserUpgrade)
			BashToShield (i, "Laser powerup");
		if (gameData.objs.objects [i].id == POW_HOMINGMSL_1 && ng && !netGame.DoHoming)
			BashToShield (i, "Homing");
		if (gameData.objs.objects [i].id == POW_HOMINGMSL_4 && ng && !netGame.DoHoming)
			BashToShield (i, "Homing");
		if (gameData.objs.objects [i].id == POW_QUADLASER && ng && !netGame.DoQuadLasers)
			BashToShield (i, "Quad Lasers");
		if (gameData.objs.objects [i].id == POW_BLUEFLAG && !(gameData.app.nGameMode & GM_CAPTURE))
			BashToShield (i, "Blue flag");
		if (gameData.objs.objects [i].id == POW_REDFLAG && !(gameData.app.nGameMode & GM_CAPTURE))
			BashToShield (i, "Red flag");
		}
	}
#ifdef RELEASE
if (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY | GM_MONSTERBALL))
#endif	
	InitHoardData ();
if (gameData.app.nGameMode & (GM_CAPTURE | GM_HOARD | GM_ENTROPY | GM_MONSTERBALL))
	MultiApplyGoalTextures ();
FindMonsterball ();	//will simply delete all Monsterballs for non-Monsterball games
MultiSortKillList ();
MultiShowPlayerList ();
gameData.objs.console->controlType = CT_FLYING;
ResetPlayerObject ();
}

//-----------------------------------------------------------------------------

int MultiFindGoalTexture (short t)
{
	int i;

if (t == TMI_FUELCEN)
	return 333;
for (i = 0;i<gameData.pig.tex.nTextures [gameStates.app.bD1Data];i++)
	if (gameData.pig.tex.pTMapInfo [i].flags & t)
		return i;
Int3 (); // Hey, there is no goal texture for this PIG!!!!
// Edit bitmaps.tbl and designate two textures to be RED and BLUE
// goal textures
return -1;
}


//-----------------------------------------------------------------------------
// override a segments texture with the owning team's textures.
// if nOldTexture  >= 0, only override textures equal to nOldTexture

void OverrideTextures (tSegment *segP, short nTexture, short nOldTexture, short nTexture2, int bFullBright, int bForce)
{
	int j, v;
	
nTexture = (nTexture < 0) ? -nTexture : MultiFindGoalTexture (nTexture);
nOldTexture = (nOldTexture < 0) ? -nOldTexture : MultiFindGoalTexture (nOldTexture);
if (nTexture >- 1)
	for (j = 0; j < 6; j++) {
		if (bForce || (segP->sides [j].nBaseTex == nOldTexture)) {
			segP->sides [j].nBaseTex = nTexture;
			if ((extraGameInfo [1].entropy.nOverrideTextures == 1) && 
				 (segP->sides [j].nOvlTex > 0) && (nTexture2 > 0))
				segP->sides [j].nOvlTex = nTexture2;
			if ((extraGameInfo [1].entropy.nOverrideTextures == 1) && bFullBright)
				for (v = 0; v < 4; v++)
					segP->sides [j].uvls [v].l = i2f (100);		//max out
			}
		}
if (bFullBright)
	gameData.segs.segment2s [SEG_IDX (segP)].static_light = i2f (100);	//make static light bright
}

//-----------------------------------------------------------------------------

#define	TMI_RED_TEAM	TMI_GOAL_RED	//-313
#define	TMI_BLUE_TEAM	TMI_GOAL_BLUE	//-333

int Goal_blue_segnum, Goal_red_segnum;

void ChangeSegmentTexture (int nSegment, int oldOwner)
{
	tSegment	*segP = gameData.segs.segments + nSegment;
	segment2 *seg2P = gameData.segs.segment2s + nSegment;
	xsegment *xSegP = gameData.segs.xSegments + nSegment;
	int		bFullBright = ((gameData.app.nGameMode & GM_HOARD) != 0) || ((gameData.app.nGameMode & GM_ENTROPY) && extraGameInfo [1].entropy.bBrightenRooms);
	static	short texOverrides [3] = {-313, TMI_BLUE_TEAM, TMI_RED_TEAM};

//if (oldOwner < 0)
//	oldOwner = segP->nOwner;
if ((gameData.app.nGameMode & GM_ENTROPY) && (extraGameInfo [1].entropy.nOverrideTextures == 2))
	return;
switch (seg2P->special) {
	case SEGMENT_IS_GOAL_BLUE:		
		Goal_blue_segnum = nSegment;
		OverrideTextures (segP, (short) ((gameData.app.nGameMode & GM_HOARD) ? TMI_GOAL_HOARD : TMI_GOAL_BLUE), -1, -1, bFullBright, 1);
		break;
		
	case SEGMENT_IS_GOAL_RED:
		Goal_red_segnum = nSegment;
		// Make both textures the same if Hoard mode
		OverrideTextures (segP, (short) ((gameData.app.nGameMode & GM_HOARD) ? TMI_GOAL_HOARD : TMI_GOAL_RED), -1, -1, bFullBright, 1);
		break;
		
	case SEGMENT_IS_ROBOTMAKER:
		if ((gameData.app.nGameMode & GM_ENTROPY) && (xSegP->owner >= 0))
			OverrideTextures (segP, texOverrides [xSegP->owner], 
									 (short) ((oldOwner < 0) ? -1 : texOverrides [oldOwner]), 316, bFullBright, oldOwner < 0);
		break;

	case SEGMENT_IS_REPAIRCEN:
		if ((gameData.app.nGameMode & GM_ENTROPY) && (xSegP->owner >= 0))
			OverrideTextures (segP, texOverrides [xSegP->owner], 
									 (short) ((oldOwner < 0) ? -1 : texOverrides [oldOwner]), 315, bFullBright, oldOwner < 0);
		break;
	
	case SEGMENT_IS_FUELCEN:
		if ((gameData.app.nGameMode & GM_ENTROPY) && (xSegP->owner >= 0))
			OverrideTextures (segP, texOverrides [xSegP->owner], 
								   (short) ((oldOwner < 0) ? -1 : texOverrides [oldOwner]), 314, bFullBright, oldOwner < 0);
		break;

	default:
		if ((gameData.app.nGameMode & GM_ENTROPY) && (xSegP->owner >= 0))
			OverrideTextures (segP, texOverrides [xSegP->owner], 
									 (short) ((oldOwner < 0) ? -1 : texOverrides [oldOwner]), -1, bFullBright, oldOwner < 0);
	}
}

//-----------------------------------------------------------------------------

void MultiApplyGoalTextures ()
{
	int		i;

if (!(gameData.app.nGameMode & GM_ENTROPY) || (extraGameInfo [1].entropy.nOverrideTextures == 1))
	for (i = 0; i <= gameData.segs.nLastSegment; i++)
		ChangeSegmentTexture (i, -1);
}

//-----------------------------------------------------------------------------

void MultiSetRobotAI (void)
{
	// Go through the objects array looking for robots and setting
	// them to certain supported types of NET AI behavior.

	//      int i;
	//
	//      for (i = 0; i <= gameData.objs.nLastObject; i++)
	// {
	//              if (gameData.objs.objects [i].nType == OBJ_ROBOT) {
	//                      gameData.objs.objects [i].aiInfo.REMOTE_OWNER = -1;
	//                      if (gameData.objs.objects [i].aiInfo.behavior == AIB_STATION)
	//                              gameData.objs.objects [i].aiInfo.behavior = AIB_NORMAL;
	//              }
	//      }
}

//-----------------------------------------------------------------------------

int MultiDeleteExtraObjects ()
{
	int i;
	int nnp = 0;
	tObject *objP;

// Go through the tObject list and remove any objects not used in
// 'Anarchy!' games.
// This function also prints the total number of available multiplayer
// positions in this level, even though this should always be 8 or more!

objP = gameData.objs.objects;
for (i = 0;i <= gameData.objs.nLastObject;i++) {
	if ((objP->nType == OBJ_PLAYER) || (objP->nType == OBJ_GHOST) || (objP->nType == OBJ_CAMBOT))
		nnp++;
	else if ((objP->nType == OBJ_ROBOT) && (gameData.app.nGameMode & GM_MULTI_ROBOTS))
		;
	else if ((objP->nType!=OBJ_NONE) && (objP->nType!=OBJ_PLAYER) && (objP->nType!=OBJ_POWERUP) && (objP->nType!=OBJ_CNTRLCEN) && (objP->nType!=OBJ_HOSTAGE) && !(objP->nType == OBJ_WEAPON && objP->id == SMALLMINE_ID)) {
		// Before deleting tObject, if it's a robot, drop it's special powerup, if any
		if (objP->nType == OBJ_ROBOT)
			if (objP->containsCount && (objP->containsType == OBJ_POWERUP))
				ObjectCreateEgg (objP);
		ReleaseObject ((short) i);
		}
	objP++;
	}
return nnp;
}

//-----------------------------------------------------------------------------

void ChangePlayerNumTo (int new_Player_num)
{
if (gameData.multi.nLocalPlayer > -1)
	memcpy (gameData.multi.players [new_Player_num].callsign, 
			  gameData.multi.players [gameData.multi.nLocalPlayer].callsign, 
			  CALLSIGN_LEN+1);
gameData.multi.nLocalPlayer = new_Player_num;
}

//-----------------------------------------------------------------------------

int MultiAllPlayersAlive ()
{
	int i;

for (i = 0; i < gameData.multi.nPlayers; i++)
	if (multiData.kills.pFlags [i] && gameData.multi.players [i].connected)
		return 0;
return 1;
}

//-----------------------------------------------------------------------------

void MultiInitiateSaveGame ()
{
	uint game_id;
	int i;
	ubyte slot;
	char filename [128];
	char desc [24];

if ((gameStates.app.bEndLevelSequence) || (gameData.reactor.bDestroyed))
	return;
if (!MultiAllPlayersAlive ()) {
	HUDInitMessage (TXT_SAVE_DEADPLRS);
	return;
 }
slot = StateGetSaveFile (filename, desc, 1);
if (!slot)
	return;
slot--;
// Make a unique game id
game_id = TimerGetFixedSeconds ();
game_id ^= gameData.multi.nPlayers<<4;
for (i = 0; i<gameData.multi.nPlayers; i++)
	game_id ^= *(uint *) gameData.multi.players [i].callsign;
if (game_id == 0) 
	game_id = 1; // 0 is invalid
MultiSendSaveGame (slot, game_id, desc);
MultiDoFrame ();
MultiSaveGame (slot, game_id, desc);
}

//-----------------------------------------------------------------------------

extern int StateGetGameId (char *);

void MultiInitiateRestoreGame ()
{
	ubyte slot;
	char filename [128];

if ((gameStates.app.bEndLevelSequence) || (gameData.reactor.bDestroyed))
	return;
if (!MultiAllPlayersAlive ()) {
	HUDInitMessage (TXT_LOAD_DEADPLRS);
	return;
	}
//StopTime ();
slot = StateGetRestoreFile (filename, 1);
if (!slot) {
	//StartTime ();
	return;
	}
gameData.app.nStateGameId = StateGetGameId (filename);
if (!gameData.app.nStateGameId)
	return;
slot--;
//StartTime ();
multi_send_restore_game (slot, gameData.app.nStateGameId);
MultiDoFrame ();
MultiRestoreGame (slot, gameData.app.nStateGameId);
}

//-----------------------------------------------------------------------------

void MultiSaveGame (ubyte slot, uint id, char *desc)
{
	char filename [128];

if ((gameStates.app.bEndLevelSequence) || (gameData.reactor.bDestroyed))
	return;
sprintf (filename, "%s.mg%d", gameData.multi.players [gameData.multi.nLocalPlayer].callsign, slot);
HUDInitMessage (TXT_SAVEGAME_NO, slot, desc);
StopTime ();
gameData.app.nStateGameId = id;
StateSaveAllSub (filename, desc, 0);
}

//-----------------------------------------------------------------------------

void MultiRestoreGame (ubyte slot, uint id)
{
	char filename [128];
	tPlayer saved_player;
	int nPlayer, i;
	uint thisid;

if ((gameStates.app.bEndLevelSequence) || (gameData.reactor.bDestroyed))
	return;
saved_player = gameData.multi.players [gameData.multi.nLocalPlayer];
sprintf (filename, "%s.mg%d", gameData.multi.players [gameData.multi.nLocalPlayer].callsign, slot);
gameData.app.bGamePaused = 1;
for (i = 0;i<gameData.multi.nPlayers;i++)
	MultiStripRobots (i);
thisid = StateGetGameId (filename);
if (thisid != id) {
	MultiBadRestore ();
	gameData.app.bGamePaused = 0;
	return;
	}
nPlayer = StateRestoreAllSub (filename, 1, 0);
gameData.app.bGamePaused = 0;
}

//-----------------------------------------------------------------------------

void extract_netplayer_stats (tNetPlayerStats *ps, tPlayer * pd)
{
	int i;

ps->flags = INTEL_INT (pd->flags);                                   // Powerup flags, see below...
ps->energy = (fix)INTEL_INT (pd->energy);                            // Amount of energy remaining.
ps->shields = (fix)INTEL_INT (pd->shields);                          // shields remaining (protection)
ps->lives = pd->lives;                                              // Lives remaining, 0 = game over.
ps->laserLevel = pd->laserLevel;                                  // Current level of the laser.
ps->primaryWeaponFlags = (ubyte) pd->primaryWeaponFlags;                  // bit set indicates the tPlayer has this weapon.
ps->secondaryWeaponFlags = (ubyte) pd->secondaryWeaponFlags;              // bit set indicates the tPlayer has this weapon.
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	ps->primaryAmmo [i] = INTEL_SHORT (pd->primaryAmmo [i]);
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
	ps->secondaryAmmo [i] = INTEL_SHORT (pd->secondaryAmmo [i]);
//memcpy (ps->primaryAmmo, pd->primaryAmmo, MAX_PRIMARY_WEAPONS*sizeof (short));        // How much ammo of each nType.
//memcpy (ps->secondaryAmmo, pd->secondaryAmmo, MAX_SECONDARY_WEAPONS*sizeof (short)); // How much ammo of each nType.
ps->last_score = INTEL_INT (pd->last_score);                           // Score at beginning of current level.
ps->score = INTEL_INT (pd->score);                                     // Current score.
ps->cloakTime = (fix)INTEL_INT (pd->cloakTime);                      // Time cloaked
ps->homingObjectDist = (fix)INTEL_INT (pd->homingObjectDist);      // Distance of nearest homing tObject.
ps->invulnerableTime = (fix)INTEL_INT (pd->invulnerableTime);        // Time invulnerable
ps->nKillGoalCount = INTEL_SHORT (pd->nKillGoalCount);
ps->netKilledTotal = INTEL_SHORT (pd->netKilledTotal);             // Number of times nKilled total
ps->netKillsTotal = INTEL_SHORT (pd->netKillsTotal);               // Number of net kills total
ps->numKillsLevel = INTEL_SHORT (pd->numKillsLevel);               // Number of kills this level
ps->numKillsTotal = INTEL_SHORT (pd->numKillsTotal);               // Number of kills total
ps->numRobotsLevel = INTEL_SHORT (pd->numRobotsLevel);             // Number of initial robots this level
ps->numRobotsTotal = INTEL_SHORT (pd->numRobotsTotal);             // Number of robots total
ps->hostages_rescuedTotal = INTEL_SHORT (pd->hostages_rescuedTotal); // Total number of hostages rescued.
ps->hostagesTotal = INTEL_SHORT (pd->hostagesTotal);                 // Total number of hostages.
ps->hostages_on_board = pd->hostages_on_board;                        // Number of hostages on ship.
}

//-----------------------------------------------------------------------------

void use_netplayer_stats (tPlayer * ps, tNetPlayerStats *pd)
{
	int i;

ps->flags = INTEL_INT (pd->flags);                       // Powerup flags, see below...
ps->energy = (fix)INTEL_INT ((int)pd->energy);           // Amount of energy remaining.
ps->shields = (fix)INTEL_INT ((int)pd->shields);         // shields remaining (protection)
ps->lives = pd->lives;                                  // Lives remaining, 0 = game over.
ps->laserLevel = pd->laserLevel;                      // Current level of the laser.
ps->primaryWeaponFlags = pd->primaryWeaponFlags;      // bit set indicates the tPlayer has this weapon.
ps->secondaryWeaponFlags = pd->secondaryWeaponFlags;  // bit set indicates the tPlayer has this weapon.
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	ps->primaryAmmo [i] = INTEL_SHORT (pd->primaryAmmo [i]);
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
	ps->secondaryAmmo [i] = INTEL_SHORT (pd->secondaryAmmo [i]);
//memcpy (ps->primaryAmmo, pd->primaryAmmo, MAX_PRIMARY_WEAPONS*sizeof (short));  // How much ammo of each nType.
//memcpy (ps->secondaryAmmo, pd->secondaryAmmo, MAX_SECONDARY_WEAPONS*sizeof (short)); // How much ammo of each nType.
ps->last_score = INTEL_INT (pd->last_score);             // Score at beginning of current level.
ps->score = INTEL_INT (pd->score);                       // Current score.
ps->cloakTime = (fix)INTEL_INT ((int)pd->cloakTime);   // Time cloaked
ps->homingObjectDist = (fix)INTEL_INT ((int)pd->homingObjectDist); // Distance of nearest homing tObject.
ps->invulnerableTime = (fix)INTEL_INT ((int)pd->invulnerableTime); // Time invulnerable
ps->nKillGoalCount = INTEL_SHORT (pd->nKillGoalCount);
ps->netKilledTotal = INTEL_SHORT (pd->netKilledTotal); // Number of times nKilled total
ps->netKillsTotal = INTEL_SHORT (pd->netKillsTotal); // Number of net kills total
ps->numKillsLevel = INTEL_SHORT (pd->numKillsLevel); // Number of kills this level
ps->numKillsTotal = INTEL_SHORT (pd->numKillsTotal); // Number of kills total
ps->numRobotsLevel = INTEL_SHORT (pd->numRobotsLevel); // Number of initial robots this level
ps->numRobotsTotal = INTEL_SHORT (pd->numRobotsTotal); // Number of robots total
ps->hostages_rescuedTotal = INTEL_SHORT (pd->hostages_rescuedTotal); // Total number of hostages rescued.
ps->hostagesTotal = INTEL_SHORT (pd->hostagesTotal);   // Total number of hostages.
ps->hostages_on_board = pd->hostages_on_board;            // Number of hostages on ship.
}

//-----------------------------------------------------------------------------

void MultiSendDropWeapon (int nObject, int seed)
{
	tObject *objP;
	int count = 0;
	int ammo_count;

if (nObject < 0)
	return;
objP = gameData.objs.objects + nObject;
ammo_count = objP->cType.powerupInfo.count;
if (objP->id == POW_OMEGA && ammo_count == F1_0)
	ammo_count = F1_0 - 1; //make fit in short
Assert (ammo_count < F1_0); //make sure fits in short
multiData.msg.buf [count++] = (char)MULTI_DROP_WEAPON;
multiData.msg.buf [count++] = (char)objP->id;
PUT_INTEL_SHORT (multiData.msg.buf+count, gameData.multi.nLocalPlayer); 
count += 2;
PUT_INTEL_SHORT (multiData.msg.buf+count, nObject); 
count += 2;
PUT_INTEL_SHORT (multiData.msg.buf+count, ammo_count); 
count += 2;
PUT_INTEL_INT (multiData.msg.buf+count, seed);
MapObjnumLocalToLocal (nObject);
#if 0
if (gameData.app.nGameMode & GM_NETWORK)
	gameData.multi.powerupsInMine [objP->id]++;
#endif
MultiSendData (multiData.msg.buf, 12, 2);
}

//-----------------------------------------------------------------------------

void MultiDoDropWeapon (char *buf)
{
	int 		nPlayer, ammo, nObject, nRemoteObj, seed;
	tObject	*objP;
	ubyte		powerupId;

powerupId = (ubyte) (buf [1]);
nPlayer = GET_INTEL_SHORT (buf + 2);
nRemoteObj = GET_INTEL_SHORT (buf + 4);
ammo = GET_INTEL_SHORT (buf + 6);
seed = GET_INTEL_INT (buf + 8);
objP = gameData.objs.objects + gameData.multi.players [nPlayer].nObject;
nObject = SpitPowerup (objP, powerupId, seed);
MapObjnumLocalToRemote (nObject, nRemoteObj, nPlayer);
if (nObject!=-1)
	gameData.objs.objects [nObject].cType.powerupInfo.count = ammo;
#if 0
if (gameData.app.nGameMode & GM_NETWORK)
	gameData.multi.powerupsInMine [powerupId]++;
#endif
}

//-----------------------------------------------------------------------------

void MultiSendGuidedInfo (tObject *miss, char done)
{
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	shortpos sp;
#endif
	int count = 0;

multiData.msg.buf [count++] = (char)MULTI_GUIDED;
multiData.msg.buf [count++] = (char)gameData.multi.nLocalPlayer;
multiData.msg.buf [count++] = done;
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
CreateShortPos ((shortpos *) (multiData.msg.buf+count), miss, 0);
count += sizeof (shortpos);
#else
CreateShortPos (&sp, miss, 1);
memcpy (& (multiData.msg.buf [count]), (ubyte *) (sp.bytemat), 9);
count += 9;
memcpy (& (multiData.msg.buf [count]), (ubyte *)& (sp.xo), 14);
count += 14;
#endif
MultiSendData (multiData.msg.buf, count, 0);
}

//-----------------------------------------------------------------------------

void MultiDoGuided (char *buf)
{
	char nPlayer = buf [1];
	int count = 3;
	static int fun = 200;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	shortpos sp;
#endif
	tObject *gmObj = gameData.objs.guidedMissile [nPlayer];

if (gmObj == NULL) {
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
	if ((gmObj < gameData.objs.objects) || 
		 (gmObj - gameData.objs.objects > gameData.objs.nLastObject)) {
		Int3 ();  // Get Jason immediately!
		return;
		}
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
ExtractShortPos (gmObj, (shortpos *) (buf+count), 0);
#else
memcpy ((ubyte *) (sp.bytemat), (ubyte *) (buf + count), 9);
memcpy ((ubyte *)& (sp.xo), (ubyte *) (buf + count + 9), 14);
ExtractShortPos (gmObj, &sp, 1);
#endif
count += sizeof (shortpos);
UpdateObjectSeg (gmObj);
}

//-----------------------------------------------------------------------------

void MultiSendStolenItems ()
{
	int i;

multiData.msg.buf [0] = MULTI_STOLEN_ITEMS;
for (i = 0; i < MAX_STOLEN_ITEMS; i++)
	multiData.msg.buf [i+1] = gameData.thief.stolenItems [i];
MultiSendData (multiData.msg.buf, MAX_STOLEN_ITEMS + 1, 1);
}

//-----------------------------------------------------------------------------

void MultiDoStolenItems (char *buf)
{
	int i;

for (i = 0; i < MAX_STOLEN_ITEMS; i++)
	gameData.thief.stolenItems [i] = buf [i+1];
}

//-----------------------------------------------------------------------------

extern void network_send_important_packet (char *, int);

void MultiSendWallStatus (int wallnum, ubyte nType, ubyte flags, ubyte state)
{
	int count = 0;

multiData.msg.buf [count++] = MULTI_WALL_STATUS;
PUT_INTEL_SHORT (multiData.msg.buf+count, wallnum);   
count += 2;
multiData.msg.buf [count++] = nType;
multiData.msg.buf [count++] = flags;
multiData.msg.buf [count++] = state;
MultiSendData (multiData.msg.buf, count, 1); // twice, just to be sure
MultiSendData (multiData.msg.buf, count, 1);
}

//-----------------------------------------------------------------------------

void MultiSendWallStatusSpecific (int nPlayer, int wallnum, ubyte nType, ubyte flags, ubyte state)
{
	// Send wall states a specific rejoining tPlayer
	short count = 0;

Assert (gameData.app.nGameMode & GM_NETWORK);
multiData.msg.buf [count++] = MULTI_WALL_STATUS;        
PUT_INTEL_SHORT (multiData.msg.buf+count, wallnum);  
count += 2;
multiData.msg.buf [count++] = nType;
multiData.msg.buf [count++] = flags;
multiData.msg.buf [count++] = state;
NetworkSendNakedPacket (multiData.msg.buf, count, nPlayer); // twice, just to be sure
NetworkSendNakedPacket (multiData.msg.buf, count, nPlayer);
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

Assert (wallnum >= 0);
gameData.walls.walls [wallnum].nType = nType;
gameData.walls.walls [wallnum].flags = flag;
gameData.walls.walls [wallnum].state = state;
if (gameData.walls.walls [wallnum].nType == WALL_OPEN) 
	DigiKillSoundLinkedToSegment (
		 (short) gameData.walls.walls [wallnum].nSegment, (short) 
		 gameData.walls.walls [wallnum].nSide, SOUND_FORCEFIELD_HUM);
}

//-----------------------------------------------------------------------------

void multi_send_jason_cheat (int num)
{
return;
}

//-----------------------------------------------------------------------------

void MultiSendKillGoalCounts ()
{
	int i, count = 1;
	multiData.msg.buf [0] = MULTI_KILLGOALS;

for (i = 0; i < MAX_PLAYERS; i++)
		multiData.msg.buf [count++] = (char)gameData.multi.players [i].nKillGoalCount;
MultiSendData (multiData.msg.buf, count, 1);
}

//-----------------------------------------------------------------------------

void MultiDoKillGoalCounts (char *buf)
{
	int i, count = 1;

for (i = 0; i < MAX_PLAYERS; i++)
	gameData.multi.players [i].nKillGoalCount = buf [count++];
}

//-----------------------------------------------------------------------------

void MultiSendHeartBeat ()
{
if (!netGame.PlayTimeAllowed)
	return;
multiData.msg.buf [0] = MULTI_HEARTBEAT;
PUT_INTEL_INT (multiData.msg.buf+1, ThisLevelTime);
MultiSendData (multiData.msg.buf, 5, 0);
}

//-----------------------------------------------------------------------------

void MultiDoHeartBeat (char *buf)
{
fix num = GET_INTEL_INT (buf + 1);
ThisLevelTime = num;
}

//-----------------------------------------------------------------------------

void MultiCheckForEntropyWinner ()
{
#if 1//def RELEASE
	xsegment *xSegP;
	int		h, i;
	char		t, bGotRoom [2] = {0, 0};

	static long		countDown;
	
if (!(gameData.app.nGameMode & GM_ENTROPY))
	return;
#if 1//def RELEASE
if (gameData.reactor.bDestroyed) {
	if (gameStates.app.nSDLTicks - countDown  >= 5000)
		StopEndLevelSequence ();
	return;
	}
#endif
countDown = -1;
gameStates.entropy.bExitSequence = 0;
for (i = 0, xSegP = gameData.segs.xSegments; i <= gameData.segs.nLastSegment; i++, xSegP++)
	if ((t = xSegP->owner) > 0) {
		bGotRoom [--t] = 1;
		if (bGotRoom [!t])
			return;
	}
for (t = 0; t < 2; t++)
	if (bGotRoom [t])
		break;
if (t == 2)	// no team as at least one room -> this is probably not an entropy enaszPlayerd level
	return;
for (h = i = 0; i < gameData.multi.nPlayers; i++)
	if (GetTeam (i) != t) {
		if (gameData.multi.players [i].secondaryAmmo [PROXIMITY_INDEX]  >= extraGameInfo [1].entropy.nCaptureVirusLimit)
			return;
		h += gameData.multi.players [i].secondaryAmmo [PROXIMITY_INDEX];
		}
if ((h  >= extraGameInfo [1].entropy.nCaptureVirusLimit) && extraGameInfo [1].entropy.nVirusStability)
	return;
HUDInitMessage (TXT_WINNING_TEAM, t ? TXT_RED : TXT_BLUE);
for (i = 0, xSegP = gameData.segs.xSegments; i <= gameData.segs.nLastSegment; i++, xSegP++) {
	if (xSegP->owner != t + 1)
		xSegP->owner = t + 1;
	ChangeSegmentTexture (i, -1);
	}
gameStates.entropy.bExitSequence = 1;
for (i = 0; i < gameData.multi.nPlayers; i++)
	if ((GetTeam (i) != t) && (gameData.multi.players [i].shields  >= 0))
		return;
countDown = gameStates.app.nSDLTicks;
#if 1//def RELEASE
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

void MultiCheckForKillGoalWinner ()
{
	int i, best = 0, bestnum = 0;
	tObject *objP;

if (gameData.reactor.bDestroyed)
	return;
for (i = 0; i < gameData.multi.nPlayers; i++)
	if (gameData.multi.players [i].nKillGoalCount>best) {
		best = gameData.multi.players [i].nKillGoalCount;
		bestnum = i;
		}
if (bestnum == gameData.multi.nLocalPlayer)
	HUDInitMessage (TXT_BEST_SCORE, best);
else
	HUDInitMessage (TXT_BEST_SCORE, gameData.multi.players [bestnum].callsign, best);
HUDInitMessage (TXT_CTRLCEN_DEAD);
objP = ObjFindFirstOfType (OBJ_CNTRLCEN);
NetDestroyReactor (objP);
}

//-----------------------------------------------------------------------------

void multi_send_seismic (fix start, fix end)
{
multiData.msg.buf [0] = MULTI_SEISMIC;
PUT_INTEL_INT (multiData.msg.buf + 1, start); 
PUT_INTEL_INT (multiData.msg.buf + 1 + sizeof (fix), end); 
MultiSendData (multiData.msg.buf, 1 + 2 * sizeof (fix), 1);
}

//-----------------------------------------------------------------------------

void MultiDoSeismic (char *buf)
{
gameStates.gameplay.seismic.nStartTime = GET_INTEL_INT (buf + 1);
gameStates.gameplay.seismic.nEndTime = GET_INTEL_INT (buf + 5);
DigiPlaySample (SOUND_SEISMIC_DISTURBANCE_START, F1_0);
}

//-----------------------------------------------------------------------------

void MultiSendLight (int nSegment, ubyte val)
{
int count = 1, i;
multiData.msg.buf [0] = MULTI_LIGHT;
PUT_INTEL_INT (multiData.msg.buf + count, nSegment); 
count += sizeof (int);
multiData.msg.buf [count++] = val;
for (i = 0; i < 6; i++,  count += 2)
	PUT_INTEL_SHORT (multiData.msg.buf+count, gameData.segs.segments [nSegment].sides [i].nOvlTex);
MultiSendData (multiData.msg.buf, count, 1);
}

//-----------------------------------------------------------------------------

void MultiSendLightSpecific (int nPlayer, int nSegment, ubyte val)
{
	short count = 1, i;

Assert (gameData.app.nGameMode & GM_NETWORK);
multiData.msg.buf [0] = MULTI_LIGHT;
PUT_INTEL_INT (multiData.msg.buf + count, nSegment); 
count += sizeof (int);
multiData.msg.buf [count++] = val;
for (i = 0; i < 6; i++, count += 2)
	PUT_INTEL_SHORT (multiData.msg.buf + count, gameData.segs.segments [nSegment].sides [i].nOvlTex); 
NetworkSendNakedPacket (multiData.msg.buf, count, nPlayer);
}

//-----------------------------------------------------------------------------

void MultiDoLight (char *buf)
{
	ubyte sides = buf [5];
	short i, seg = GET_INTEL_INT (buf + 1);

buf += 6;
for (i = 0; i < 6; i++, buf += 2) {
	if ((sides & (1 << i))) {
		SubtractLight (seg, i);
		gameData.segs.segments [seg].sides [i].nOvlTex = GET_INTEL_SHORT (buf);
		}
	}
}

//-----------------------------------------------------------------------------

void MultiDoFlags (char *buf)
{
	char nPlayer = buf [1];
	uint flags;

flags = GET_INTEL_INT (buf + 2);
if (nPlayer!=gameData.multi.nLocalPlayer)
	gameData.multi.players [nPlayer].flags = flags;
}

//-----------------------------------------------------------------------------

void MultiSendFlags (char nPlayer)
{
multiData.msg.buf [0] = MULTI_FLAGS;
multiData.msg.buf [1] = nPlayer;
PUT_INTEL_INT (multiData.msg.buf + 2, gameData.multi.players [(int) nPlayer].flags);
MultiSendData (multiData.msg.buf, 6, 1);
}

//-----------------------------------------------------------------------------

void MultiDoWeapons (char *buf)
{
	int	i, bufP = 1;
	char	nPlayer = buf [bufP++];

gameData.multi.players [nPlayer].shields  = 
gameData.objs.objects [gameData.multi.players [nPlayer].nObject].shields = GET_INTEL_INT (buf + bufP);
bufP += 4;
gameData.multi.players [(int) nPlayer].primaryWeaponFlags = GET_INTEL_SHORT (buf + bufP);
bufP += 2;
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++) {
	gameData.multi.players [(int) nPlayer].secondaryAmmo [i] = GET_INTEL_SHORT (buf + bufP);
	bufP += 2;
	}
gameData.multi.players [(int) nPlayer].laserLevel = multiData.msg.buf [bufP];
}

//-----------------------------------------------------------------------------

void MultiSendWeapons (int bForce)
{
	int t = gameStates.app.nSDLTicks;

	static int nTimeout = 0;

if (bForce || (t - nTimeout > 3000)) {
		int i, bufP = 0;

	nTimeout = t;
	multiData.msg.buf [bufP++] = (char) MULTI_WEAPONS;
	multiData.msg.buf [bufP++] = (char) gameData.multi.nLocalPlayer;
	PUT_INTEL_INT (multiData.msg.buf + bufP, gameData.multi.players [gameData.multi.nLocalPlayer].shields);
	bufP += 4;
	PUT_INTEL_SHORT (multiData.msg.buf + bufP, gameData.multi.players [gameData.multi.nLocalPlayer].primaryWeaponFlags);
	bufP += 2;
	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++) {
		PUT_INTEL_SHORT (multiData.msg.buf + bufP, gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [i]);
		bufP += 2;
		}
	multiData.msg.buf [bufP++] = gameData.multi.players [gameData.multi.nLocalPlayer].laserLevel;
	MultiSendData (multiData.msg.buf, bufP, 1);
	}
}

//-----------------------------------------------------------------------------

void MultiDoMonsterball (char *buf)
{
	int			bCreate, bufP = 1;
	short			nSegment;
	vmsVector	v;

bCreate = (int) buf [bufP++];
nSegment = GET_INTEL_SHORT (buf + bufP);
if (bCreate) {
	gameData.hoard.nMonsterballSeg = nSegment;
	CreateMonsterball ();
	return;
	}
bufP += 2;
v.p.x = GET_INTEL_INT (buf + bufP);
bufP += 4;
v.p.y = GET_INTEL_INT (buf + bufP);
bufP += 4;
v.p.z = GET_INTEL_INT (buf + bufP);
bufP += 4;
if (!gameData.hoard.monsterballP) {
	gameData.hoard.nMonsterballSeg = FindSegByPoint (&v, nSegment);
	CreateMonsterball ();
	gameData.hoard.monsterballP->position.vPos = v;
	}
v.p.x = GET_INTEL_INT (buf + bufP);
bufP += 4;
v.p.y = GET_INTEL_INT (buf + bufP);
bufP += 4;
v.p.z = GET_INTEL_INT (buf + bufP);
bufP += 4;
PhysApplyForce (gameData.hoard.monsterballP, &v);
v.p.x = GET_INTEL_INT (buf + bufP);
bufP += 4;
v.p.y = GET_INTEL_INT (buf + bufP);
bufP += 4;
v.p.z = GET_INTEL_INT (buf + bufP);
bufP += 4;
PhysApplyRot (gameData.hoard.monsterballP, &v);
}

//-----------------------------------------------------------------------------

void MultiSendMonsterball (int bForce, int bCreate)
{
	int t = gameStates.app.nSDLTicks;

	static int nTimeout = 0;

if (!(gameData.app.nGameMode & GM_MONSTERBALL))
	return;
if (!gameData.hoard.monsterballP)
	return;
if (bForce || (t - nTimeout > 1000)) {
		int bufP = 0;

	nTimeout = t;
	multiData.msg.buf [bufP++] = (char) MULTI_MONSTERBALL;
	multiData.msg.buf [bufP++] = (char) bCreate;
	PUT_INTEL_SHORT (multiData.msg.buf + bufP, gameData.hoard.monsterballP->position.nSegment);
	bufP += 2;
	PUT_INTEL_INT (multiData.msg.buf + bufP, gameData.hoard.monsterballP->position.vPos.p.x);
	bufP += 4;
	PUT_INTEL_INT (multiData.msg.buf + bufP, gameData.hoard.monsterballP->position.vPos.p.y);
	bufP += 4;
	PUT_INTEL_INT (multiData.msg.buf + bufP, gameData.hoard.monsterballP->position.vPos.p.z);
	bufP += 4;
	PUT_INTEL_INT (multiData.msg.buf + bufP, gameData.hoard.monsterballP->mType.physInfo.velocity.p.x);
	bufP += 4;
	PUT_INTEL_INT (multiData.msg.buf + bufP, gameData.hoard.monsterballP->mType.physInfo.velocity.p.y);
	bufP += 4;
	PUT_INTEL_INT (multiData.msg.buf + bufP, gameData.hoard.monsterballP->mType.physInfo.velocity.p.z);
	bufP += 4;
	PUT_INTEL_INT (multiData.msg.buf + bufP, gameData.hoard.monsterballP->mType.physInfo.rotVel.p.x);
	bufP += 4;
	PUT_INTEL_INT (multiData.msg.buf + bufP, gameData.hoard.monsterballP->mType.physInfo.rotVel.p.y);
	bufP += 4;
	PUT_INTEL_INT (multiData.msg.buf + bufP, gameData.hoard.monsterballP->mType.physInfo.rotVel.p.z);
	bufP += 4;
	MultiSendData (multiData.msg.buf, bufP, 1);
	}
}

//-----------------------------------------------------------------------------

void MultiSendDropBlobs (char nPlayer)
{
multiData.msg.buf [0] = MULTI_DROP_BLOB;
multiData.msg.buf [1] = nPlayer;
MultiSendData (multiData.msg.buf, 2, 0);
}

//-----------------------------------------------------------------------------

void MultiDoDropBlob (char *buf)
{
	char nPlayer = buf [1];
	
DropAfterburnerBlobs (&gameData.objs.objects [gameData.multi.players [nPlayer].nObject], 2, i2f (1), -1, NULL, 0);
}

//-----------------------------------------------------------------------------

void MultiSendPowerupUpdate ()
{
	int i;


multiData.msg.buf [0] = MULTI_POWERUP_UPDATE;
for (i = 0; i < MAX_POWERUP_TYPES; i++)
	multiData.msg.buf [i+1] = gameData.multi.maxPowerupsAllowed [i];
MultiSendData (multiData.msg.buf, MAX_POWERUP_TYPES+1, 1);
}

//-----------------------------------------------------------------------------

void MultiDoPowerupUpdate (char *buf)
{
	int i;

for (i = 0;i<MAX_POWERUP_TYPES;i++)
	if (buf [i+1]>gameData.multi.maxPowerupsAllowed [i])
		gameData.multi.maxPowerupsAllowed [i] = buf [i+1];
}

//-----------------------------------------------------------------------------

#if 0 // never used...
void MultiSendActiveDoor (int i)
{
	int count;

multiData.msg.buf [0] = MULTI_ACTIVE_DOOR;
multiData.msg.buf [1] = i;
multiData.msg.buf [2] = gameData.walls.nOpenDoors;
count = 3;
memcpy (multiData.msg.buf + 3, gameData.walls.activeDoors + i, sizeof (struct tActiveDoor);
count += sizeof (tActiveDoor);
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
{
tActiveDoor *ad = (tActiveDoor *) (multiData.msg.buf + 3);
ad->nPartCount = INTEL_INT (ad->nPartCount);
ad->nFrontWall [0] = INTEL_SHORT (ad->nFrontWall [0]);
ad->nFrontWall [1] = INTEL_SHORT (ad->nFrontWall [1]);
ad->nBackWall [0] = INTEL_SHORT (ad->nBackWall [0]);
ad->nBackWall [1] = INTEL_SHORT (ad->nBackWall [1]);
ad->time = INTEL_INT (ad->time);
}
#endif
//MultiSendData (multiData.msg.buf, sizeof (struct tActiveDoor)+3, 1);
MultiSendData (multiData.msg.buf, count, 1);
}
#endif // 0 (never used)

//-----------------------------------------------------------------------------

void MultiDoActiveDoor (char *buf)
{
	char i = multiData.msg.buf [1];
	
gameData.walls.nOpenDoors = buf [2];
memcpy (&gameData.walls.activeDoors [(int)i], buf+3, sizeof (struct tActiveDoor));
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
{
tActiveDoor *ad = gameData.walls.activeDoors + i;
ad->nPartCount = INTEL_INT (ad->nPartCount);
ad->nFrontWall [0] = INTEL_SHORT (ad->nFrontWall [0]);
ad->nFrontWall [1] = INTEL_SHORT (ad->nFrontWall [1]);
ad->nBackWall [0] = INTEL_SHORT (ad->nBackWall [0]);
ad->nBackWall [1] = INTEL_SHORT (ad->nBackWall [1]);
ad->time = INTEL_INT (ad->time);
}
#endif //WORDS_BIGENDIAN
}

//-----------------------------------------------------------------------------

void MultiSendSoundFunction (char whichfunc, char sound)
{
	int count = 0;

multiData.msg.buf [0] = MULTI_SOUND_FUNCTION;   
count++;
multiData.msg.buf [1] = gameData.multi.nLocalPlayer;             
count++;
multiData.msg.buf [2] = whichfunc;              
count++;
multiData.msg.buf [3] = sound; 
count++;       // this would probably work on the PC as well.  Jason?
MultiSendData (multiData.msg.buf, 4, 0);
}

//-----------------------------------------------------------------------------

#define AFTERBURNER_LOOP_START  20098
#define AFTERBURNER_LOOP_END    25776

void MultiDoSoundFunction (char *buf)
{
	// for afterburner
	char nPlayer, whichfunc;
	short sound;

if (gameData.multi.players [gameData.multi.nLocalPlayer].connected!=1)
	return;

nPlayer = buf [1];
whichfunc = buf [2];
sound = buf [3];
if (whichfunc == 0)
	DigiKillSoundLinkedToObject (gameData.multi.players [nPlayer].nObject);
else if (whichfunc == 3)
	DigiLinkSoundToObject3 (sound, (short) gameData.multi.players [nPlayer].nObject, 1, F1_0, i2f (256), AFTERBURNER_LOOP_START, AFTERBURNER_LOOP_END);
}

//-----------------------------------------------------------------------------

void MultiSendCaptureBonus (char nPlayer)
{
	Assert ((gameData.app.nGameMode & (GM_CAPTURE | GM_ENTROPY)) ||
			  ((gameData.app.nGameMode & GM_MONSTERBALL) == GM_MONSTERBALL));

multiData.msg.buf [0] = MULTI_CAPTURE_BONUS;
multiData.msg.buf [1] = nPlayer;
MultiSendData (multiData.msg.buf, 2, 1);
MultiDoCaptureBonus (multiData.msg.buf);
}

//-----------------------------------------------------------------------------

void MultiSendOrbBonus (char nPlayer)
{
Assert (gameData.app.nGameMode & GM_HOARD);
multiData.msg.buf [0] = MULTI_ORB_BONUS;
multiData.msg.buf [1] = nPlayer;
multiData.msg.buf [2] = (char) gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [PROXIMITY_INDEX];
MultiSendData (multiData.msg.buf, 3, 1);
MultiDoOrbBonus (multiData.msg.buf);
}

//-----------------------------------------------------------------------------

void MultiDoCaptureBonus (char *buf)
{
	// Figure out the results of a network kills and add it to the
	// appropriate tPlayer's tally.

	char 	nPlayer = buf [1];
	short	nTeam, nKillGoal, penalty = 0, bonus;
	//char	szTeam [20];

if (gameData.app.nGameMode & GM_HOARD)
	bonus = 5;
else if (gameData.app.nGameMode & GM_ENTROPY)
	bonus = 3;
else if (gameData.app.nGameMode & GM_MONSTERBALL)
	bonus = extraGameInfo [1].monsterball.nBonus;
else
	bonus = 1;
nKMatrixKillsChanged = 1;
if (nPlayer < 0) {
	penalty = 1;
	nPlayer = -nPlayer - 1;
	}
nTeam = GetTeam (nPlayer) ^ penalty;
if (nPlayer == gameData.multi.nLocalPlayer)
	HUDInitMessage (TXT_SCORED);
else
	HUDInitMessage (TXT_SCORED2, gameData.multi.players [nPlayer].callsign);
multiData.kills.nTeam [nTeam] += bonus;
if (nPlayer == gameData.multi.nLocalPlayer)
	DigiPlaySample (SOUND_HUD_YOU_GOT_GOAL, F1_0*2);
else if (GetTeam (nPlayer) == TEAM_RED)
	DigiPlaySample (SOUND_HUD_RED_GOT_GOAL, F1_0*2);
else
	DigiPlaySample (SOUND_HUD_BLUE_GOT_GOAL, F1_0*2);
gameData.multi.players [nPlayer].flags &= ~(PLAYER_FLAGS_FLAG);  // Clear capture flag
if (penalty) {
#if 0
	gameData.multi.players [nPlayer].netKillsTotal -= bonus;
	gameData.multi.players [nPlayer].nKillGoalCount -= bonus;
	if (netGame.KillGoal > 0) {
		nKillGoal = netGame.KillGoal * bonus;
		if (multiData.kills.nTeam [nTeam] >= nKillGoal) {
			sprintf (szTeam, "%s Team", nTeam ? TXT_RED : TXT_BLUE);
			HUDInitMessage (TXT_REACH_KILLGOAL, szTeam);
			HUDInitMessage (TXT_CTRLCEN_DEAD);
			NetDestroyReactor (ObjFindFirstOfType (OBJ_CNTRLCEN));
			}		
		}
#endif
	}
else {
	gameData.multi.players [nPlayer].netKillsTotal += bonus;
	gameData.multi.players [nPlayer].nKillGoalCount += bonus;
	if (netGame.KillGoal > 0) {
		nKillGoal = netGame.KillGoal * bonus;
		if (gameData.multi.players [nPlayer].nKillGoalCount >= nKillGoal) {
			if (nPlayer == gameData.multi.nLocalPlayer) {
				HUDInitMessage (TXT_REACH_KILLGOAL);
				gameData.multi.players [gameData.multi.nLocalPlayer].shields = i2f (200);
				}
			else
				HUDInitMessage (TXT_REACH_KILLGOAL, gameData.multi.players [nPlayer].callsign);
			HUDInitMessage (TXT_CTRLCEN_DEAD);
			NetDestroyReactor (ObjFindFirstOfType (OBJ_CNTRLCEN));
			}
		}
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
	// appropriate tPlayer's tally.

void MultiDoOrbBonus (char *buf)
{
	char nPlayer = buf [1];
	int nKillGoal;
	int bonus = GetOrbBonus (buf [2]);

nKMatrixKillsChanged = 1;
if (nPlayer == gameData.multi.nLocalPlayer)
	HUDInitMessage (TXT_SCORED_ORBS, bonus);
else
	HUDInitMessage (TXT_SCORED_ORBS2, gameData.multi.players [nPlayer].callsign, buf [2]);
if (nPlayer == gameData.multi.nLocalPlayer)
	DigiStartSoundQueued (SOUND_HUD_YOU_GOT_GOAL, F1_0*2);
else if (gameData.app.nGameMode & GM_TEAM) {
	if (GetTeam (nPlayer) == TEAM_RED)
		DigiPlaySample (SOUND_HUD_RED_GOT_GOAL, F1_0*2);
	else
		DigiPlaySample (SOUND_HUD_BLUE_GOT_GOAL, F1_0*2);
	}
else
	DigiPlaySample (SOUND_OPPONENT_HAS_SCORED, F1_0*2);
if (bonus>PhallicLimit) {
	if (nPlayer == gameData.multi.nLocalPlayer)
		HUDInitMessage (TXT_RECORD, bonus);
	else
		HUDInitMessage (TXT_RECORD2, gameData.multi.players [nPlayer].callsign, bonus);
	DigiPlaySample (SOUND_BUDDY_MET_GOAL, F1_0*2);
	PhallicMan = nPlayer;
	PhallicLimit = bonus;
	}
gameData.multi.players [nPlayer].flags &= ~(PLAYER_FLAGS_FLAG);  // Clear orb flag
multiData.kills.nTeam [GetTeam (nPlayer)] += bonus;
gameData.multi.players [nPlayer].netKillsTotal += bonus;
gameData.multi.players [nPlayer].nKillGoalCount += bonus;
multiData.kills.nTeam [GetTeam (nPlayer)] %= 1000;
gameData.multi.players [nPlayer].netKillsTotal %= 1000;
gameData.multi.players [nPlayer].nKillGoalCount %= 1000;
if (netGame.KillGoal>0) {
	nKillGoal = netGame.KillGoal*5;
	if (gameData.multi.players [nPlayer].nKillGoalCount >= nKillGoal) {
		if (nPlayer == gameData.multi.nLocalPlayer) {
			HUDInitMessage (TXT_REACH_KILLGOAL);
			gameData.multi.players [gameData.multi.nLocalPlayer].shields = i2f (200);
			}
		else
			HUDInitMessage (TXT_REACH_KILLGOAL2, gameData.multi.players [nPlayer].callsign);
		HUDInitMessage (TXT_CTRLCEN_DEAD);
		NetDestroyReactor (ObjFindFirstOfType (OBJ_CNTRLCEN));
		}
	}
MultiSortKillList ();
MultiShowPlayerList ();
}

//-----------------------------------------------------------------------------

void MultiSendShields (void)
{
multiData.msg.buf [0] = MULTI_PLAYER_SHIELDS;
multiData.msg.buf [1] = gameData.multi.nLocalPlayer;
PUT_INTEL_INT (multiData.msg.buf+2, gameData.multi.players [gameData.multi.nLocalPlayer].shields);
MultiSendData (multiData.msg.buf, 6, 1);
}

//-----------------------------------------------------------------------------

void MultiDoCheating (char *buf)
{
	char nPlayer = buf [1];

HUDInitMessage (TXT_PLAYER_CHEATED, gameData.multi.players [nPlayer].callsign);
DigiPlaySampleLooped (SOUND_CONTROL_CENTER_WARNING_SIREN, F1_0, 3);
}

//-----------------------------------------------------------------------------

void MultiSendCheating (void)
{
multiData.msg.buf [0] = MULTI_CHEATING;
multiData.msg.buf [1] = gameData.multi.nLocalPlayer;
MultiSendData (multiData.msg.buf, 2, 1);
}

//-----------------------------------------------------------------------------

void MultiSendGotFlag (char nPlayer)
{
multiData.msg.buf [0] = MULTI_GOT_FLAG;
multiData.msg.buf [1] = nPlayer;
DigiStartSoundQueued (SOUND_HUD_YOU_GOT_FLAG, F1_0*2);
MultiSendData (multiData.msg.buf, 2, 1);
MultiSendFlags ((char) gameData.multi.nLocalPlayer);
}

//-----------------------------------------------------------------------------

int SoundHacked = 0;
tDigiSound ReversedSound;

void MultiSendGotOrb (char nPlayer)
{
multiData.msg.buf [0] = MULTI_GOT_ORB;
multiData.msg.buf [1] = nPlayer;
DigiPlaySample (SOUND_YOU_GOT_ORB, F1_0*2);
MultiSendData (multiData.msg.buf, 2, 1);
MultiSendFlags ((char) gameData.multi.nLocalPlayer);
}

//-----------------------------------------------------------------------------

void MultiDoGotFlag (char *buf)
{
	char nPlayer = buf [1];

if (nPlayer == gameData.multi.nLocalPlayer)
	DigiStartSoundQueued (SOUND_HUD_YOU_GOT_FLAG, F1_0*2);
else if (GetTeam (nPlayer) == TEAM_RED)
	DigiStartSoundQueued (SOUND_HUD_RED_GOT_FLAG, F1_0*2);
else
	DigiStartSoundQueued (SOUND_HUD_BLUE_GOT_FLAG, F1_0*2);
gameData.multi.players [nPlayer].flags |= PLAYER_FLAGS_FLAG;
ResetFlightPath (&gameData.pig.flags [!GetTeam (nPlayer)].path, 10, -1);
HUDInitMessage (TXT_PICKFLAG2, gameData.multi.players [nPlayer].callsign);
}

//-----------------------------------------------------------------------------

void MultiDoGotOrb (char *buf)
{
	char nPlayer = buf [1];

Assert (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY));
if (gameData.app.nGameMode & GM_TEAM) {
	if (GetTeam (nPlayer) == GetTeam (gameData.multi.nLocalPlayer))
		DigiPlaySample (SOUND_FRIEND_GOT_ORB, F1_0*2);
	else
		DigiPlaySample (SOUND_OPPONENT_GOT_ORB, F1_0*2);
   }
else
	DigiPlaySample (SOUND_OPPONENT_GOT_ORB, F1_0*2);
gameData.multi.players [nPlayer].flags|= PLAYER_FLAGS_FLAG;
if (gameData.app.nGameMode & GM_ENTROPY)
	HUDInitMessage (TXT_PICKVIRUS2, gameData.multi.players [nPlayer].callsign);
else
	HUDInitMessage (TXT_PICKORB2, gameData.multi.players [nPlayer].callsign);
}

//-----------------------------------------------------------------------------

void DropOrb ()
{
	int nObject, seed;

if (!(gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)))
	Int3 (); // How did we get here? Get Leighton!
if (!gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [PROXIMITY_INDEX]) {
	HUDInitMessage ((gameData.app.nGameMode & GM_HOARD) ? TXT_NO_ORBS : TXT_NO_VIRUS);
	return;
	}
seed = d_rand ();
nObject = SpitPowerup (gameData.objs.console, POW_HOARD_ORB, seed);
if (nObject < 0)
	return;
HUDInitMessage ((gameData.app.nGameMode & GM_HOARD) ? TXT_DROP_ORB : TXT_DROP_VIRUS);
DigiPlaySample (SOUND_DROP_WEAPON, F1_0);
if (nObject > -1)
	if (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY))
		MultiSendDropFlag (nObject, seed);
// If empty, tell everyone to stop drawing the box around me
if (!--gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [PROXIMITY_INDEX])
	MultiSendFlags ((char) gameData.multi.nLocalPlayer);
}

//-----------------------------------------------------------------------------

void DropFlag ()
{
	int nObject, seed;

if (!(gameData.app.nGameMode & GM_CAPTURE) && !(gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)))
	return;
if (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)) {
	DropOrb ();
	return;
	}
if (!(gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_FLAG)) {
	HUDInitMessage (TXT_NO_FLAG);
	return;
	}
HUDInitMessage (TXT_DROP_FLAG);
DigiPlaySample (SOUND_DROP_WEAPON, F1_0);
seed = d_rand ();
nObject = SpitPowerup (gameData.objs.console, (ubyte) ((GetTeam (gameData.multi.nLocalPlayer) == TEAM_RED) ? POW_BLUEFLAG : POW_REDFLAG), seed);
if (nObject < 0)
	return;
if ((gameData.app.nGameMode & GM_CAPTURE) && nObject>-1)
	MultiSendDropFlag (nObject, seed);
gameData.multi.players [gameData.multi.nLocalPlayer].flags &= ~ (PLAYER_FLAGS_FLAG);
}

//-----------------------------------------------------------------------------

void MultiSendDropFlag (int nObject, int seed)
{
	tObject *objP;
	int count = 0;

objP = &gameData.objs.objects [nObject];
multiData.msg.buf [count++] = (char)MULTI_DROP_FLAG;
multiData.msg.buf [count++] = (char)objP->id;
PUT_INTEL_SHORT (multiData.msg.buf+count, gameData.multi.nLocalPlayer); 
count += 2;
PUT_INTEL_SHORT (multiData.msg.buf+count, nObject); 
count += 2;
PUT_INTEL_SHORT (multiData.msg.buf+count, objP->cType.powerupInfo.count); 
count += 2;
PUT_INTEL_INT (multiData.msg.buf+count, seed);
MapObjnumLocalToLocal (nObject);
#if 0
if (!(gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)))
	if (gameData.app.nGameMode & GM_NETWORK)
		gameData.multi.powerupsInMine [objP->id]++;
#endif		
MultiSendData (multiData.msg.buf, 12, 2);
}

//-----------------------------------------------------------------------------

void MultiDoDropFlag (char *buf)
{
	int nPlayer, ammo, nObject, nRemoteObj, seed;
	tObject *objP;
	ubyte powerupId;

powerupId = (ubyte) buf [1];
nPlayer = GET_INTEL_SHORT (buf + 2);
nRemoteObj = GET_INTEL_SHORT (buf + 4);
ammo = GET_INTEL_SHORT (buf + 6);
seed = GET_INTEL_INT (buf + 8);

objP = gameData.objs.objects + gameData.multi.players [nPlayer].nObject;
nObject = SpitPowerup (objP, powerupId, seed);
MapObjnumLocalToRemote (nObject, nRemoteObj, nPlayer);
if (nObject!=-1)
	gameData.objs.objects [nObject].cType.powerupInfo.count = ammo;
if (gameData.app.nGameMode & GM_ENTROPY)
	gameData.objs.objects [nObject].matCenCreator = GetTeam (nPlayer) + 1;
else if (!(gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY))) {
#if 0	
	if (gameData.app.nGameMode & GM_NETWORK)
		gameData.multi.powerupsInMine [powerupId]++;
#endif		
	gameData.multi.players [nPlayer].flags &= ~ (PLAYER_FLAGS_FLAG);
	}
}

//-----------------------------------------------------------------------------

void MultiBadRestore ()
{
SetFunctionMode (FMODE_MENU);
ExecMessageBox (NULL, NULL, 1, TXT_OK, 
	            "A multi-save game was restored\nthat you are missing or does not\nmatch that of the others.\nYou must rejoin if you wish to\ncontinue.");
SetFunctionMode (FMODE_GAME);
multiData.bQuitGame = 1;
multiData.menu.bLeave = 1;
MultiResetStuff ();
}

//-----------------------------------------------------------------------------

void MultiSendRobotControls (char nPlayer)
{
	int count = 2;

multiData.msg.buf [0] = MULTI_ROBOT_CONTROLS;
multiData.msg.buf [1] = nPlayer;
memcpy (& (multiData.msg.buf [count]), &multiData.robots.controlled, MAX_ROBOTS_CONTROLLED*4);
count +=  (MAX_ROBOTS_CONTROLLED*4);
memcpy (& (multiData.msg.buf [count]), &multiData.robots.agitation, MAX_ROBOTS_CONTROLLED*4);
count +=  (MAX_ROBOTS_CONTROLLED*4);
memcpy (& (multiData.msg.buf [count]), &multiData.robots.controlledTime, MAX_ROBOTS_CONTROLLED*4);
count +=  (MAX_ROBOTS_CONTROLLED*4);
memcpy (& (multiData.msg.buf [count]), &multiData.robots.lastSendTime, MAX_ROBOTS_CONTROLLED*4);
count +=  (MAX_ROBOTS_CONTROLLED*4);
memcpy (& (multiData.msg.buf [count]), &multiData.robots.lastMsgTime, MAX_ROBOTS_CONTROLLED*4);
count +=  (MAX_ROBOTS_CONTROLLED*4);
memcpy (& (multiData.msg.buf [count]), &multiData.robots.sendPending, MAX_ROBOTS_CONTROLLED*4);
count +=  (MAX_ROBOTS_CONTROLLED*4);
memcpy (& (multiData.msg.buf [count]), &multiData.robots.fired, MAX_ROBOTS_CONTROLLED*4);
count +=  (MAX_ROBOTS_CONTROLLED*4);
NetworkSendNakedPacket (multiData.msg.buf, 142, nPlayer);
}

//-----------------------------------------------------------------------------

void MultiDoRobotControls (char *buf)
{
	int count = 2;

if (buf [1]!=gameData.multi.nLocalPlayer) {
	Int3 (); // Get Jason!  Recieved a coop_sync that wasn't ours!
	return;
	}
memcpy (&multiData.robots.controlled, buf + count, MAX_ROBOTS_CONTROLLED*4);
count +=  (MAX_ROBOTS_CONTROLLED*4);
memcpy (&multiData.robots.agitation, buf + count, MAX_ROBOTS_CONTROLLED*4);
count +=  (MAX_ROBOTS_CONTROLLED*4);
memcpy (&multiData.robots.controlledTime, buf + count, MAX_ROBOTS_CONTROLLED*4);
count +=  (MAX_ROBOTS_CONTROLLED*4);
memcpy (&multiData.robots.lastSendTime, buf + count, MAX_ROBOTS_CONTROLLED*4);
count +=  (MAX_ROBOTS_CONTROLLED*4);
memcpy (&multiData.robots.lastMsgTime, buf + count, MAX_ROBOTS_CONTROLLED*4);
count +=  (MAX_ROBOTS_CONTROLLED*4);
memcpy (&multiData.robots.sendPending, buf + count, MAX_ROBOTS_CONTROLLED*4);
count +=  (MAX_ROBOTS_CONTROLLED*4);
memcpy (&multiData.robots.fired, buf + count, MAX_ROBOTS_CONTROLLED*4);
count +=  (MAX_ROBOTS_CONTROLLED*4);
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

int MultiPowerupIsAllowed (int id)
{
switch (id) {
	case POW_INVULNERABILITY: 
		if (!netGame.DoInvulnerability)
			return (0);
		break;
	case POW_CLOAK: 
		if (!netGame.DoCloak)
			return (0);
		break;
	case POW_AFTERBURNER: 
		if (!netGame.DoAfterburner)
			return (0);
		break;
	case POW_FUSION: 
		if (!netGame.DoFusions)
			return (0);
		break;
	case POW_PHOENIX: 
		if (!netGame.DoPhoenix)
			return (0);
		break;
	case POW_HELIX: 
		if (!netGame.DoHelix)
			return (0);
		break;
	case POW_MEGAMSL: 
		if (!netGame.DoMegas)
			return (0);
		break;
	case POW_SMARTMSL: 
		if (!netGame.DoSmarts)
			return (0);
		break;
	case POW_GAUSS: 
		if (!netGame.DoGauss)
			return (0);
		break;
	case POW_VULCAN: 
		if (!netGame.DoVulcan)
			return (0);
		break;
	case POW_PLASMA: 
		if (!netGame.DoPlasma)
			return (0);
		break;
	case POW_OMEGA: 
		if (!netGame.DoOmega)
			return (0);
		break;
	case POW_SUPERLASER: 
		if (!netGame.DoSuperLaser)
			return (0);
		break;
	case POW_PROXMINE: 
		if (!netGame.DoProximity)
			return (0);
		break;
	case POW_VULCAN_AMMO: 
		if (!(netGame.DoVulcan || netGame.DoGauss))
			return (0);
		break;
	case POW_SPREADFIRE: 
		if (!netGame.DoSpread)
			return (0);
		break;
	case POW_SMARTMINE: 
		if (!netGame.DoSmartMine)
			return (0);
		break;
	case POW_FLASHMSL_1: 
		if (!netGame.DoFlash)
			return (0);
		break;
	case POW_FLASHMSL_4: 
		if (!netGame.DoFlash)
			return (0);
		break;
	case POW_GUIDEDMSL_1: 
		if (!netGame.DoGuided)
			return (0);
		break;
	case POW_GUIDEDMSL_4: 
		if (!netGame.DoGuided)
			return (0);
		break;
	case POW_EARTHSHAKER: 
		if (!netGame.DoEarthShaker)
			return (0);
		break;
	case POW_MERCURYMSL_1: 
		if (!netGame.DoMercury)
			return (0);
		break;
	case POW_MERCURYMSL_4: 
		if (!netGame.DoMercury)
			return (0);
		break;
	case POW_CONVERTER: 
		if (!netGame.DoConverter)
			return (0);
		break;
	case POW_AMMORACK: 
		if (!netGame.DoAmmoRack)
			return (0);
		break;
	case POW_HEADLIGHT: 
		if (!netGame.DoHeadlight)
			return (0);
		break;
	case POW_LASER: 
		if (!netGame.DoLaserUpgrade)
			return (0);
		break;
	case POW_HOMINGMSL_1: 
		if (!netGame.DoHoming)
			return (0);
		break;
	case POW_HOMINGMSL_4: 
		if (!netGame.DoHoming)
			return (0);
		break;
	case POW_QUADLASER: 
		if (!netGame.DoQuadLasers)
			return (0);
		break;
	case POW_BLUEFLAG: 
		if (!(gameData.app.nGameMode & GM_CAPTURE))
			return (0);
		break;
	case POW_REDFLAG: 
		if (!(gameData.app.nGameMode & GM_CAPTURE))
			return (0);
		break;
	}
return (1);
}

//-----------------------------------------------------------------------------

void MultiSendFinishGame ()
{
	multiData.msg.buf [0] = MULTI_FINISH_GAME;
	multiData.msg.buf [1] = gameData.multi.nLocalPlayer;

	MultiSendData (multiData.msg.buf, 2, 1);
}

//-----------------------------------------------------------------------------

extern void DoFinalBossHacks ();

void MultiDoFinishGame (char *buf)
{
if (buf [0]!=MULTI_FINISH_GAME)
	return;
if (gameData.missions.nCurrentLevel!=gameData.missions.nLastLevel)
	return;
DoFinalBossHacks ();
}

//-----------------------------------------------------------------------------

void MultiSendTriggerSpecific (char nPlayer, ubyte trig)
{
	multiData.msg.buf [0] = MULTI_START_TRIGGER;
	multiData.msg.buf [1] = trig;

NetworkSendNakedPacket (multiData.msg.buf, 2, nPlayer);
}

//-----------------------------------------------------------------------------

void MultiDoStartTrigger (char *buf)
{
gameData.trigs.triggers [(int) ((ubyte) buf [1])].flags |= TF_DISABLED;
}

//-----------------------------------------------------------------------------
// This function adds a kill to lifetime stats of this tPlayer, and possibly
// gives a promotion.  If so, it will tell everyone else

void MultiAddLifetimeKills ()
{

	int oldrank;

if (!gameData.app.nGameMode & GM_NETWORK)
	return;
oldrank = GetMyNetRanking ();
networkData.nNetLifeKills++;
if (oldrank!=GetMyNetRanking ()) {
	MultiSendRanking ();
	if (!gameOpts->multi.bNoRankings) {
		HUDInitMessage (TXT_PROMOTED, pszRankStrings [GetMyNetRanking ()]);
		DigiPlaySample (SOUND_BUDDY_MET_GOAL, F1_0*2);
		netPlayers.players [gameData.multi.nLocalPlayer].rank = GetMyNetRanking ();
		}
	}
WritePlayerFile ();
}

//-----------------------------------------------------------------------------

void MultiAddLifetimeKilled ()
{
	// This function adds a "nKilled" to lifetime stats of this tPlayer, and possibly
	// gives a demotion.  If so, it will tell everyone else

	int oldrank;

if (!gameData.app.nGameMode & GM_NETWORK)
	return;
oldrank = GetMyNetRanking ();
networkData.nNetLifeKilled++;
if (oldrank!=GetMyNetRanking ()) {
	MultiSendRanking ();
	netPlayers.players [gameData.multi.nLocalPlayer].rank = GetMyNetRanking ();
	if (!gameOpts->multi.bNoRankings)
		HUDInitMessage (TXT_DEMOTED, pszRankStrings [GetMyNetRanking ()]);
	}
WritePlayerFile ();
}

//-----------------------------------------------------------------------------

void MultiSendRanking ()
{
	multiData.msg.buf [0] = (char)MULTI_RANK;
	multiData.msg.buf [1] = (char)gameData.multi.nLocalPlayer;
	multiData.msg.buf [2] = (char)GetMyNetRanking ();

	MultiSendData (multiData.msg.buf, 3, 1);
}

//-----------------------------------------------------------------------------

void MultiDoRanking (char *buf)
{
	char rankstr [20];
	char nPlayer = buf [1];
	char rank = buf [2];

	if (netPlayers.players [nPlayer].rank<rank)
		strcpy (rankstr, TXT_RANKUP);
	else if (netPlayers.players [nPlayer].rank>rank)
		strcpy (rankstr, TXT_RANKDOWN);
	else
		return;

	netPlayers.players [nPlayer].rank = rank;

	if (!gameOpts->multi.bNoRankings)
		HUDInitMessage (TXT_RANKCHANGE2, gameData.multi.players [nPlayer].callsign, 
								rankstr, pszRankStrings [(int)rank]);
}

//-----------------------------------------------------------------------------

void MultiSendModemPing ()
{
multiData.msg.buf [0] = MULTI_MODEM_PING;
MultiSendData (multiData.msg.buf, 1, 1);
}

//-----------------------------------------------------------------------------

void MultiSendModemPingReturn (char *buf)
{
multiData.msg.buf [0] = MULTI_MODEM_PING_RETURN;
MultiSendData (multiData.msg.buf, 1, 1);
}

//-----------------------------------------------------------------------------

void  MultiDoModemPingReturn (char *buf)
{
if (pingStats [0].launchTime)
	return;
xPingReturnTime = TimerGetFixedSeconds ();
HUDInitMessage (TXT_PINGTIME, 
					 f2i (FixMul (xPingReturnTime - pingStats [0].launchTime, i2f (1000))));
pingStats [0].launchTime = 0;
}

//-----------------------------------------------------------------------------

void MultiQuickSoundHack (int num)
{
	int length, i;

num = DigiXlatSound ((short) num);
length = gameData.pig.snd.sounds [gameOpts->sound.bD1Sound][num].length;
ReversedSound.data = (ubyte *)d_malloc (length);
ReversedSound.length = length;
for (i = 0; i < length; i++)
	ReversedSound.data [i] = gameData.pig.snd.sounds [gameOpts->sound.bD1Sound][num].data [length-i-1];
SoundHacked = 1;
}

//-----------------------------------------------------------------------------

void MultiSendPlayByPlay (int num, int spnum, int dpnum)
{
#if 0
if (!(gameData.app.nGameMode & GM_HOARD))
	return;
multiData.msg.buf [0] = MULTI_PLAY_BY_PLAY;
multiData.msg.buf [1] = (char)num;
multiData.msg.buf [2] = (char)spnum;
multiData.msg.buf [3] = (char)dpnum;
MultiSendData (multiData.msg.buf, 4, 1);
MultiDoPlayByPlay (multiData.msg.buf);
#endif
}

//-----------------------------------------------------------------------------

void MultiDoPlayByPlay (char *buf)
{
	int whichplay = buf [1];
	int spnum = buf [2];
	int dpnum = buf [3];

if (!(gameData.app.nGameMode & GM_HOARD)) {
	Int3 (); // Get Leighton, something bad has happened.
	return;
	}
switch (whichplay) {
	case 0: // Smacked!
		HUDInitMessage (TXT_SMACKED, gameData.multi.players [dpnum].callsign, gameData.multi.players [spnum].callsign);
		break;
	case 1: // Spanked!
		HUDInitMessage (TXT_SPANKED, gameData.multi.players [dpnum].callsign, gameData.multi.players [spnum].callsign);
		break;
	default:
		Int3 ();
	}
}

//-----------------------------------------------------------------------------

void MultiDoReturnFlagHome (char *buf)
{
	tObject	*pObj = gameData.objs.objects;
	int		i;
	ushort	nType = buf [1];
	ushort	id = buf [2];

for (i = 0; i < gameFileInfo.objects.count; i++, pObj = gameData.objs.objects + pObj->next) {
	if ((pObj->nType == nType) && (pObj->id == id)) {
		ReturnFlagHome (pObj);
		break;
		}
	}
}

//-----------------------------------------------------------------------------

void MultiSendReturnFlagHome (short nObject)
{
multiData.msg.buf [0] = (char) MULTI_RETURN_FLAG;
multiData.msg.buf [1] = (char) gameData.objs.objects [nObject].nType;
multiData.msg.buf [2] = (char) gameData.objs.objects [nObject].id;
MultiSendData (multiData.msg.buf, 3, 0);
}

//-----------------------------------------------------------------------------

void MultiDoConquerWarning (char *buf)
{
DigiPlaySample (SOUND_CONTROL_CENTER_WARNING_SIREN, F3_0);
}

//-----------------------------------------------------------------------------

void MultiSendConquerWarning ()
{
multiData.msg.buf [0] = (char) MULTI_CONQUER_WARNING;
multiData.msg.buf [1] = 0; // dummy values
multiData.msg.buf [2] = 0;
MultiSendData (multiData.msg.buf, 3, 0);
}

//-----------------------------------------------------------------------------

void MultiDoStopConquerWarning (char *buf)
{
DigiStopSound (SOUND_CONTROL_CENTER_WARNING_SIREN);
}

//-----------------------------------------------------------------------------

void MultiSendStopConquerWarning ()
{
multiData.msg.buf [0] = (char) MULTI_STOP_CONQUER_WARNING;
multiData.msg.buf [1] = 0; // dummy values
multiData.msg.buf [2] = 0;
MultiSendData (multiData.msg.buf, 3, 0);
}

//-----------------------------------------------------------------------------

void MultiDoConquerRoom (char *buf)
{
ConquerRoom (buf [1], buf [2], buf [3]);
}

//-----------------------------------------------------------------------------

void MultiSendConquerRoom (char nOwner, char prevOwner, char group)
{
multiData.msg.buf [0] = (char) MULTI_CONQUER_ROOM;
multiData.msg.buf [1] = nOwner;
multiData.msg.buf [2] = prevOwner;
multiData.msg.buf [3] = group;
MultiSendData (multiData.msg.buf, 4, 0);
}

//-----------------------------------------------------------------------------

void MultiSendTeleport (char nPlayer, short nSegment, char nSide)
{
multiData.msg.buf [0] = (char) MULTI_TELEPORT;
multiData.msg.buf [1] = nPlayer; // dummy values
* ((short *) (multiData.msg.buf + 2)) = INTEL_SHORT (nSegment);
multiData.msg.buf [4] = nSide; // dummy values
MultiSendData (multiData.msg.buf, 5, 0);
}

//-----------------------------------------------------------------------------

void MultiDoTeleport (char *buf)
{
	short	nObject = gameData.multi.players [buf [1]].nObject;
	short nSegment = GET_INTEL_SHORT (buf + 2);
//	short	nSide = buf [4];

TriggerSetObjPos (nObject, nSegment);
CreatePlayerAppearanceEffect (gameData.objs.objects + nObject);
}

//-----------------------------------------------------------------------------

tMultiHandlerInfo multiHandlers [MULTI_MAX_TYPE + 1] = {
	{MultiDoPosition, 1}, 
	{MultiDoReappear, 1}, 
	{MultiDoFire, 1}, 
	{MultiDoKill, 0}, 
	{MultiDoRemObj, 1}, 
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
	{MultiDoCreatePowerup, 1}, 
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
	{MultiDoKillGoalCounts, 1}, 
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
	{MultiDoShields, 1}, 
	{MultiDoInvul, 1}, 
	{MultiDoDeInvul, 1},
	{MultiDoWeapons, 1},
	{MultiDoMonsterball, 1},
	{MultiDoCheating, 1},
	{MultiDoSyncKills, 1},
	};

//-----------------------------------------------------------------------------

void MultiProcessData (char *buf, int len)
{
	// Take an entire message (that has already been checked for validity, 
	// if necessary) and act on it.

	ubyte nType;
	len = len;

	nType = buf [0];

	if (nType > MULTI_MAX_TYPE)
	{
		Int3 ();
		return;
	}


#ifdef NETPROFILING
	TTRecv [nType]++;
	fprintf (ReceiveLogFile, "Packet nType: %d Len:%d TT = %d\n", nType, len, TTRecv [nType]);
	fflush (RecieveLogFile);
#endif
con_printf (CON_VERBOSE, "multi data %d\n", nType);
#ifdef RELEASE
	if (nType <= MULTI_MAX_TYPE) {
		tMultiHandlerInfo	*pmh = multiHandlers + nType;
		if (pmh->fpMultiHandler && !(gameStates.app.bEndLevelSequence && pmh->noEndLevelSeq))
			pmh->fpMultiHandler (buf);
		}
#else //_DEBUG
	switch (nType)
	{
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
			MultiDoRemObj (buf); 
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
	case MULTI_KILLGOALS:
		if (!gameStates.app.bEndLevelSequence) 
			MultiDoKillGoalCounts (buf); 
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
			MultiDoShields (buf); 
		break;
	case MULTI_CHEATING:
		if (!gameStates.app.bEndLevelSequence) 
			MultiDoCheating (buf); 
		break;
	case MULTI_SYNC_KILLS:
		if (!gameStates.app.bEndLevelSequence) 
			MultiDoSyncKills (buf); 
	default:
		Int3 ();
	}
#endif //_DEBUG
}

//-----------------------------------------------------------------------------
//eof