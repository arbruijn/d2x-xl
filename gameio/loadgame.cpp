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
#include <ctype.h>
#ifndef _WIN32
#	include <unistd.h>
#endif
#include <time.h>

#include "descent.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "console.h"
#include "game.h"
#include "player.h"
#include "key.h"
#include "object.h"
#include "objrender.h"
#include "objsmoke.h"
#include "lightning.h"
#include "physics.h"
#include "error.h"
#include "joy.h"
#include "mono.h"
#include "iff.h"
#include "pcx.h"
#include "timer.h"
#include "cameras.h"
#include "rendermine.h"
#include "fireweapon.h"
#include "omega.h"
#include "screens.h"
#include "textures.h"
#include "slew.h"
#include "cockpit.h"
#include "texmap.h"
#include "3d.h"
#include "effects.h"
#include "menu.h"
#include "segmath.h"
#include "wall.h"
#include "ai.h"
#include "producers.h"
#include "trigger.h"
#include "audio.h"
#include "loadobjects.h"
#include "scores.h"
#include "ibitblt.h"
#include "u_mem.h"
#include "palette.h"
#include "morph.h"
#include "light.h"
#include "dynlight.h"
#include "newdemo.h"
#include "briefings.h"
#include "collide.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "segment.h"
#include "loadgame.h"
#include "gamefont.h"
#include "menu.h"
#include "endlevel.h"
#include "interp.h"
#include "multi.h"
#include "network.h"
#include "netmisc.h"
#include "modem.h"
#include "playerprofile.h"
#include "ctype.h"
#include "fireball.h"
#include "kconfig.h"
#include "config.h"
#include "robot.h"
#include "automap.h"
#include "reactor.h"
#include "powerup.h"
#include "text.h"
#include "cfile.h"
#include "hogfile.h"
#include "piggy.h"
#include "textdata.h"
#include "texmerge.h"
#include "paging.h"
#include "mission.h"
#include "savegame.h"
#include "songs.h"
#include "gamepal.h"
#include "movie.h"
#include "credits.h"
#include "loadgeometry.h"
#include "lightmap.h"
#include "lightcluster.h"
#include "polymodel.h"
#include "movie.h"
#include "particles.h"
#include "interp.h"
#include "sphere.h"
#include "hiresmodels.h"
#include "entropy.h"
#include "monsterball.h"
#include "sparkeffect.h"
#include "transprender.h"
#include "slowmotion.h"
#include "soundthreads.h"
#include "menubackground.h"
#include "monsterball.h"
#include "systemkeys.h"
#include "createmesh.h"
#include "renderthreads.h"
#include "multi.h"
#include "addon_bitmaps.h"
#include "marker.h"
#include "findpath.h"
#include "waypoint.h"
#include "transprender.h"
#include "scores.h"
#include "hudicons.h"

#if defined (FORCE_FEEDBACK)
 #include "tactile.h"
#endif

#include "strutil.h"
#include "rle.h"
#include "input.h"

#define SPAWN_MIN_DIST	I2X (15 * 20)

//------------------------------------------------------------------------------

void InitStuckObjects (void);
void ClearStuckObjects (void);
void InitAIForShip (void);
void ShowLevelIntro (int32_t nLevel);
int32_t InitPlayerPosition (int32_t bRandom, int32_t nSpawnPos = -1);
void ReturningToLevelMessage (void);
void AdvancingToLevelMessage (void);
void DoEndGame (void);
void AdvanceLevel (int32_t bSecret, int32_t bFromSecret);
void FilterObjectsFromLevel (void);

// From allender -- you'll find these defines in state.c and cntrlcen.c
// since I couldn't think of a good place to put them and i wanted to
// fix this stuff fast!  Sorry about that...

#define SECRETB_FILENAME	"secret.sgb"
#define SECRETC_FILENAME	"secret.sgc"

//missionManager.nCurrentLevel starts at 1 for the first level
//-1,-2,-3 are secret levels
//0 means not a real level loaded
// Global variables telling what sort of game we have

//	Extra prototypes declared for the sake of LINT
void CopyDefaultsToRobotsAll (void);

//	HUDClearMessages external, declared in cockpit.h
#ifndef _GAUGES_H
void HUDClearMessages (); // From hud.c
#endif

void SetFunctionMode (int32_t);
void FreeHoardData (void);

extern int32_t nLastLevelPathCreated;
extern int32_t nLastMsgYCrd;

//--------------------------------------------------------------------

void VerifyConsoleObject (void)
{
Assert (N_LOCALPLAYER > -1);
Assert (LOCALPLAYER.nObject > -1);
gameData.objData.pConsole = OBJECT (LOCALPLAYER.nObject);
}

//------------------------------------------------------------------------------

int32_t CountRobotsInLevel (void)
{
	int32_t	robotCount = 0;
	CObject*	pObj;

FORALL_ROBOT_OBJS (pObj)
	robotCount++;
return robotCount;
}

//------------------------------------------------------------------------------

int32_t CountHostagesInLevel (void)
{
	int32_t 	count = 0;
	CObject*	pObj;

FORALL_STATIC_OBJS (pObj)
	if (pObj->info.nType == OBJ_HOSTAGE)
		count++;
return count;
}

//------------------------------------------------------------------------------
//added 10/12/95: delete buddy bot if coop game.  Probably doesn't really belong here. -MT
void GameStartInitNetworkPlayers (void)
{
	int32_t	i, j, t, bCoop = IsCoopGame,
				segNum, segType,
				playerObjs [MAX_PLAYERS], startSegs [MAX_PLAYERS],
				nPlayers, nMaxPlayers = bCoop ? MAX_COOP_PLAYERS + 1 : MAX_PLAYERS;
	CObject*	pObj, * pPrevObj;

	// Initialize network player start locations and CObject numbers

memset (gameStates.multi.bPlayerIsTyping, 0, sizeof (gameStates.multi.bPlayerIsTyping));
//VerifyConsoleObject ();
nPlayers = 0;
j = 0;
for (CObjectIterator iter (pObj); pObj; pObj = (pPrevObj ? iter.Step () : iter.Start ())) {
	pPrevObj = pObj;
	t = pObj->info.nType;
	if ((t == OBJ_PLAYER) || (t == OBJ_GHOST) || (t == OBJ_COOP)) {
		if ((nPlayers >= nMaxPlayers) || (bCoop ? (j && (t != OBJ_COOP)) : (t == OBJ_COOP))) {
			pPrevObj = iter.Back ();
			ReleaseObject (pObj->Index ());
			}
		else {
			playerObjs [nPlayers] = pObj->Index ();
			startSegs [nPlayers] = pObj->info.nSegment;
			nPlayers++;
			}
		j++;
		}
	else if (t == OBJ_ROBOT) {
		if (pObj->IsGuideBot () && IsMultiGame) {
			pPrevObj = iter.Back ();
			ReleaseObject (pObj->Index ());
			}
		}
	}

for (i = 0; i < nMaxPlayers; i++)
	gameData.multiplayer.bAdjustPowerupCap [i] = true;

// the following code takes care of team players being assigned the proper start locations
// in enhanced CTF
for (i = 0; i < nPlayers; i++) {
// find a player object that resides in a segment of proper type for the current player start info
	for (j = 0; j < nPlayers; j++) {
		segNum = startSegs [j];
		if (segNum < 0)
			continue;
		segType = bCoop ? SEGMENT (segNum)->m_function : SEGMENT_FUNC_NONE;
#if 0
		switch (segType) {
			case SEGMENT_FUNC_GOAL_RED:
			case SEGMENT_FUNC_TEAM_RED:
				if (i < nPlayers / 2) // (GetTeam (i) != TEAM_RED)
					continue;
				SEGMENT (segNum)->m_nType = SEGMENT_FUNC_NONE;
				break;
			case SEGMENT_FUNC_GOAL_BLUE:
			case SEGMENT_FUNC_TEAM_BLUE:
				if (i >= nPlayers / 2) //GetTeam (i) != TEAM_BLUE)
					continue;
				SEGMENT (segNum)->m_nType = SEGMENT_FUNC_NONE;
				break;
			default:
				break;
			}
#endif
		pObj = OBJECT (playerObjs [j]);
		pObj->SetType (OBJ_PLAYER);
		gameData.multiplayer.playerInit [i].position = pObj->info.position;
		gameData.multiplayer.playerInit [i].nSegment = pObj->info.nSegment;
		gameData.multiplayer.playerInit [i].nSegType = segType;
		PLAYER (i).SetObject (playerObjs [j]);
		pObj->info.nId = i;
		startSegs [j] = -1;
		break;
		}
	}
gameData.SetViewer (gameData.objData.pConsole = OBJECTS.Buffer ()); // + LOCALPLAYER.nObject;
gameData.multiplayer.nPlayerPositions = nPlayers;

#if DBG
if (gameData.multiplayer.nPlayerPositions != (bCoop ? 4 : 8)) {
#if TRACE
	//console.printf (CON_VERBOSE, "--NOT ENOUGH MULTIPLAYER POSITIONS IN THIS MINE!--\n");
#endif
	//Int3 (); // Not enough positions!!
}
#endif
if (IS_D2_OEM && IsMultiGame && (missionManager.nCurrentMission == missionManager.nBuiltInMission [0]) && (missionManager.nCurrentLevel == 8)) {
	for (i = 0; i < nPlayers; i++)
		if (PLAYER (i).connected && !(NETPLAYER (i).versionMinor & 0xF0)) {
			TextBox ("Warning!", BG_STANDARD, 1, TXT_OK,
					   "This special version of Descent II\nwill disconnect after this level.\nPlease purchase the full version\nto experience all the levels!");
			return;
			}
	}
if (IS_MAC_SHARE && IsMultiGame && (missionManager.nCurrentMission == missionManager.nBuiltInMission [0]) && (missionManager.nCurrentLevel == 4)) {
	for (i = 0; i < nPlayers; i++)
		if (PLAYER (i).connected && !(NETPLAYER (i).versionMinor & 0xF0)) {
			TextBox ("Warning!", BG_STANDARD, 1, TXT_OK,
					   "This shareware version of Descent II\nwill disconnect after this level.\nPlease purchase the full version\nto experience all the levels!");
			return;
			}
	}
}

//------------------------------------------------------------------------------

void GameStartRemoveUnusedPlayers (void)
{
	int32_t i;

if (IsMultiGame) {
	for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++) {
		if (!PLAYER (i).connected || (i >= N_PLAYERS))
			MultiTurnPlayerToGhost (i);
		}
	}
else {		// Note link to above if!!!
	for (i = 1; i < gameData.multiplayer.nPlayerPositions; i++)
		ReleaseObject ((int16_t) PLAYER (i).nObject);
	}
}

//------------------------------------------------------------------------------

#define TEAMKEY(_p)	((GetTeam (_p) == TEAM_RED) ? KEY_RED : KEY_BLUE)

void InitAmmoAndEnergy (void)
{
if (LOCALPLAYER.Energy (false) < gameStates.gameplay.InitialEnergy ())
	LOCALPLAYER.ResetEnergy (gameStates.gameplay.InitialEnergy ());
if (LOCALPLAYER.Shield (false) < gameStates.gameplay.InitialEnergy ())
	LOCALPLAYER.ResetShield (gameStates.gameplay.InitialShield ());
if (LOCALPLAYER.primaryWeaponFlags & (1 << OMEGA_INDEX))
	SetMaxOmegaCharge ();
if (LOCALPLAYER.secondaryAmmo [0] < BUILTIN_MISSILES)
	LOCALPLAYER.secondaryAmmo [0] = BUILTIN_MISSILES;
gameData.multiplayer.weaponStates [N_LOCALPLAYER].nBuiltinMissiles = BUILTIN_MISSILES;
if (LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER) 
	gameData.physicsData.xAfterburnerCharge = I2X (1);
OBJECT (N_LOCALPLAYER)->ResetDamage ();
}

//------------------------------------------------------------------------------

// Setup player for new game
void ResetPlayerData (bool bNewGame, bool bSecret, bool bRestore, int32_t nPlayer)
{
CPlayerData& player = PLAYER ((nPlayer < 0) ? N_LOCALPLAYER : nPlayer);

player.numKillsLevel = 0;
player.numRobotsLevel = CountRobotsInLevel ();
player.hostages.nLevel = CountHostagesInLevel ();
if (!bRestore)
	player.hostages.nTotal += player.hostages.nLevel;
//player.Setup ();
if (bNewGame) {
	player.score = 0;
	player.lastScore = 0;
	player.lives = gameStates.gameplay.nInitialLives;
	player.level = 1;
	player.timeTotal = 0;
	player.hoursLevel = 0;
	player.hoursTotal = 0;
	player.SetEnergy (gameStates.gameplay.InitialEnergy ());
	player.SetShield (gameStates.gameplay.InitialShield ());
	player.nKillerObj = -1;
	player.netKilledTotal = 0;
	player.netKillsTotal = 0;
	player.numKillsLevel = 0;
	player.numKillsTotal = 0;
	player.numRobotsTotal = 0;
	player.nScoreGoalCount = 0;
	player.hostages.nRescued = 0;
	player.numRobotsTotal = player.numRobotsLevel;
	player.hostages.nOnBoard = 0;
	player.SetLaserLevels (0, 0);
	player.flags = 0;
	player.nCloaks =
	player.nInvuls = 0;
	if (nPlayer == N_LOCALPLAYER)
		ResetShipData (N_LOCALPLAYER, bRestore ? 1 : 0);
	if (IsMultiGame && !IsCoopGame) {
		if (IsTeamGame && gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bTeamDoors)
			player.flags |= KEY_GOLD | TEAMKEY (N_LOCALPLAYER);
		else
			player.flags |= (KEY_BLUE | KEY_RED | KEY_GOLD);
		}
	else
		player.timeLevel = 0;
	if ((nPlayer = N_LOCALPLAYER))
		gameStates.app.bFirstSecretVisit = 1;
	}
else {
	if (nPlayer == N_LOCALPLAYER)
		gameData.multiplayer.weaponStates [nPlayer].nShip = gameOpts->gameplay.nShip [0];
	player.lastScore = player.score;
	player.level = missionManager.nCurrentLevel;
	if (!networkData.nJoinState) {
		player.timeLevel = 0;
		player.hoursLevel = 0;
		}
	player.nKillerObj = -1;
	player.hostages.nOnBoard = 0;
	if (!bSecret) {
		if (nPlayer == N_LOCALPLAYER)
			InitAmmoAndEnergy ();
		player.flags &=	~(PLAYER_FLAGS_INVULNERABLE | PLAYER_FLAGS_CLOAKED | KEY_BLUE | KEY_RED | KEY_GOLD);
		if (!(extraGameInfo [IsMultiGame].loadout.nDevice & PLAYER_FLAGS_FULLMAP))
			player.flags &= ~PLAYER_FLAGS_FULLMAP;
		player.cloakTime = 0;
		player.invulnerableTime = 0;
		if (IsMultiGame && !IsCoopGame) {
			if (IsTeamGame && gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bTeamDoors)
				player.flags |= KEY_GOLD | TEAMKEY (N_LOCALPLAYER);
			else
				player.flags |= (KEY_BLUE | KEY_RED | KEY_GOLD);
			}
		}
	else if (gameStates.app.bD1Mission)
		InitAmmoAndEnergy ();
	gameStates.app.bPlayerIsDead = 0; // Added by RH
	player.homingObjectDist = -I2X (1); // Added by RH
	gameData.laserData.xLastFiredTime =
	gameData.laserData.xNextFireTime =
	gameData.missileData.xLastFiredTime =
	gameData.missileData.xNextFireTime = gameData.timeData.xGame; // added by RH, solved demo playback bug
	controls [0].afterburnerState = 0;
	gameStates.gameplay.bLastAfterburnerState = 0;
	audio.DestroyObjectSound (player.nObject);
#ifdef FORCE_FEEDBACK
	if (TactileStick)
		tactile_set_button_jolt ();
#endif
	gameData.objData.pMissileViewer = NULL;
	}
gameData.bossData.ResetHitTimes ();
}

//------------------------------------------------------------------------------

void AddPlayerLoadout (bool bRestore)
{
if (gameStates.app.bHaveExtraGameInfo [IsMultiGame]) {
	LOCALPLAYER.primaryWeaponFlags |= extraGameInfo [IsMultiGame].loadout.nGuns;
	if (gameStates.app.bD1Mission)
	   LOCALPLAYER.primaryWeaponFlags &= ~(HAS_FLAG (HELIX_INDEX) | HAS_FLAG (GAUSS_INDEX) | HAS_FLAG (PHOENIX_INDEX) | HAS_FLAG (OMEGA_INDEX));
	if (!gameStates.app.bD1Mission && IsBuiltinWeapon (SUPER_LASER_INDEX))
		LOCALPLAYER.SetSuperLaser (MAX_SUPERLASER_LEVEL - MAX_LASER_LEVEL);
	else if (IsBuiltinWeapon (LASER_INDEX))
		LOCALPLAYER.SetStandardLaser (MAX_LASER_LEVEL);
	if (gameOpts->gameplay.nShip [0] == 1)
		LOCALPLAYER.primaryWeaponFlags &= ~(1 << FUSION_INDEX);
	if (!bRestore && (IsBuiltinWeapon (VULCAN_INDEX) | IsBuiltinWeapon (GAUSS_INDEX)))
		LOCALPLAYER.primaryAmmo [1] = GAUSS_WEAPON_AMMO_AMOUNT;
	LOCALPLAYER.flags |= extraGameInfo [IsMultiGame].loadout.nDevice;
	if (extraGameInfo [1].bDarkness)
		LOCALPLAYER.flags |= PLAYER_FLAGS_HEADLIGHT;
	if (gameStates.app.bD1Mission) {
	   LOCALPLAYER.primaryWeaponFlags &= ~(HAS_FLAG (HELIX_INDEX) | HAS_FLAG (GAUSS_INDEX) | HAS_FLAG (PHOENIX_INDEX) | HAS_FLAG (OMEGA_INDEX));
	   LOCALPLAYER.flags &= ~(PLAYER_FLAGS_FULLMAP | PLAYER_FLAGS_AMMO_RACK | PLAYER_FLAGS_CONVERTER | PLAYER_FLAGS_AFTERBURNER | PLAYER_FLAGS_HEADLIGHT);
	   }
	}
}

//------------------------------------------------------------------------------

bool StartObserverMode (int32_t nPlayer, int32_t nMode)
{
#if DBG
if ((nMode == 2) && (nPlayer == N_LOCALPLAYER) && gameOpts->gameplay.bObserve) {
#else
if (IsMultiGame && (nMode == 2) && (nPlayer == N_LOCALPLAYER) && gameOpts->gameplay.bObserve) {
#endif
	gameStates.render.bObserving = 1;
	MultiTurnPlayerToGhost (N_LOCALPLAYER);
	LOCALPLAYER.SetObservedPlayer (-1);
	return true;
	}
return false;
}

//------------------------------------------------------------------------------

void StopObserverMode (void)
{
if (OBSERVING) {
	gameStates.render.bObserving = 0;
	MultiTurnGhostToPlayer (N_LOCALPLAYER);
	SetChaseCam (0);
	automap.SetActive (0);
	}
}

//------------------------------------------------------------------------------

// Setup player for a brand-new ship
void ResetShipData (int32_t nPlayer, int32_t nMode)
{
if (nPlayer < 0)
	nPlayer = N_LOCALPLAYER;

CPlayerData& player = PLAYER (nPlayer);

if (nPlayer == N_LOCALPLAYER) {
	if (gameStates.app.bChangingShip) {
		gameOpts->gameplay.nShip [0] = gameOpts->gameplay.nShip [1];
		gameStates.app.bChangingShip = 0;
		}
	gameData.multiplayer.weaponStates [nPlayer].nShip = gameOpts->gameplay.nShip [0];
	}

if (gameData.demoData.nState == ND_STATE_RECORDING) {
	NDRecordLaserLevel (player.LaserLevel (), 0);
	NDRecordPlayerWeapon (0, 0);
	NDRecordPlayerWeapon (1, 0);
	}

player.Setup ();
player.SetObservedPlayer (nPlayer);
player.SetEnergy (gameStates.gameplay.InitialEnergy ());
player.SetShield (gameStates.gameplay.InitialShield ());
player.SetLaserLevels (0, 0);
player.nKillerObj = -1;
player.hostages.nOnBoard = 0;

int32_t	i;
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++) {
	player.primaryAmmo [i] = 0;
	bLastPrimaryWasSuper [i] = 0;
	}
for (i = 1; i < MAX_SECONDARY_WEAPONS; i++) {
	player.secondaryAmmo [i] = 0;
	bLastSecondaryWasSuper [i] = 0;
	}
gameData.multiplayer.weaponStates [nPlayer].nBuiltinMissiles = BUILTIN_MISSILES;
player.secondaryAmmo [0] = BUILTIN_MISSILES;
player.primaryWeaponFlags = HAS_LASER_FLAG;
player.secondaryWeaponFlags = HAS_CONCUSSION_FLAG;
gameData.weaponData.nOverridden = 0;
gameData.weaponData.nPrimary = 0;
gameData.weaponData.nSecondary = 0;
gameData.multiplayer.weaponStates [nPlayer].nAmmoUsed = 0;
player.flags &= ~
	(PLAYER_FLAGS_QUAD_LASERS |
	 PLAYER_FLAGS_AFTERBURNER |
	 PLAYER_FLAGS_CLOAKED |
	 PLAYER_FLAGS_INVULNERABLE |
	 PLAYER_FLAGS_FULLMAP |
	 PLAYER_FLAGS_CONVERTER |
	 PLAYER_FLAGS_AMMO_RACK |
	 PLAYER_FLAGS_HEADLIGHT |
	 PLAYER_FLAGS_HEADLIGHT_ON |
	 PLAYER_FLAGS_FLAG);
player.cloakTime = 0;
player.invulnerableTime = 0;
player.homingObjectDist = -I2X (1); // Added by RH

if (nPlayer == N_LOCALPLAYER) {
	gameData.physicsData.xAfterburnerCharge = (player.flags & PLAYER_FLAGS_AFTERBURNER) ? I2X (1) : 0;
	gameStates.app.bPlayerIsDead = 0;		//player no longer dead
	gameStates.gameplay.bLastAfterburnerState = 0;
	gameData.objData.pMissileViewer = NULL;		///reset missile camera if out there
	controls [0].afterburnerState = 0;
	if (gameOpts->gameplay.nShip [1] < 0)
		gameOpts->gameplay.nShip [1] = (missionConfig.m_playerShip < 0) ? missionConfig.SelectShip (gameOpts->gameplay.nShip [0]) : missionConfig.SelectShip (missionConfig.m_playerShip);
	if (gameOpts->gameplay.nShip [1] > -1) {
		gameOpts->gameplay.nShip [0] = missionConfig.SelectShip (gameOpts->gameplay.nShip [1]);
		gameOpts->gameplay.nShip [1] = -1;
		}
	}
audio.DestroyObjectSound (nPlayer);
// When the ship got blown up, its root submodel had been converted to a debris object.
// Make it a player object again.
CObject* pObj = OBJECT (player.nObject);
if (pObj) {
	OBJECT (nPlayer)->ResetDamage ();
	AddPlayerLoadout (nMode == 1);
	pObj->info.nType = OBJ_PLAYER;
	StartObserverMode (nPlayer, nMode);
	pObj->SetLife (IMMORTAL_TIME);
	pObj->info.nFlags = 0;
	pObj->rType.polyObjInfo.nSubObjFlags = 0;
	pObj->mType.physInfo.flags = PF_TURNROLL | PF_LEVELLING | PF_WIGGLE | PF_USES_THRUST;
	}
InitAIForShip ();
}

//------------------------------------------------------------------------------

//do whatever needs to be done when a player dies in multiplayer
void DoGameOver (void)
{
if (missionManager.nCurrentMission == missionManager.nBuiltInMission [0])
	scoreManager.Add (0);
SetFunctionMode (FMODE_MENU);
gameData.appData.SetGameMode (GM_GAME_OVER);
longjmp (gameExitPoint, 0);		// Exit out of game loop
}

//------------------------------------------------------------------------------

//update various information about the player
void UpdatePlayerStats (void)
{
LOCALPLAYER.timeLevel += gameData.timeData.xFrame;	//the never-ending march of time...
if (LOCALPLAYER.timeLevel > I2X (3600)) {
	LOCALPLAYER.timeLevel -= I2X (3600);
	LOCALPLAYER.hoursLevel++;
	}
LOCALPLAYER.timeTotal += gameData.timeData.xFrame;	//the never-ending march of time...
if (LOCALPLAYER.timeTotal > I2X (3600)) {
	LOCALPLAYER.timeTotal -= I2X (3600);
	LOCALPLAYER.hoursTotal++;
	}
}

//	------------------------------------------------------------------------------

void SetVertigoRobotFlags (void)
{
	CObject*	pObj;

gameData.objData.nVertigoBotFlags = 0;
FORALL_ROBOT_OBJS (pObj)
	if ((pObj->info.nId >= 66) && !pObj->IsBoss ())
		gameData.objData.nVertigoBotFlags |= (1 << (pObj->info.nId - 64));
}


//------------------------------------------------------------------------------

char *LevelName (int32_t nLevel)
{
return (gameStates.app.bAutoRunMission && *szAutoMission) ? szAutoMission : (nLevel < 0) ?
		 missionManager.szSecretLevelNames [-nLevel-1] :
		 missionManager.szLevelNames [nLevel-1];
}

//------------------------------------------------------------------------------

char *MakeLevelFilename (int32_t nLevel, char *pszFilename, const char *pszFileExt)
{
CFile::ChangeFilenameExtension (pszFilename, strlwr (LevelName (nLevel)), pszFileExt);
return pszFilename;
}

//------------------------------------------------------------------------------

char *LevelSongName (int32_t nLevel)
{
	char *szNoSong = "";

return (gameStates.app.bAutoRunMission || !missionManager.nSongs) ? szNoSong : missionManager.szSongNames [missionManager.songIndex [nLevel % missionManager.nSongs]];
}

//------------------------------------------------------------------------------

void UnloadLevelData (int32_t bRestore, bool bQuit)
{
ENTER (0, 0);
PrintLog (1);

paletteManager.EnableEffect (true);
if (bQuit)
	DestroyRenderThreads ();
ResetModFolders ();
textureManager.Destroy ();
gameStates.render.bOmegaModded = 0;
gameOpts->render.textures.bUseHires [0] = gameOpts->render.textures.bUseHires [1];
gameOpts->render.bHiresModels [0] = gameOpts->render.bHiresModels [1];
if (gameOpts->UseHiresSound () != gameOpts->sound.bHires [1]) {
	gameOpts->sound.bHires [0] = gameOpts->sound.bHires [1];
	audio.Reset ();
	}
/*---*/PrintLog (0, "unloading mine rendering data\n");
gameData.renderData.mine.Destroy ();

/*---*/PrintLog (0, "stopping sounds\n");
audio.DestroyObjectSound (LOCALPLAYER.nObject);
audio.StopAllChannels ();

/*---*/PrintLog (0, "reconfiguring audio\n");
if (!bRestore) {
	gameStates.gameplay.slowmo [0].fSpeed =
	gameStates.gameplay.slowmo [1].fSpeed = 1.0f;
	gameStates.gameplay.slowmo [0].nState =
	gameStates.gameplay.slowmo [1].nState = 0;
	if (audio.SlowDown () != 1.0f) {
		audio.Shutdown ();
		audio.Setup (1);
		}
	}
/*---*/PrintLog (0, "unloading lightmaps\n");
lightmapManager.Destroy ();
/*---*/PrintLog (0, "unloading hoard data\n");
/*---*/FreeHoardData ();
/*---*/PrintLog (0, "unloading textures\n");
UnloadTextures ();
/*---*/PrintLog (0, "unloading custom sounds\n");
FreeSoundReplacements ();
/*---*/PrintLog (0, "unloading addon sounds\n");
FreeAddonSounds ();
/*---*/PrintLog (0, "unloading hardware lights\n");
lightManager.Reset ();
/*---*/PrintLog (0, "unloading hires models\n");
FreeHiresModels (1);
/*---*/PrintLog (0, "unloading cambot\n");
UnloadCamBot ();
/*---*/PrintLog (0, "unloading additional models\n");
FreeModelExtensions ();
/*---*/PrintLog (0, "unloading additional model textures\n");
FreeObjExtensionBitmaps ();
/*---*/PrintLog (0, "unloading additional model textures\n");
UnloadHiresAnimations ();
/*---*/PrintLog (0, "freeing spark effect buffers\n");
sparkManager.Destroy ();
/*---*/PrintLog (0, "freeing auxiliary poly model data\n");
gameData.modelData.Destroy ();
/*---*/PrintLog (0, "Destroying camera objects\n");
cameraManager.Destroy ();
/*---*/PrintLog (0, "Destroying omega lightnings\n");
omegaLightning.Destroy (-1);
/*---*/PrintLog (0, "Destroying monsterball\n");
RemoveMonsterball ();
/*---*/PrintLog (0, "Unloading way points\n");
wayPointManager.Destroy ();
/*---*/PrintLog (0, "Unloading mod text messages\n");
FreeModTexts ();
/*---*/PrintLog (0, "Unloading addon texures\n");
UnloadAddonImages ();
/*---*/PrintLog (0, "Unloading particle texures\n");
particleImageManager.FreeAll ();
/*---*/PrintLog (0, "Unloading HUD icons\n");
hudIcons.Destroy ();
PrintLog (-1);
RETURN
}

//------------------------------------------------------------------------------

int32_t LoadModData (char* pszLevelName, int32_t bLoadTextures, int32_t nStage)
{
ENTER (0, 0);

	int32_t	nLoadRes = 0;
	char		szFile [FILENAME_LEN];

// try to read mod files, and load default files if that fails
if (nStage == 0) {
	SetD1Sound ();
	SetDataVersion (-1);
#if 0
	LoadD2Sounds (true);
#else
	if (gameStates.app.bD1Mission)
		LoadD1Sounds (false);
	else
		LoadD2Sounds (false);
	songManager.LoadDescentSongs (gameFolders.mods.szCurrent, gameStates.app.bD1Mission);
	LoadAddonSounds ();
	gameStates.app.bCustomSounds = false;
	if (gameStates.app.bHaveMod && (gameStates.app.bD1Mission ? LoadD1Sounds (true) : LoadD2Sounds (true))) {
		gameStates.app.bCustomSounds = true;
		songManager.LoadModPlaylist ();
		if (gameOpts->UseHiresSound () != gameOpts->sound.bHires [1]) {
			WaitForSoundThread ();
			audio.Reset ();
			songManager.PlayLevelSong (missionManager.nCurrentLevel, 1);
			}
		}
#endif
	if (!gameStates.app.bD1Mission/*missionManager.nEnhancedMission*/) {
		sprintf (szFile, "d2x.ham");
		/*---*/PrintLog (1, "trying vertigo custom robots (d2x.ham)\n");
		nLoadRes = LoadRobotExtensions ("d2x.ham", gameFolders.missions.szRoot, /*missionManager.nEnhancedMission*/2);
		PrintLog (-1);
		}
	if (ReadHamFile (1) || ReadHamFile (2))
		gameStates.app.bCustomData = true;
	else if (gameStates.app.bCustomData) {
		ReadHamFile ();
		gameStates.app.bCustomData = false;
		}
	if (gameStates.app.bHaveMod) {
		/*---*/PrintLog (1, "trying custom robots (hxm) from mod '%s'\n", gameFolders.mods.szName);
		if (LoadRobotReplacements (gameFolders.mods.szName, gameFolders.mods.szCurrent, 0, 0, true, false))
			gameStates.app.bCustomData = true;
		PrintLog (-1);
		}
	if (missionManager.nEnhancedMission) {
		/*---*/PrintLog (1, "reading additional robots\n");
		if (missionManager.nEnhancedMission < 3)
			nLoadRes = 0;
		else {
			sprintf (szFile, "%s.vham", gameFolders.mods.szName);
			/*---*/PrintLog (1, "trying custom robots (vham) from mod '%s'\n", gameFolders.mods.szName);
			nLoadRes = LoadRobotExtensions (szFile, gameFolders.mods.szCurrent, missionManager.nEnhancedMission);
			PrintLog (-1);
			}
		if (nLoadRes == 0) {
			sprintf (szFile, "%s.ham", gameStates.app.szCurrentMissionFile);
			/*---*/PrintLog (1, "trying custom robots (ham) from level '%s'\n", gameStates.app.szCurrentMissionFile);
			nLoadRes = LoadRobotExtensions (szFile, gameFolders.missions.szRoot, missionManager.nEnhancedMission);
			PrintLog (-1);
			}
		if (nLoadRes > 0) {
			strncpy (szFile, gameStates.app.szCurrentMissionFile, 6);
			strcat (szFile, "-l.mvl");
			/*---*/PrintLog (1, "initializing additional robot movies\n");
			movieManager.InitExtraRobotLib (szFile);
			PrintLog (-1);
			}
		PrintLog (-1);
		}
	if (gameStates.app.bD1Mission) {
		/*---*/PrintLog (1, "loading Descent 1 textures\n");
		LoadD1Textures ();
		PrintLog (-1);
		}
	}
else {
	ResetPogEffects ();
	if (!gameStates.app.bD1Mission) {
		if (bLoadTextures)
			LoadLevelTextures ();
		LoadTextData (pszLevelName, ".msg", gameData.messages);
		LoadTextData (pszLevelName, ".snd", &gameData.soundData);
		LoadReplacementBitmaps (pszLevelName);
		LoadSoundReplacements (pszLevelName);
		particleImageManager.LoadAll ();
		}

	/*---*/PrintLog (1, "loading replacement robots\n");
	if (0 > LoadRobotReplacements (pszLevelName, NULL, 0, 0, true)) {
		PrintLog (-1);
		RETVAL (-1);
		}
	/*---*/PrintLog (1, "loading cambot\n");
	gameData.botData.nCamBotId = (LoadRobotReplacements ("cambot.hxm", NULL, 1, 0) > 0) ? gameData.botData.nTypes [0] - 1 : -1;
	PrintLog (-1);
	gameData.botData.nCamBotModel = gameData.modelData.nPolyModels - 1;

	/*---*/PrintLog (1, "loading replacement models\n");
	if (*gameFolders.mods.szModels [0]) {
		sprintf (gameFolders.mods.szModels [1], "%s%s", gameFolders.mods.szModels [0], LevelFolder (missionManager.nCurrentLevel));
		sprintf (gameFolders.var.szModels [2], "%s%s", gameFolders.var.szModels [1], gameFolders.mods.szLevel);
		LoadHiresModels (2);
		}
	LoadHiresModels (1);
	PrintLog (-1);

	/*---*/PrintLog (1, "initializing cambot\n");
	InitCamBots (0);
	PrintLog (-1);
	/*---*/PrintLog (1, "loading mod texts\n");
	LoadModTexts ();
	PrintLog (-1);
	}
RETVAL (nLoadRes);
}

//------------------------------------------------------------------------------

static void CleanupBeforeGame (int32_t nLevel, int32_t bRestore)
{
ENTER (0, 0);
DestroyRenderThreads ();
transparencyRenderer.ResetBuffers ();
gameData.Destroy ();
gameStates.app.SRand ();
gameData.timeData.tLast = 0;
gameStates.render.nLightingMethod = gameStates.app.bNostalgia ? 0 : gameOpts->render.nLightingMethod;
gameStates.app.bBetweenLevels = 1;
gameStates.render.bFreeCam = (gameStates.render.bEnableFreeCam ? 0 : -1);
gameStates.app.bGameRunning = 0;
gameStates.app.bPlayerIsDead = 0;
LOCALPLAYER.m_bExploded = 0;
gameData.physicsData.side.nSegment = -1;
gameData.physicsData.side.nSide = -1;
markerManager.Init ();
gameStates.gameplay.bKillBossCheat = 0;
gameStates.render.nFlashScale = 0;
gameOpts->app.nScreenShotInterval = 0;	//better reset this every time a level is loaded
automap.m_bFull = 0;
ogl.m_data.nHeadlights = -1;
gameData.renderData.nColoredFaces = 0;
gameData.appData.nFrameCount = 0;
gameData.appData.nMineRenderCount = 0;
gameData.objData.lists.Init ();
memset (gameData.appData.semaphores, 0, sizeof (gameData.appData.semaphores));
transparencyRenderer.Init ();
#if PROFILING
PROF_INIT
#endif
memset (gameData.statsData.player, 0, sizeof (tPlayerStats));
gameData.renderData.mine.bObjectRendered.Clear (char (0xff));
gameData.renderData.mine.bRenderSegment.Clear (char (0xff));
gameData.renderData.mine.bCalcVertexColor.Clear (0);
memset (gameData.multiplayer.weaponStates, 0, sizeof (gameData.multiplayer.weaponStates));
memset (gameData.multiplayer.bWasHit, 0, sizeof (gameData.multiplayer.bWasHit));
memset (gameData.multiplayer.nLastHitTime, 0, sizeof (gameData.multiplayer.nLastHitTime));
memset (gameData.multiplayer.tAppearing, 0, sizeof (gameData.multiplayer.tAppearing));
memset (gameData.weaponData.firing, 0, sizeof (gameData.weaponData.firing));
gameData.objData.objects.Clear ();
lightClusterManager.Init ();
gameData.renderData.faceIndex.Init ();
lightManager.ResetIndex ();
for (int32_t i = 0; i < MAX_PLAYERS; i++)
	gameData.objData.SetGuidedMissile (i, NULL);
omegaLightning.Init ();
SetRearView (0);
SetFreeCam (0);
CGenericCockpit::Rewind (false);
cockpit->Init ();
gameData.multiplayer.bMoving = -1;
missionManager.nCurrentLevel = nLevel;
UnloadLevelData (bRestore, false);
ControlRenderThreads ();
/*---*/PrintLog (1, "restoring default robot settings\n");
RestoreDefaultModels ();
SetChaseCam (0);
SetFreeCam (0);
StopObserverMode ();
PrintLog (-1);
paletteManager.EnableEffect (true);
RETURN
}

//------------------------------------------------------------------------------
//load a level off disk. level numbers start at 1.  Secret levels are -1,-2,-3

extern char szAutoMission [255];

int32_t LoadLevel (int32_t nLevel, bool bLoadTextures, bool bRestore)
{
ENTER (0, 0);

	char			*pszLevelName;
	char			szHogName [FILENAME_LEN];
	CPlayerInfo	savePlayer;
	int32_t		nRooms, nCurrentLevel = missionManager.nCurrentLevel;

strlwr (pszLevelName = LevelName (nLevel));
CleanupBeforeGame (nLevel, bRestore);
gameStates.render.SetCartoonStyle (gameOpts->render.bCartoonize);
gameStates.app.bD1Mission = gameStates.app.bAutoRunMission 
									 ? (strstr (szAutoMission, "rdl") != NULL) 
									 : (missionManager [missionManager.nCurrentMission].nDescentVersion == 1);
MakeModFolders (hogFileManager.MissionName (), nLevel);
if (!(gameStates.app.bHaveMod || missionManager.IsBuiltIn (hogFileManager.MissionName ())))
	 MakeModFolders (gameStates.app.bD1Mission ? "Descent: First Strike" : "Descent 2: Counterstrike!", nLevel);
lightManager.SetMethod ();

if (!gameStates.app.bPrecomputeLightmaps) {
#if 1
	if (LoadModData (NULL, 0, 0) < 0) {
		gameStates.app.bBetweenLevels = 0;
		missionManager.nCurrentLevel = nCurrentLevel;
		PrintLog (-1);
		RETVAL (-1);
		}
#endif
	songManager.PlayLevelSong (missionManager.nCurrentLevel, 1);
	/*---*/PrintLog (1, "Initializing particle manager\n");
	InitObjectSmoke ();
	PrintLog (-1);
	gameData.pigData.tex.bitmapColors.Clear ();
	gameData.modelData.thrusters.Clear ();
	savePlayer = LOCALPLAYER;
	if (!gameStates.app.bAutoRunMission &&
		 (!nLevel || (nLevel > missionManager.nLastLevel) || (nLevel < missionManager.nLastSecretLevel))) {
		gameStates.app.bBetweenLevels = 0;
		missionManager.nCurrentLevel = nCurrentLevel;
		Warning ("Invalid level number!");
		RETVAL (0)
		}
	nLastMsgYCrd = -1;		//so we don't restore backgound under msg
	if (!gameStates.app.bProgressBars)
		messageBox.Show (TXT_LOADING);
	}

/*---*/PrintLog (1, "loading texture brightness and color info\n");
SetDataVersion (-1);
memcpy (gameData.pigData.tex.brightness.Buffer (),
			gameData.pigData.tex.defaultBrightness [gameStates.app.bD1Mission].Buffer (),
			gameData.pigData.tex.brightness. Size ());
LoadTextureBrightness (pszLevelName, NULL);
gameData.renderData.color.textures = gameData.renderData.color.defaultTextures [gameStates.app.bD1Mission];
LoadTextureColors (pszLevelName, NULL);
PrintLog (-1);

if (!gameStates.app.bPrecomputeLightmaps) {
	/*---*/PrintLog (1, "loading mission configuration info\n");
	missionConfig.Init ();
	missionConfig.Load ();
	missionConfig.Load (pszLevelName);
	missionConfig.Apply ();
	PrintLog (-1);
	}

InitTexColors ();

int32_t nLoadRes;
for (int32_t nAttempts = 0; nAttempts < 2; nAttempts++) {
	if (!(nLoadRes = LoadLevelData (pszLevelName, nLevel)))
		break;	//actually load the data from disk!
	nLoadRes = 1;
	if (nAttempts)
		break;
	if (strstr (hogFileManager.AltFiles ().szName, ".hog"))
		break;
	sprintf (szHogName, "%s%s%s", gameFolders.missions.szRoot, gameFolders.missions.szSubFolder, pszLevelName);
	if (!hogFileManager.UseMission (szHogName))
		break;
	}
if (nLoadRes) {
	/*---*/PrintLog (0, "Couldn't load '%s' (%d)\n", pszLevelName, nLoadRes);
	gameStates.app.bBetweenLevels = 0;
	missionManager.nCurrentLevel = nCurrentLevel;
	Warning (TXT_LOAD_ERROR, pszLevelName);
	RETVAL (0)
	}

if (!gameStates.app.bPrecomputeLightmaps) {
	if (!gameStates.app.bProgressBars)
		messageBox.Show (TXT_LOADING);
	paletteManager.SetGame (paletteManager.Load (szCurrentLevelPalette, pszLevelName, 1, 1, 1));		//don't change screen
#if 1
	if (LoadModData (pszLevelName, bLoadTextures, 1) < 0) {
		gameStates.app.bBetweenLevels = 0;
		missionManager.nCurrentLevel = nCurrentLevel;
		RETVAL (-1);
		}
#endif
	}
#if 1
if (!lightManager.Setup (nLevel)) {
	PrintLog (-1, "Not enough memory for light data\n");
	RETVAL (-1);
	}
#endif
if (!gameStates.app.bPrecomputeLightmaps) {
	/*---*/PrintLog (1, "loading endlevel data\n");
	LoadEndLevelData (nLevel);
	PrintLog (-1);

	ResetNetworkObjects ();
	ResetChildObjects ();
	ResetPlayerPaths ();
	FixObjectSizes ();
	if (!bRestore)
		wayPointManager.Setup ();
	/*---*/PrintLog (1, "counting entropy rooms\n");
	nRooms = CountRooms ();
	if (IsEntropyGame) {
		if (!nRooms) {
			Warning (TXT_NO_ENTROPY);
			gameData.appData.nGameMode &= ~GM_ENTROPY;
			gameData.appData.nGameMode |= GM_TEAM;
			}
		}
	else if ((gameData.appData.GameMode (GM_CAPTURE | GM_HOARD)) ||
				((gameData.appData.GameMode (GM_MONSTERBALL)) == GM_MONSTERBALL)) {
	/*---*/PrintLog (1, "gathering CTF+ flag goals\n");
		if (GatherFlagGoals () != 3) {
			Warning (TXT_NO_CTF);
			gameData.appData.nGameMode &= ~GM_CAPTURE;
			gameData.appData.nGameMode |= GM_TEAM;
			}
		}
	PrintLog (-1);

	gameData.renderData.lights.segDeltas.Clear ();
	/*---*/PrintLog (1, "initializing door animations\n");
	InitDoorAnims ();
	PrintLog (-1);

	(CPlayerInfo&) LOCALPLAYER = savePlayer;
	gameData.hoardData.nMonsterballSeg = -1;
	if (!IsMultiGame)
		InitEntropySettings (0);	//required for repair centers
	//songManager.PlayLevelSong (missionManager.nCurrentLevel, 1);
	if (!gameStates.app.bProgressBars)
		messageBox.Clear ();		//remove message before new palette loaded
	//paletteManager.ResumeEffect ();		//actually load the palette
	if (!bRestore) {
		/*---*/PrintLog (1, "rebuilding OpenGL context\n");
		ogl.SetRenderQuality ();
		ogl.RebuildContext (1);
		PrintLog (-1);
		}
	ResetPingStats ();
	gameStates.gameplay.nDirSteps = 0;
	gameStates.gameplay.bMineMineCheat = 0;
	gameStates.render.bAllVisited = 0;
	gameStates.render.bViewDist = 1;
	gameStates.render.bHaveSkyBox = -1;
	gameStates.app.cheats.nUnlockLevel = 0;
	gameStates.render.nFrameFlipFlop = 0;
	gameStates.app.bUsingConverter = 0;
	/*---*/PrintLog (1, "resetting color information\n");
	gameData.renderData.color.vertices.Clear ();
	gameData.renderData.color.segments.Clear ();
	PrintLog (-1);
	/*---*/PrintLog (1, "resetting speed boost information\n");
	gameData.objData.speedBoost.Clear ();
	PrintLog (-1);
	if (!ogl.m_features.bStencilBuffer)
		extraGameInfo [0].bShadows = 0;
	D2SetCaption ();
	if (!bRestore) {
		gameData.renderData.lights.bInitDynColoring = 1;
		gameData.omegaData.xCharge [IsMultiGame] = MAX_OMEGA_CHARGE;
		SetMaxOmegaCharge ();
		ConvertObjects ();
		SetEquipmentMakerStates ();
		SetupWalls ();
		SetupEffects ();
	//	lightManager.Setup (nLevel);
		gameData.timeData.nPaused = 0;
		}
	LoadAddonImages ();
	CreateShieldSphere ();
	PrintLog (1, "initializing energy spark render data\n");
	sparkManager.Setup ();
	PrintLog (-1);
	PrintLog (1, "setting robot generator vertigo robot flags\n");
	SetVertigoRobotFlags ();
	PrintLog (-1);
	PrintLog (1, "initializing debris collision handlers\n");
	SetDebrisCollisions ();
	PrintLog (-1);
	PrintLog (1, "building sky box segment list\n");
	BuildSkyBoxSegList ();
	PrintLog (-1);
	/*---*/PrintLog (1, "stopping music\n");
	//songManager.StopAll ();
	audio.SetFxVolume ((gameConfig.nAudioVolume [1] * 32768) / 8, 1);
	audio.SetVolumes ((gameConfig.nAudioVolume [0] * 32768) / 8, (gameConfig.nMidiVolume * 128) / 8);
	PrintLog (-1);
	CreateSoundThread ();
	}

gameStates.render.bDepthSort = 1;
gameStates.app.bBetweenLevels = 0;
PrintLog (-1);
return 1;
}

//------------------------------------------------------------------------------

extern bool bRegisterBitmaps;

//starts a new game on the given level
int32_t StartNewGame (int32_t nLevel)
{
gameData.appData.SetGameMode (GM_NORMAL);
SetFunctionMode (FMODE_GAME);
missionManager.SetNextLevel (0);
missionManager.SetNextLevel (-1, 1);
N_PLAYERS = 1;
gameData.objData.nLastObject [0] = 0;
networkData.bNewGame = 0;
missionManager.DeleteLevelStates ();
missionManager.SaveLevelStates ();
InitMultiPlayerObject (0);
bRegisterBitmaps = true;
if (!StartNewLevel (nLevel, true)) {
	gameStates.app.bAutoRunMission = 0;
	bRegisterBitmaps = false;
	return 0;
	}
LOCALPLAYER.startingLevel = nLevel;		// Mark where they started
GameDisableCheats ();
InitSeismicDisturbances ();
OBJECT (0)->info.controlType = CT_AI;
OBJECT (0)->info.nType = OBJ_ROBOT;
OBJECT (0)->info.nId = 33;
ROBOTINFO (33)->xMaxSpeed[0] = I2X(80);
InitAIObject (0, AIB_NORMAL, OBJECT (0)->Segment());
gameStates.app.cheats.bMadBuddy = 1;
gameOpts->gameplay.bUseD1AI = 0;
return 1;
}

//------------------------------------------------------------------------------

#ifndef _NETWORK_H
extern int32_t NetworkEndLevelPoll2 (CMenu& menu, int32_t& key, int32_t nCurItem); // network.c
#endif

//	Does the bonus scoring.
//	Call with deadFlag = 1 if player died, but deserves some portion of bonus (only skill points), anyway.
void DoEndLevelScoreGlitz (int32_t network)
{
	#define N_GLITZITEMS 11

	int32_t		nLevelPoints, nSkillPoints, nEnergyPoints, nShieldPoints, nHostagePoints, nAllHostagePoints, nEndGamePoints;
	char			szAllHostages [64];
	char			szEndGame [64];
	char			szMenu [N_GLITZITEMS + 1][40];
	CMenu			m (N_GLITZITEMS + 1);
	int32_t		i, c;
	char			szTitle [128];
	int32_t		bIsLastLevel = 0;
	int32_t		nMineLevel = 0;

audio.DestroyObjectSound (LOCALPLAYER.nObject);
audio.StopAllChannels ();
SetScreenMode (SCREEN_MENU);		//go into menu mode
if (gameStates.app.bHaveExtraData)

//	Compute level player is on, deal with secret levels (negative numbers)
nMineLevel = LOCALPLAYER.level;
if (nMineLevel < 0)
	nMineLevel *= -(missionManager.nLastLevel / missionManager.nSecretLevels);
else if (nMineLevel == 0)
	nMineLevel = 1;
nEndGamePoints =
nSkillPoints =
nShieldPoints =
nEnergyPoints =
nHostagePoints =
nAllHostagePoints = 0;
bIsLastLevel = 0;
nLevelPoints = LOCALPLAYER.score - LOCALPLAYER.lastScore;
szAllHostages [0] = 0;
szEndGame [0] = 0;
if (!gameStates.app.cheats.bEnabled) {
	if (gameStates.app.nDifficultyLevel > 1) {
		nSkillPoints = nLevelPoints * (gameStates.app.nDifficultyLevel) / 4;
		nSkillPoints -= nSkillPoints % 100;
		}
	else
		nSkillPoints = 0;
	if (0 > (nShieldPoints = X2I (LOCALPLAYER.Shield ()) * 5 * nMineLevel))
		nShieldPoints = 0;
	else
		nShieldPoints -= nShieldPoints % 50;
	if (0 > (nEnergyPoints = X2I (LOCALPLAYER.Energy ()) * 2 * nMineLevel))
		nEnergyPoints = 0;
	else
		nEnergyPoints -= nEnergyPoints % 50;
	nHostagePoints = LOCALPLAYER.hostages.nOnBoard * 500 * (gameStates.app.nDifficultyLevel+1);
	if ((LOCALPLAYER.hostages.nOnBoard > 0) && (LOCALPLAYER.hostages.nOnBoard == LOCALPLAYER.hostages.nLevel)) {
		nAllHostagePoints = LOCALPLAYER.hostages.nOnBoard * 1000 * (gameStates.app.nDifficultyLevel+1);
		sprintf (szAllHostages, "%s%i", TXT_FULL_RESCUE_BONUS, nAllHostagePoints);
		}
	if (!IsMultiGame && LOCALPLAYER.lives && (missionManager.nCurrentLevel == missionManager.nLastLevel)) {		//player has finished the game!
		nEndGamePoints = LOCALPLAYER.lives * 10000;
		sprintf (szEndGame, "%s%i  ", TXT_SHIP_BONUS, nEndGamePoints);
		bIsLastLevel = 1;
		}
	cockpit->AddBonusPointsToScore (nSkillPoints + nEnergyPoints + nShieldPoints + nHostagePoints + nAllHostagePoints + nEndGamePoints);
	}
c = 0;
sprintf (szMenu [c++], "%s%i  ", TXT_SHIELD_BONUS, nShieldPoints);		// Return at start to lower menu...
sprintf (szMenu [c++], "%s%i  ", TXT_ENERGY_BONUS, nEnergyPoints);
sprintf (szMenu [c++], "%s%i  ", TXT_HOSTAGE_BONUS, nHostagePoints);
sprintf (szMenu [c++], "%s%i  ", TXT_SKILL_BONUS, nSkillPoints);
if (*szAllHostages)
	sprintf (szMenu [c++], "%s  ", szAllHostages);
if (bIsLastLevel)
	sprintf (szMenu [c++], "%s  ", szEndGame);
sprintf (szMenu [c++], "%s%i  ", TXT_TOTAL_BONUS,
			nShieldPoints + nEnergyPoints + nHostagePoints + nSkillPoints + nAllHostagePoints + nEndGamePoints);
sprintf (szMenu [c++], "%s%i  ", TXT_TOTAL_SCORE, LOCALPLAYER.score);
for (i = 0; i < c; i++)
	m.AddText ("", szMenu [i]);
sprintf (szTitle,
			"%s%s %d %s\n%s %s",
			gameOpts->menus.nStyle ? "" : bIsLastLevel ? "\n\n\n":"\n",
			 (missionManager.nCurrentLevel < 0) ? TXT_SECRET_LEVEL : TXT_LEVEL,
			 (missionManager.nCurrentLevel < 0) ? -missionManager.nCurrentLevel : missionManager.nCurrentLevel,
			TXT_COMPLETE,
			missionManager.szCurrentLevel,
			TXT_DESTROYED);
Assert (c <= N_GLITZITEMS);
paletteManager.DisableEffect ();

if (gameOpts->gameplay.bSkipEndLevelScreen)
	return;

if (network && IsNetworkGame)
	m.Menu (NULL, szTitle, NetworkEndLevelPoll2, NULL, BG_SUBMENU, BG_STARS);
else
// NOTE LINK TO ABOVE!!!
gameStates.app.bGameRunning = 0;
backgroundManager.SetShadow (false);
m.Menu (NULL, szTitle, NULL, NULL, BG_SUBMENU, BG_STARS);
backgroundManager.SetShadow (true);
}

//	-----------------------------------------------------------------------------------------------------

//give the player the opportunity to save his game
void DoEndlevelMenu ()
{
//No between level saves......!!!	StateSaveAll (1);
}

//------------------------------------------------------------------------------

//	Returns true if secret level has been destroyed.
int32_t PSecretLevelDestroyed (void)
{
if (gameStates.app.bFirstSecretVisit)
	return 0;		//	Never been there, can't have been destroyed.
if (CFile::Exist (SECRETC_FILENAME, gameFolders.user.szSavegames, 0))
	return 0;
return 1;
}

//	-----------------------------------------------------------------------------------------------------

void DoSecretMessage (const char *msg)
{
	int32_t fMode = gameStates.app.nFunctionMode;

StopTime ();
SetFunctionMode (FMODE_MENU);
TextBox (NULL, BG_STARS, 1, TXT_OK, msg);
SetFunctionMode (fMode);
StartTime (0);
}

//	-----------------------------------------------------------------------------------------------------

void InitSecretLevel (int32_t nLevel)
{
Assert (missionManager.nCurrentLevel == nLevel);	//make sure level set right
Assert (gameStates.app.nFunctionMode == FMODE_GAME);
meshBuilder.ComputeFaceKeys ();
GameStartInitNetworkPlayers (); // Initialize the gameData.multiplayer.players array for this level
HUDClearMessages ();
automap.ClearVisited ();
ResetPlayerData (false, true, false);
gameData.SetViewer (OBJECT (LOCALPLAYER.nObject));
GameStartRemoveUnusedPlayers ();
if (gameStates.app.bGameSuspended & SUSP_TEMPORARY)
	gameStates.app.bGameSuspended &= ~(SUSP_ROBOTS | SUSP_TEMPORARY);
gameData.reactorData.bDestroyed = 0;
cockpit->Init ();
paletteManager.ResetEffect ();
audio.Prepare ();
audio.SetupRouter ();
}

//------------------------------------------------------------------------------

//	Called from trigger.cpp when player is on a secret level and hits exit to return to base level.
void ExitSecretLevel (void)
{
bool bResume = networkThread.Suspend ();
if ((gameData.demoData.nState == ND_STATE_RECORDING) || (gameData.demoData.nState == ND_STATE_PAUSED))
	NDStopRecording ();
if (!(gameStates.app.bD1Mission || gameData.reactorData.bDestroyed))
	saveGameManager.Save (0, 2, 0, SECRETC_FILENAME);
if (!gameStates.app.bD1Mission && CFile::Exist (SECRETB_FILENAME, gameFolders.user.szSavegames, 0)) {
	int32_t pwSave = gameData.weaponData.nPrimary;
	int32_t swSave = gameData.weaponData.nSecondary;

	ReturningToLevelMessage ();
	saveGameManager.Load (1, 1, 0, SECRETB_FILENAME);
	SetD1Sound ();
	SetDataVersion (-1);
	SetPosFromReturnSegment (1);
	//LOCALPLAYER.lives--;	//	re-lose the life, LOCALPLAYER.lives got written over in restore.
	gameData.weaponData.nPrimary = pwSave;
	gameData.weaponData.nSecondary = swSave;
	}
else { // File doesn't exist, so can't return to base level.  Advance to next one.
	if (missionManager.nEntryLevel == missionManager.nLastLevel)
		DoEndGame ();
	else {
		if (!gameStates.app.bD1Mission)
			AdvancingToLevelMessage ();
		if (gameStates.app.bPlayerIsDead)
			DoPlayerDead ();
		else {
			DoEndLevelScoreGlitz (0);
			if (!StartNewLevel (missionManager.nEntryLevel + 1, false))
				longjmp (gameExitPoint, 0);
			}
		}
	}
if (bResume)
	networkThread.Resume ();
}

//------------------------------------------------------------------------------

//	Called from trigger.cpp when player is exiting via a directed exit
int32_t ReenterLevel (void)
{
	char nState = missionManager.GetLevelState (missionManager.NextLevel ());

if (nState < 0) 
	return 0;

	char	szFile [FILENAME_LEN] = {'\0'};

if ((gameData.demoData.nState == ND_STATE_RECORDING) || (gameData.demoData.nState == ND_STATE_PAUSED))
	NDStopRecording ();
missionManager.LevelStateName (szFile);
if (!(gameStates.app.bD1Mission || gameData.reactorData.bDestroyed))
	saveGameManager.Save (0, -1, 0, szFile);
else if (CFile::Exist (szFile, gameFolders.var.szCache, 0))
	CFile::Delete (szFile, gameFolders.user.szSavegames);
szFile [0] = '\x02';
missionManager.LevelStateName (szFile + 1, missionManager.NextLevel ());
if (!gameStates.app.bD1Mission && (nState > 0) && CFile::Exist (szFile, gameFolders.var.szCache, 0)) {
	int32_t pwSave = gameData.weaponData.nPrimary;
	int32_t swSave = gameData.weaponData.nSecondary;
	saveGameManager.Load (1, -1, 0, szFile + 1);
	SetD1Sound ();
	SetDataVersion (-1);
	InitPlayerPosition (1);
	int32_t nMaxPlayers = IsCoopGame ? MAX_COOP_PLAYERS + 1 : MAX_PLAYERS;
	for (int32_t i = 0; i < nMaxPlayers; i++)
		gameData.multiplayer.bAdjustPowerupCap [i] = true;
	//LOCALPLAYER.lives--;	//	re-lose the life, LOCALPLAYER.lives got written over in restore.
	gameData.weaponData.nPrimary = pwSave;
	gameData.weaponData.nSecondary = swSave;
	missionManager.nCurrentLevel = missionManager.NextLevel ();
	return 1;
	}
return 0;
}

//------------------------------------------------------------------------------
//	Set invulnerableTime and cloakTime in player struct to preserve amount of time left to
//	be invulnerable or cloaked.
void DoCloakInvulSecretStuff (fix xOldGameTime)
{
if (LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE) {
		fix	time_used;

	time_used = xOldGameTime - LOCALPLAYER.invulnerableTime;
	LOCALPLAYER.invulnerableTime = gameData.timeData.xGame - time_used;
	}

if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
		fix	time_used;

	time_used = xOldGameTime - LOCALPLAYER.cloakTime;
	LOCALPLAYER.cloakTime = gameData.timeData.xGame - time_used;
	}
}

//------------------------------------------------------------------------------
//called when the player has finished a level
void PlayerFinishedLevel (int32_t bSecret)
{
	Assert (!bSecret);

LOCALPLAYER.hostages.nRescued += LOCALPLAYER.hostages.nOnBoard;
if (IsNetworkGame)
	CONNECT (N_LOCALPLAYER, CONNECT_WAITING); // Finished but did not die
gameStates.render.cockpit.nLastDrawn [0] =
gameStates.render.cockpit.nLastDrawn [1] = -1;
SetScreenMode (SCREEN_MENU);
if (missionManager.nCurrentLevel < 0)
	ExitSecretLevel ();
else
	AdvanceLevel (0,0);
}

//------------------------------------------------------------------------------

void PlayLevelMovie (const char *pszExt, int32_t nLevel)
{
	char szFilename [FILENAME_LEN];

movieManager.Play (MakeLevelFilename (nLevel, szFilename, pszExt), MOVIE_OPTIONAL, 0, gameOpts->movies.bResize);
}

//------------------------------------------------------------------------------

void PlayLevelIntroMovie (int32_t nLevel)
{
PlayLevelMovie (".mvi", nLevel);
}

//------------------------------------------------------------------------------

void PlayLevelExtroMovie (int32_t nLevel)
{
PlayLevelMovie (".mvx", nLevel);
}

//------------------------------------------------------------------------------

#define MOVIE_REQUIRED 1

//called when the player has finished the last level
void DoEndGame (void)
{
SetFunctionMode (FMODE_MENU);
if ((gameData.demoData.nState == ND_STATE_RECORDING) || (gameData.demoData.nState == ND_STATE_PAUSED))
	NDStopRecording ();
SetScreenMode (SCREEN_MENU);
gameData.renderData.frame.Activate ("DoEndGame (frame, 1)");
KeyFlush ();
if (!IsMultiGame) {
	if (missionManager.nCurrentMission == (gameStates.app.bD1Mission ? missionManager.nBuiltInMission [1] : missionManager.nBuiltInMission [0])) {
		int32_t bPlayed = MOVIE_NOT_PLAYED;	//default is not bPlayed

		if (!gameStates.app.bD1Mission) {
			subTitles.Init (ENDMOVIE ".tex");
			bPlayed = movieManager.Play (ENDMOVIE, MOVIE_REQUIRED, 0, gameOpts->movies.bResize);
			subTitles.Close ();
			}
		else if (movieManager.m_bHaveExtras) {
			//InitSubTitles (ENDMOVIE ".tex");	//ingore errors
			bPlayed = movieManager.Play (D1_ENDMOVIE, MOVIE_REQUIRED, 0, gameOpts->movies.bResize);
			subTitles.Close ();
			}
		if (!bPlayed) {
			if (IS_D2_OEM) {
				songManager.Play (SONG_TITLE, 0);
				briefing.Run ("end2oem.tex", 1);
				}
			else {
				songManager.Play (SONG_ENDGAME, 0);
				briefing.Run (gameStates.app.bD1Mission ? "endreg.tex" : "ending2.tex", gameStates.app.bD1Mission ? 0x7e : 1);
				}
			}
		}
	else {    //not multi
		char szBriefing [FILENAME_LEN];
		PlayLevelExtroMovie (missionManager.nCurrentLevel);
		sprintf (szBriefing, "%s.tex", gameStates.app.szCurrentMissionFile);
		briefing.Run (szBriefing, missionManager.nLastLevel + 1);   //level past last is endgame briefing

		//try doing special credits
		sprintf (szBriefing,"%s.ctb",gameStates.app.szCurrentMissionFile);
		creditsRenderer.Show (szBriefing);
		}
	}
KeyFlush ();
if (IsMultiGame)
	MultiEndLevelScore ();
else
	// NOTE LINK TO ABOVE
	DoEndLevelScoreGlitz (0);

if ((missionManager.nCurrentMission == missionManager.nBuiltInMission [0]) && !(IsMultiGame || IsCoopGame)) {
	gameData.renderData.frame.Activate ("DoEndGame (frame, 2)");
	CCanvas::Current ()->Clear (BLACK_RGBA);
	paletteManager.ResetEffect ();
	//paletteManager.Load (D2_DEFAULT_PALETTE, NULL, 0, 1, 0);
	scoreManager.Add (0);
	}
SetFunctionMode (FMODE_MENU);
if (gameData.appData.GameMode (GM_SERIAL | GM_MODEM))
	gameData.appData.SetGameMode (gameData.appData.GameMode () | GM_GAME_OVER);		//preserve modem setting so go back into modem menu
else
	gameData.appData.SetGameMode (GM_GAME_OVER);
longjmp (gameExitPoint, 0);		// Exit out of game loop
}

//------------------------------------------------------------------------------
//from which level each do you get to each secret level
//called to go to the next level (if there is one)
//if bSecret is true, advance to secret level, else next Normal one
//	Return true if game over.
void AdvanceLevel (int32_t bSecret, int32_t bFromSecret)
{
	int32_t result;

gameStates.app.bBetweenLevels = 1;
Assert (!bSecret);
if (!bFromSecret && ((missionManager.nCurrentLevel != missionManager.nLastLevel) ||
	  extraGameInfo [IsMultiGame].bRotateLevels)) {
	if (IsMultiGame)
		MultiEndLevelScore ();
	else {
		PlayLevelExtroMovie (missionManager.nCurrentLevel);
		DoEndLevelScoreGlitz (0);		//give bonuses
		}
	}
if ((gameData.demoData.nState == ND_STATE_RECORDING) || (gameData.demoData.nState == ND_STATE_PAUSED))
	NDStopRecording ();
if (IsMultiGame) {
	if ((result = MultiEndLevel (&bSecret))) { // Wait for other players to reach this point
		gameStates.app.bBetweenLevels = 0;
		if (missionManager.nCurrentLevel != missionManager.nLastLevel)		//player has finished the game!
			return;
		longjmp (gameExitPoint, 0);		// Exit out of game loop
		}
	}

if (gameData.reactorData.bDestroyed)
	missionManager.SetLevelState (missionManager.nCurrentLevel, -1);
missionManager.SaveLevelStates ();

if ((missionManager.NextLevel () > 0) && 
	 (missionManager.NextLevel () > missionManager.LastLevel ()) && 
	 (missionManager.NextLevel (1) < 1) &&
	 !extraGameInfo [IsMultiGame].bRotateLevels) //player has finished the game!
	DoEndGame ();
else {
	if (missionManager.NextLevel () < 1) 
		missionManager.SetNextLevel (missionManager.nCurrentLevel + 1);		//assume go to next Normal level
	if (missionManager.NextLevel () > missionManager.nLastLevel) {
		if (extraGameInfo [IsMultiGame].bRotateLevels)
			missionManager.SetNextLevel (1);
		else
			missionManager.SetNextLevel (missionManager.LastLevel ());
		}
	if (!IsMultiGame)
		DoEndlevelMenu (); // Let user save their game
	if (!ReenterLevel ()) {
		if (!StartNewLevel (missionManager.NextLevel (), false))
			longjmp (gameExitPoint, 0);
		}
	}
gameStates.app.bBetweenLevels = 0;
}

//------------------------------------------------------------------------------

void DiedInMineMessage (void)
{
if (IsMultiGame)
	return;
gameStates.app.bGameRunning = 0;
SetScreenMode (SCREEN_MENU);		//go into menu mode
backgroundManager.SetShadow (false);
TextBox (NULL, BG_STARS, 1, TXT_OK, TXT_DIED_IN_MINE);
backgroundManager.SetShadow (true);
gameStates.app.bGameRunning = 1;
}

//------------------------------------------------------------------------------
//	Called when player dies on secret level.
void ReturningToLevelMessage (void)
{
	char	msg [128];

if (IsMultiGame)
	return;
StopTime ();
SetScreenMode (SCREEN_MENU);		//go into menu mode
gameData.renderData.frame.Activate ("ReturningToLevelMessage (frame)");
int32_t nFunctionMode = gameStates.app.nFunctionMode;
SetFunctionMode (FMODE_MENU);
if (missionManager.nEntryLevel < 0)
	sprintf (msg, TXT_SECRET_LEVEL_RETURN);
else
	sprintf (msg, TXT_RETURN_LVL, missionManager.nEntryLevel);
TextBox (NULL, BG_STARS, 1, TXT_OK, msg);
SetFunctionMode (nFunctionMode);
StartTime (0);
}

//------------------------------------------------------------------------------
//	Called when player dies on secret level.
void AdvancingToLevelMessage (void)
{
	char	msg [128];

	//	Only supposed to come here from a secret level.
Assert (missionManager.nCurrentLevel < 0);
if (IsMultiGame)
	return;
StopTime ();
SetScreenMode (SCREEN_MENU);		//go into menu mode
gameData.renderData.frame.Activate ("AdvancingToLevelMessage (frame)");
int32_t nFunctionMode = gameStates.app.nFunctionMode;
SetFunctionMode (FMODE_MENU);
sprintf (msg, "Base level destroyed.\nAdvancing to level %i", missionManager.nEntryLevel + 1);
backgroundManager.SetShadow (false);
TextBox (NULL, BG_STARS, 1, TXT_OK, msg);
SetFunctionMode (nFunctionMode);
StartTime (0);
}

//	-----------------------------------------------------------------------------------
//	Set the player's position from the globals gameData.segData.secret.nReturnSegment and gameData.segData.secret.returnOrient.
void SetPosFromReturnSegment (int32_t bRelink)
{
	CObject*	pObj = OBJECT (LOCALPLAYER.nObject);

pObj->info.position.vPos = SEGMENT (gameData.segData.secret.nReturnSegment)->Center ();
if (bRelink)
	pObj->RelinkToSeg (gameData.segData.secret.nReturnSegment);
ResetPlayerObject ();
pObj->info.position.mOrient = gameData.segData.secret.returnOrient;
}

//------------------------------------------------------------------------------
//called when the player is starting a level (new game or new ship)
void StartLevel (int32_t nLevel, int32_t bRandom)
{
if (nLevel == 0x7fffffff)
	PrintLog (1, "restarting level\n");
else
	PrintLog (1, "starting level %d\n", nLevel);
Assert (!gameStates.app.bPlayerIsDead);
VerifyConsoleObject ();
PrintLog (0, "initializing player position\n");
gameStates.app.nSpawnPos = InitPlayerPosition (bRandom, gameStates.app.nSpawnPos);
if (OBSERVING)
	automap.SetActive (-1);
else {
	gameStates.app.nSpawnPos = -1;
	MultiTurnGhostToPlayer (N_LOCALPLAYER);
	}
gameData.objData.pConsole->info.controlType = CT_FLYING;
gameData.objData.pConsole->info.movementType = MT_PHYSICS;
DisableObjectProducers ();
PrintLog (0, "clearing transient objects\n");
ClearTransientObjects (0);		//0 means leave proximity bombs
// gameData.objData.pConsole->CreateAppearanceEffect ();
gameStates.render.bDoAppearanceEffect = 1;
if (IsMultiGame) {
	if (IsCoopGame)
		MultiSendScore ();
	if (LOCALOBJECT->Type () == OBJ_PLAYER)
		MultiSendReappear ();
	NetworkFlushData (); // will send position, shield and weapon info
	}
PrintLog (0, "keeping network busy\n");
if (IsNetworkGame)
	NetworkDoFrame (1);
PrintLog (0, "resetting AI paths\n");
AIResetAllPaths ();
gameData.bossData.ResetHitTimes ();
PrintLog (0, "clearing stuck objects\n");
ClearStuckObjects ();
if (nLevel != 0x7fffffff)
	meshBuilder.ComputeFaceKeys ();
ResetTime ();
ResetRearView ();
gameData.fusionData.xAutoFireTime = 0;
gameData.SetFusionCharge (0);
gameStates.app.cheats.bRobotsFiring = 1;
gameStates.app.cheats.bD1CheatsEnabled = 0;
SetD1Sound ();
SetDataVersion (-1);
PrintLog (-1);
}

//------------------------------------------------------------------------------

//called when the player is starting a new level for Normal game mode and restore state
//	bSecret set if came from a secret level

int32_t PrepareLevel (int32_t nLevel, bool bLoadTextures, bool bSecret, bool bRestore, bool bNewGame)
{
	int32_t funcRes;

gameStates.multi.bTryAutoDL = 1;

reloadLevel:

if (!IsMultiGame) {
	gameStates.render.cockpit.nLastDrawn [0] =
	gameStates.render.cockpit.nLastDrawn [1] = -1;
	}
gameStates.render.cockpit.bBigWindowSwitch = 0;
if (gameData.demoData.nState == ND_STATE_PAUSED)
	gameData.demoData.nState = ND_STATE_RECORDING;
if (gameData.demoData.nState == ND_STATE_RECORDING) {
	NDSetNewLevel (nLevel);
	NDRecordStartFrame (gameData.appData.nFrameCount, gameData.timeData.xFrame);
	}
if (IsMultiGame)
	SetFunctionMode (FMODE_MENU); // Cheap fix to prevent problems with errror dialogs in loadlevel.
SetWarnFunc (ShowInGameWarning);
try {
	funcRes = LoadLevel (nLevel, bLoadTextures, bRestore);
	}
catch (int32_t e) {
	if (e == EX_OUT_OF_MEMORY)
		Warning ("Couldn't load the level:\nNot enough memory.");
	else if (e == EX_IO_ERROR)
		Warning ("Couldn't load the level:\nReading some data failed.");
	else
		Warning ("Couldn't load the level.");
	funcRes = 0;
	}
catch (...) {
	Warning ("Couldn't load the level.");
	funcRes = 0;
	}
ClearWarnFunc (ShowInGameWarning);
if (funcRes <= 0) {
	try {
		CleanupAfterGame (false);
		}
	catch (...) {
		Warning ("Internal error when cleaning up.");
		}
	return 0;
	}
Assert (gameStates.app.bAutoRunMission || (missionManager.nCurrentLevel == nLevel));	//make sure level set right

missionManager.SetLevelState (missionManager.nCurrentLevel, 1);
missionManager.SaveLevelStates ();
GameStartInitNetworkPlayers (); // Initialize the gameData.multiplayer.players array for
#if 0 //DBG										  // this level
InitHoardData ();
SetMonsterballForces ();
#endif
//	gameData.objData.pViewer = OBJECT (LOCALPLAYER.nObject);
if (N_PLAYERS > gameData.multiplayer.nPlayerPositions) {
	TextBox (NULL, BG_STANDARD, 1, TXT_OK, "Too many players for this level.");
#if 1
	while (N_PLAYERS > gameData.multiplayer.nPlayerPositions) {
		--N_PLAYERS;
		memset (gameData.multiplayer.players + N_PLAYERS, 0, sizeof (PLAYER (N_PLAYERS)));
		}
#else
	return 0;
#endif
	}
Assert (N_PLAYERS <= gameData.multiplayer.nPlayerPositions);
	//If this assert fails, there's not enough start positions

if (IsNetworkGame) {
	switch (NetworkLevelSync ()) { // After calling this, N_LOCALPLAYER is set
		case -1:
			return 0;
		case 0:
			if (N_LOCALPLAYER < 0)
				return 0;
			fontManager.Remap ();
			break;
		case 1:
			networkData.nStatus = 2;
			goto reloadLevel;
		}
	}
Assert (gameStates.app.nFunctionMode == FMODE_GAME);
HUDClearMessages ();
automap.ClearVisited ();

for (int32_t i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	ResetPlayerData (bNewGame, bSecret, bRestore, i);
if (IsCoopGame && networkData.nJoinState) {
	for (int32_t i = 0; i < N_PLAYERS; i++)
		PLAYER (i).flags |= netGameInfo.PlayerFlags () [i];
	}
audio.Prepare ();
if (IsMultiGame)
	MultiPrepLevel (); // Removes robots from level if necessary
else
	ResetMonsterball (false); //will simply remove all Monsterballs
GameStartRemoveUnusedPlayers ();
if (gameStates.app.bGameSuspended & SUSP_TEMPORARY)
	gameStates.app.bGameSuspended &= ~(SUSP_ROBOTS | SUSP_TEMPORARY);
gameData.reactorData.bDestroyed = 0;
gameStates.render.glFOV = DEFAULT_FOV;
SetScreenMode (SCREEN_GAME);
cockpit->Init ();
audio.SetupRouter ();
InitRobotsForLevel ();
InitShakerDetonates ();
MorphInit ();
InitAllObjectProducers ();
InitThiefForLevel ();
InitStuckObjects ();
GameFlushInputs ();		// clear out the keyboard
if (!IsMultiGame)
	FilterObjectsFromLevel ();
TurnCheatsOff ();
if (!(IsMultiGame || (gameStates.app.cheats.bEnabled & 2))) {
	SetHighestLevel (missionManager.nCurrentLevel);
	}
else
	LoadPlayerProfile (1);		//get window sizes
ResetSpecialEffects ();
if (networkData.nJoinState == 1) {
	networkData.nJoinState = 0;
	StartLevel (nLevel, 1);
	}
else {
	StartLevel (nLevel, 0);		// Note link to above if!
	}
CopyDefaultsToRobotsAll ();
if (!bRestore) {
	InitReactorForLevel (0);
	InitAIObjects ();
	gameData.modelData.Prepare ();
	}
if (!bRestore) {
	/*---*/PrintLog (1, "initializing sound sources\n");
	SetSoundSources ();
	PrintLog (-1);
	}
#if 0
LOCALPLAYER.nInvuls =
LOCALPLAYER.nCloaks = 0;
#endif
//	Say player can use FLASH cheat to mark path to exit.
nLastLevelPathCreated = -1;
return 1;
}

//	-----------------------------------------------------------------------------------------------------
//called when the player is starting a level (new game or new ship)

static void StartSecretLevel (void)
{
Assert (!gameStates.app.bPlayerIsDead);
InitPlayerPosition (0);
VerifyConsoleObject ();
gameData.objData.pConsole->info.controlType = CT_FLYING;
gameData.objData.pConsole->info.movementType = MT_PHYSICS;
// -- WHY? -- DisableObjectProducers ();
ClearTransientObjects (0);		//0 means leave proximity bombs
// gameData.objData.pConsole->CreateAppearanceEffect ();
gameStates.render.bDoAppearanceEffect = 1;
AIResetAllPaths ();
ResetTime ();
ResetRearView ();
gameData.fusionData.xAutoFireTime = 0;
gameData.SetFusionCharge (0);
gameStates.app.cheats.bRobotsFiring = 1;
SetD1Sound ();
if (gameStates.app.bD1Mission) {
	LOCALPLAYER.ResetEnergy (INITIAL_ENERGY);
	LOCALPLAYER.ResetShield (INITIAL_SHIELD);
	}
}

//------------------------------------------------------------------------------

static int32_t PrepareSecretLevel (int32_t nLevel, bool bLoadTextures)
{
	CMenu	menu (1);

bool bResume = networkThread.Suspend ();
gameStates.app.xThisLevelTime = 0;
menu.AddText ("", " ");
gameStates.render.cockpit.nLastDrawn [0] = -1;
gameStates.render.cockpit.nLastDrawn [1] = -1;

if (gameData.demoData.nState == ND_STATE_PAUSED)
	gameData.demoData.nState = ND_STATE_RECORDING;

if (gameData.demoData.nState == ND_STATE_RECORDING) {
	NDSetNewLevel (nLevel);
	NDRecordStartFrame (gameData.appData.nFrameCount, gameData.timeData.xFrame);
	}
else if (gameData.demoData.nState != ND_STATE_PLAYBACK) {
	paletteManager.DisableEffect ();
	SetScreenMode (SCREEN_MENU);		//go into menu mode
	if (gameStates.app.bFirstSecretVisit)
		DoSecretMessage (gameStates.app.bD1Mission ? TXT_ALTERNATE_EXIT : TXT_SECRET_EXIT);
	else if (CFile::Exist (SECRETC_FILENAME,gameFolders.user.szSavegames,0))
		DoSecretMessage (gameStates.app.bD1Mission ? TXT_ALTERNATE_EXIT : TXT_SECRET_EXIT);
	else {
		char	text_str [128];

		sprintf (text_str, TXT_ADVANCE_LVL, missionManager.nCurrentLevel+1);
		DoSecretMessage (text_str);
		}
	}

if (gameStates.app.bFirstSecretVisit || (gameData.demoData.nState == ND_STATE_PLAYBACK)) {
	if (!LoadLevel (nLevel, bLoadTextures, false))
		return 0;
	InitSecretLevel (nLevel);
	if (!gameStates.app.bAutoRunMission && gameStates.app.bD1Mission)
		ShowLevelIntro (nLevel);
	//songManager.PlayLevelSong (missionManager.nCurrentLevel, 0);
	InitRobotsForLevel ();
	InitReactorForLevel (0);
	InitAIObjects ();
	InitShakerDetonates ();
	MorphInit ();
	InitAllObjectProducers ();
	ResetSpecialEffects ();
	StartSecretLevel ();
	if (gameStates.app.bD1Mission)
		LOCALPLAYER.flags &= ~PLAYER_FLAGS_ALL_KEYS;
	}
else {
	if (CFile::Exist (SECRETC_FILENAME, gameFolders.user.szSavegames, 0)) {
		int32_t	pw_save, sw_save, nCurrentLevel;

		pw_save = gameData.weaponData.nPrimary;
		sw_save = gameData.weaponData.nSecondary;
		nCurrentLevel = missionManager.nCurrentLevel;
		saveGameManager.Load (1, 1, 0, SECRETC_FILENAME);
		missionManager.nEntryLevel = nCurrentLevel;
		gameData.weaponData.nPrimary = pw_save;
		gameData.weaponData.nSecondary = sw_save;
		ResetSpecialEffects ();
		StartSecretLevel ();
		}
	else {
		char	text_str [128];

		sprintf (text_str, TXT_ADVANCE_LVL, missionManager.nCurrentLevel+1);
		DoSecretMessage (text_str);
		if (!LoadLevel (nLevel, bLoadTextures, false))
			return 0;
		InitSecretLevel (nLevel);
		return 1;
		}
	InitReactorForLevel (0);
	}

if (gameStates.app.bFirstSecretVisit)
	CopyDefaultsToRobotsAll ();
TurnCheatsOff ();
//	Say player can use FLASH cheat to mark path to exit.
nLastLevelPathCreated = -1;
gameStates.app.bFirstSecretVisit = 0;
if (bResume)
	networkThread.Resume ();
return 1;
}

//------------------------------------------------------------------------------
//	Called from switch.c when player passes through secret exit.  That means he was on a non-secret level and he
//	is passing to the secret level.
//	Do a savegame.
void EnterSecretLevel (void)
{
	int32_t	i;

Assert (!IsMultiGame);
missionManager.nEntryLevel = missionManager.nCurrentLevel;
if (gameData.reactorData.bDestroyed)
	DoEndLevelScoreGlitz (0);
if (gameData.demoData.nState != ND_STATE_PLAYBACK)
	saveGameManager.Save (0, 1, 0, NULL);	//	Not between levels (ie, save all), IS a secret level, NO filename override
//	Find secret level number to go to, stuff in missionManager.NextLevel ().
for (i = 0; i < -missionManager.nLastSecretLevel; i++)
	if (missionManager.secretLevelTable [i] == missionManager.nCurrentLevel) {
		missionManager.SetNextLevel (-i - 1);
		break;
		}
	else if (i && (missionManager.secretLevelTable [i] > missionManager.nCurrentLevel)) {	//	Allows multiple exits in same group.
		missionManager.SetNextLevel (-i);
		break;
		}
if (i >= -missionManager.nLastSecretLevel)		//didn't find level, so must be last
	missionManager.SetNextLevel (missionManager.LastSecretLevel ());
PrepareSecretLevel (missionManager.NextLevel (), true);
// do_cloak_invul_stuff ();
}

//------------------------------------------------------------------------------

void CObject::Bash (uint8_t nId)
{
if (Type () == OBJ_POWERUP)
	RemovePowerupInMine (info.nId);
AddPowerupInMine (nId, 1, true);
if (Type () != OBJ_POWERUP)
	SetType (OBJ_POWERUP);
info.renderType = RT_POWERUP;
info.controlType = CT_POWERUP;
info.nId = nId;
SetSizeFromPowerup ();
rType.animationInfo.nClipIndex = gameData.objData.pwrUp.info [nId].nClipIndex;
rType.animationInfo.xFrameTime = gameData.effectData.animations [0][rType.animationInfo.nClipIndex].xFrameTime;
}

//------------------------------------------------------------------------------

void CObject::BashToShield (bool bBash)
{
if (bBash)
	Bash (POW_SHIELD_BOOST);
}

//------------------------------------------------------------------------------

void CObject::BashToEnergy (bool bBash)
{
if (bBash)
	Bash (POW_ENERGY);
}

//------------------------------------------------------------------------------

void FilterObjectsFromLevel (void)
{
	CObject*	pObj;

FORALL_POWERUP_OBJS (pObj)
	pObj->BashToShield ((pObj->info.nId == POW_REDFLAG) || (pObj->info.nId == POW_BLUEFLAG));
}

//------------------------------------------------------------------------------

struct tIntroMovieInfo {
	int32_t	nLevel;
	char	szMovieName [FILENAME_LEN];
} szIntroMovies [] = {{ 1,"pla"},
							 { 5,"plb"},
							 { 9,"plc"},
							 {13,"pld"},
							 {17,"ple"},
							 {21,"plf"},
							 {24,"plg"}};

#define NUM_INTRO_MOVIES (sizeof (szIntroMovies) / sizeof (*szIntroMovies))

void ShowLevelIntro (int32_t nLevel)
{
//if shareware, show a briefing?
if (!IsMultiGame) {
	uint32_t i;

	PlayLevelIntroMovie (nLevel);
	if (!gameStates.app.bD1Mission && (missionManager.nCurrentMission == missionManager.nBuiltInMission [0])) {
		if (IS_SHAREWARE) {
			if (nLevel == 1)
				briefing.Run ("brief2.tex", 1);
			}
		else if (IS_D2_OEM) {
			if ((nLevel == 1) && !gameStates.movies.bIntroPlayed) {
				gameStates.movies.bIntroPlayed = 1;
				briefing.Run ("brief2o.tex", 1);
				}
			}
		else { // full version
			if (movieManager.m_bHaveExtras && (nLevel == 1)) {
				movieManager.PlayIntro ();
				}
			for (i = 0; i < NUM_INTRO_MOVIES; i++) {
				if (szIntroMovies [i].nLevel == nLevel) {
					gameStates.video.nScreenMode = -1;
					movieManager.Play (szIntroMovies [i].szMovieName, MOVIE_REQUIRED, 0, gameOpts->movies.bResize);
					break;
					}
				}
			if (movieManager.m_nRobots) {
				int32_t hires_save = gameStates.menus.bHiresAvailable;
				if (movieManager.m_nRobots == 1) {		//lowres only
					gameStates.menus.bHiresAvailable = 0;		//pretend we can't do highres
					if (hires_save != gameStates.menus.bHiresAvailable)
						gameStates.video.nScreenMode = -1;		//force reset
					}
				briefing.Run ("robot.tex", nLevel);
				gameStates.menus.bHiresAvailable = hires_save;
				}
			}
		}
	else {	//not the built-in mission.  check for add-on briefing
		if ((missionManager [missionManager.nCurrentMission].nDescentVersion == 1) &&
			 (missionManager.nCurrentMission == missionManager.nBuiltInMission [1])) {
			if (movieManager.m_bHaveExtras && (nLevel == 1)) {
				if (movieManager.Play ("briefa.mve", MOVIE_REQUIRED, 0, gameOpts->movies.bResize) != MOVIE_ABORTED)
					 movieManager.Play ("briefb.mve", MOVIE_REQUIRED, 0, gameOpts->movies.bResize);
				}
			else
				briefing.Run (missionManager.szBriefingFilename [0], nLevel);
			}
		else {
			char szBriefing [FILENAME_LEN];
			sprintf (szBriefing, "%s.tex", gameStates.app.szCurrentMissionFile);
			briefing.Run (szBriefing, nLevel);
			}
		}
	}
}

//	---------------------------------------------------------------------------
// If a player enters a level listed in the mission's secret level table,
// all secret levels he can reach from now on are ones he hasn't been able
// to reach before (in other words: No secret exits yet to come lead to
// secret levels he's been to before), so set the proper flag.

void MaybeSetFirstSecretVisit (int32_t nLevel)
{
for (int32_t i = 0; i < missionManager.nSecretLevels; i++) {
	if (missionManager.secretLevelTable [i] == nLevel) {
		gameStates.app.bFirstSecretVisit = 1;
		}
	}
}

//------------------------------------------------------------------------------
//called when the player is starting a new level for Normal game model
//	bSecret if came from a secret level
int32_t StartNewLevel (int32_t nLevel, bool bNewGame)
{
bool bResume = networkThread.Suspend ();
gameStates.app.xThisLevelTime = 0;
gameData.reactorData.bDestroyed = 0;
ogl.SetDrawBuffer (GL_BACK, 0);
MakeModFolders (hogFileManager.MissionName (), nLevel);
if (nLevel < 0) {
	if (!PrepareSecretLevel (nLevel, false))
		return 0;
	}
else {
	MaybeSetFirstSecretVisit (nLevel);
	if (!gameStates.app.bAutoRunMission)
		ShowLevelIntro (nLevel);
	if (!PrepareLevel (nLevel, 1, false, false, bNewGame))
		return 0;
	}
if (!bNewGame)
	ogl.ChooseDrawBuffer ();
if (bResume)
	networkThread.Resume ();

OBJECT (0)->info.controlType = CT_AI;
OBJECT (0)->info.nType = OBJ_ROBOT;
OBJECT (0)->info.nId = 33;
ROBOTINFO (33)->xMaxSpeed[0] = I2X(100);
InitAIObject (0, AIB_NORMAL, OBJECT (0)->Segment());
gameStates.app.cheats.bMadBuddy = 1;

return 1;
}

//------------------------------------------------------------------------------
// Sort descending

typedef struct tSpawnTable {
	int32_t	i;
	fix	xDist;
	} tSpawnTable;

void SortSpawnTable (tSpawnTable *spawnTable, int32_t left, int32_t right)
{
	int32_t	l = left,
			r = right;
	fix	m = spawnTable [(l + r) / 2].xDist;

do {
	while (spawnTable [l].xDist > m)
		l++;
	while (spawnTable [r].xDist < m)
		r--;
	if (l <= r) {
		if (l < r) {
			tSpawnTable h = spawnTable [l];
			spawnTable [l] = spawnTable [r];
			spawnTable [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	SortSpawnTable (spawnTable, l, right);
if (left < r)
	SortSpawnTable (spawnTable, left, r);
}

//------------------------------------------------------------------------------

int32_t GetSegmentTeam (int32_t nSegType)
{
switch (nSegType) {
	case SEGMENT_FUNC_GOAL_RED:
	case SEGMENT_FUNC_TEAM_RED:
		return 2;
	case SEGMENT_FUNC_GOAL_BLUE:
	case SEGMENT_FUNC_TEAM_BLUE:
		return 1;
	default:
		return 3;
	}
}

//------------------------------------------------------------------------------

int32_t GetRandomPlayerPosition (int32_t nPlayer)
{
	CObject*		pObj;
	tSpawnTable	spawnTable [MAX_PLAYERS];
	int32_t		nSpawnSegs = 0;
	int32_t		nTeam = IsTeamGame ? GetTeam (nPlayer) + 1 : 3;
	int32_t		i, j;
	fix			xDist;

// Put the indices of all spawn point in the spawn table that are sufficiently far away from any player in the match
//gameStates.app.SRand ();

for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++) {
	if (!(nTeam & GetSegmentTeam (gameData.multiplayer.playerInit [i].nSegType)))
		continue; // exclude team specific spawn points of the wrong team
	spawnTable [nSpawnSegs].i = i;
	spawnTable [nSpawnSegs].xDist = 0x7fffffff;
	for (j = 0; j < N_PLAYERS; j++) {
		if (j != N_LOCALPLAYER) {
			pObj = OBJECT (PLAYER (j).nObject);
			if ((pObj->info.nType == OBJ_PLAYER)) {
				xDist = simpleRouter [0].PathLength (pObj->info.position.vPos, pObj->info.nSegment,
																 gameData.multiplayer.playerInit [i].position.vPos, gameData.multiplayer.playerInit [i].nSegment,
																 10, WID_PASSABLE_FLAG, -1);	//	Used to be 5, search up to 10 segments
				if (xDist < 0)
					continue;
				if (spawnTable [nSpawnSegs].xDist > xDist)
					spawnTable [nSpawnSegs].xDist = xDist;
				}
			}
		}
	++nSpawnSegs;
	}

// sort by descending distance from closest player in mine
SortSpawnTable (spawnTable, 0, nSpawnSegs - 1);

for (j = 0; j < nSpawnSegs; j++)
	if (spawnTable [j].xDist < SPAWN_MIN_DIST)
		break;
return spawnTable [(j > 1) ? Rand (j) : 0].i;
}

//------------------------------------------------------------------------------
//initialize the player CObject position & orientation (at start of game, or new ship)
int32_t InitPlayerPosition (int32_t bRandom, int32_t nSpawnPos)
{
if (nSpawnPos < 0) {
	nSpawnPos = 0;
	if (!IsMultiGame || IsCoopGame) // If not deathmatch
		nSpawnPos = N_LOCALPLAYER;
	else if (bRandom == 1)
		nSpawnPos = GetRandomPlayerPosition (N_LOCALPLAYER);
	else 
		goto done; // If deathmatch and not Random, positions were already determined by sync packet
	}
MovePlayerToSpawnPos (nSpawnPos, gameData.objData.pConsole);

done:

ResetPlayerObject ();
controls.ResetCruise ();
return nSpawnPos;
}

//------------------------------------------------------------------------------

fix RobotDefaultShield (CObject *pObj)
{
	tRobotInfo*	pRobotInfo;
	int32_t		objId, i;
	fix			shield;

if (pObj->info.nType != OBJ_ROBOT)
	return (pObj->info.nType == OBJ_REACTOR) ? ReactorStrength () : 0;
objId = pObj->info.nId;
//Assert (objId < gameData.botData.nTypes [0]);
i = gameStates.app.bD1Mission && (objId < gameData.botData.nTypes [1]);
pRobotInfo = gameData.botData.info [i] + objId;
//	Boost shield for Thief and Buddy based on level.
shield = pRobotInfo->strength;
if (pRobotInfo->thief || pRobotInfo->companion) {
	shield = (shield * (abs (missionManager.nCurrentLevel) + 7)) / 8;
	if (pRobotInfo->companion) {
		//	Now, scale guide-bot hits by skill level
		switch (gameStates.app.nDifficultyLevel) {
			case 0:
				shield = I2X (20000);
				break;		//	Trainee, basically unkillable
			case 1:
				shield *= 3;
				break;		//	Rookie, pretty dang hard
			case 2:
				shield *= 2;
				break;		//	Hotshot, a bit tough
			default:
				break;
			}
		}
	}
else if (pRobotInfo->bossFlag) {	//	MK, 01/16/95, make boss shield lower on lower diff levels.
	shield = shield / (DIFFICULTY_LEVEL_COUNT + 3) * (gameStates.app.nDifficultyLevel + 4);
//	Additional wimpification of bosses at Trainee
	if (gameStates.app.nDifficultyLevel == 0)
		shield /= 2;
	}
return shield;
}

//------------------------------------------------------------------------------
//	Initialize default parameters for one robot, copying from gameData.botData.pInfo to *pObj.
//	What about setting size!?  Where does that come from?
void CopyDefaultsToRobot (CObject *pObj)
{
pObj->SetShield (RobotDefaultShield (pObj));
}

//------------------------------------------------------------------------------
//	Copy all values from the robot info structure to all instances of robots.
//	This allows us to change bitmaps.tbl and have these changes manifested in existing robots.
//	This function should be called at level load time.
void CopyDefaultsToRobotsAll (void)
{
	CObject*	pObj;

FORALL_ROBOT_OBJS (pObj)
	CopyDefaultsToRobot (pObj);

}

//------------------------------------------------------------------------------

void DoPlayerDead (void)
{
if (gameStates.menus.nInMenu)
	return;

int32_t bSecret = (missionManager.nCurrentLevel < 0);

gameStates.app.bGameRunning = 0;
StopTriggeredSounds ();
audio.PauseAll ();		//kill any continuing sounds (eg. forcefield hum)
DeadPlayerEnd ();		//terminate death sequence (if playing)
paletteManager.ResetEffect ();
if (IsCoopGame && gameStates.app.bHaveExtraGameInfo [1])
	LOCALPLAYER.score =
	(LOCALPLAYER.score * (100 - nCoopPenalties [(int32_t) extraGameInfo [1].nCoopPenalty])) / 100;
if (gameStates.multi.bPlayerIsTyping [N_LOCALPLAYER] && IsMultiGame)
	MultiSendMsgQuit ();
gameStates.entropy.bConquering = 0;

if (IsMultiGame)
	MultiDoDeath (LOCALPLAYER.nObject);
else {
	if (!--LOCALPLAYER.lives) {
		DoGameOver ();
		return;
		}
	}
if (gameData.reactorData.bDestroyed) {
	//clear out stuff so no bonus
	LOCALPLAYER.hostages.nOnBoard = 0;
	LOCALPLAYER.SetEnergy (0);
	LOCALPLAYER.SetShield (0);
	CONNECT (N_LOCALPLAYER, CONNECT_DIED_IN_MINE);
	DiedInMineMessage (); // Give them some indication of what happened
	}
// if player dead, always leave D2 secret level, but only leave D1 secret level if reactor destroyed
if (bSecret && (!gameStates.app.bD1Mission || gameData.reactorData.bDestroyed)) {
	ExitSecretLevel ();
	ResetShipData (-1, 2);
	gameStates.render.cockpit.nLastDrawn [0] =
	gameStates.render.cockpit.nLastDrawn [1] = -1;
	}
else {
	if (gameData.reactorData.bDestroyed) {
		FindNextLevel ();
		AdvanceLevel (0, 0);
		ResetShipData (-1, 2);
		}
	else if (!gameStates.entropy.bExitSequence) {
		ResetShipData (-1, 2);
		gameStates.app.nSpawnPos = -1;
		StartLevel (0x7fffffff, 1);
		}
	}
audio.ResumeAll ();
}

//------------------------------------------------------------------------------
//eof
