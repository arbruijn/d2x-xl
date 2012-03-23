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
#include "fuelcen.h"
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

#if defined (TACTILE)
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
void ShowLevelIntro (int nLevel);
void InitPlayerPosition (int bRandom);
void ReturningToLevelMessage (void);
void AdvancingToLevelMessage (void);
void DoEndGame (void);
void AdvanceLevel (int bSecret, int bFromSecret);
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

void SetFunctionMode (int);
void InitHoardData (void);
void FreeHoardData (void);

extern int nLastLevelPathCreated;
extern int nTimeLastMoved;
extern int nDescentCriticalError;
extern int nLastMsgYCrd;

//--------------------------------------------------------------------

void VerifyConsoleObject (void)
{
Assert (N_LOCALPLAYER > -1);
Assert (LOCALPLAYER.nObject > -1);
gameData.objs.consoleP = OBJECTS + LOCALPLAYER.nObject;
Assert (gameData.objs.consoleP->info.nType == OBJ_PLAYER);
Assert (gameData.objs.consoleP->info.nId == N_LOCALPLAYER);
}

//------------------------------------------------------------------------------

int CountRobotsInLevel (void)
{
	int robotCount = 0;
	//int 		i;
	CObject	*objP;

FORALL_ROBOT_OBJS (objP, i)
	robotCount++;
return robotCount;
}

//------------------------------------------------------------------------------

int CountHostagesInLevel (void)
{
	int 		count = 0;
	//int 		i;
	CObject	*objP;

FORALL_STATIC_OBJS (objP, i)
	if (objP->info.nType == OBJ_HOSTAGE)
		count++;
return count;
}

//------------------------------------------------------------------------------
//added 10/12/95: delete buddy bot if coop game.  Probably doesn't really belong here. -MT
void GameStartInitNetworkPlayers (void)
{
	int		i, j, t, bCoop = IsCoopGame,
				segNum, segType,
				playerObjs [MAX_PLAYERS], startSegs [MAX_PLAYERS],
				nPlayers, nMaxPlayers = bCoop ? MAX_COOP_PLAYERS + 1 : MAX_PLAYERS;
	CObject	*objP, *nextObjP;

	// Initialize network player start locations and CObject numbers

memset (gameStates.multi.bPlayerIsTyping, 0, sizeof (gameStates.multi.bPlayerIsTyping));
//VerifyConsoleObject ();
nPlayers = 0;
j = 0;
for (objP = gameData.objs.lists.all.head; objP; objP = nextObjP) {
	i = objP->Index ();
	nextObjP = objP->Links (0).next;
	t = objP->info.nType;
	if ((t == OBJ_PLAYER) || (t == OBJ_GHOST) || (t == OBJ_COOP)) {
		if ((nPlayers >= nMaxPlayers) || (bCoop ? (j && (t != OBJ_COOP)) : (t == OBJ_COOP)))
			ReleaseObject (short (i));
		else {
			playerObjs [nPlayers] = i;
			startSegs [nPlayers] = objP->info.nSegment;
			nPlayers++;
			}
		j++;
		}
	else if (t == OBJ_ROBOT) {
		if (ROBOTINFO (objP->info.nId).companion && IsMultiGame)
			ReleaseObject (short (i));		//kill the buddy in netgames
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
		segType = bCoop ? SEGMENTS [segNum].m_function : SEGMENT_FUNC_NONE;
#if 0
		switch (segType) {
			case SEGMENT_FUNC_GOAL_RED:
			case SEGMENT_FUNC_TEAM_RED:
				if (i < nPlayers / 2) // (GetTeam (i) != TEAM_RED)
					continue;
				SEGMENTS [segNum].m_nType = SEGMENT_FUNC_NONE;
				break;
			case SEGMENT_FUNC_GOAL_BLUE:
			case SEGMENT_FUNC_TEAM_BLUE:
				if (i >= nPlayers / 2) //GetTeam (i) != TEAM_BLUE)
					continue;
				SEGMENTS [segNum].m_nType = SEGMENT_FUNC_NONE;
				break;
			default:
				break;
			}
#endif
		objP = OBJECTS + playerObjs [j];
		objP->SetType (OBJ_PLAYER);
		gameData.multiplayer.playerInit [i].position = objP->info.position;
		gameData.multiplayer.playerInit [i].nSegment = objP->info.nSegment;
		gameData.multiplayer.playerInit [i].nSegType = segType;
		gameData.multiplayer.players [i].SetObject (playerObjs [j]);
		objP->info.nId = i;
		startSegs [j] = -1;
		break;
		}
	}
gameData.objs.viewerP = gameData.objs.consoleP = OBJECTS.Buffer (); // + LOCALPLAYER.nObject;
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
		if (gameData.multiplayer.players [i].connected && !(netPlayers [0].m_info.players [i].versionMinor & 0xF0)) {
			MsgBox ("Warning!", NULL, 1, TXT_OK,
								 "This special version of Descent II\nwill disconnect after this level.\nPlease purchase the full version\nto experience all the levels!");
			return;
			}
	}
if (IS_MAC_SHARE && IsMultiGame && (missionManager.nCurrentMission == missionManager.nBuiltInMission [0]) && (missionManager.nCurrentLevel == 4)) {
	for (i = 0; i < nPlayers; i++)
		if (gameData.multiplayer.players [i].connected && !(netPlayers [0].m_info.players [i].versionMinor & 0xF0)) {
			MsgBox ("Warning!", NULL, 1 , TXT_OK,
								 "This shareware version of Descent II\nwill disconnect after this level.\nPlease purchase the full version\nto experience all the levels!");
			return;
			}
	}
}

//------------------------------------------------------------------------------

void GameStartRemoveUnusedPlayers (void)
{
	int i;

	// 'Remove' the unused players

if (IsMultiGame) {
	for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++) {
		if (!gameData.multiplayer.players [i].connected || (i >= gameData.multiplayer.nPlayers))
			MultiMakePlayerGhost (i);
		}
	}
else {		// Note link to above if!!!
	for (i = 1; i < gameData.multiplayer.nPlayerPositions; i++)
		ReleaseObject ((short) gameData.multiplayer.players [i].nObject);
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
gameData.multiplayer.nBuiltinMissiles = BUILTIN_MISSILES;
if (LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER) 
	gameData.physics.xAfterburnerCharge = I2X (1);
OBJECTS [N_LOCALPLAYER].ResetDamage ();
}

//------------------------------------------------------------------------------

// Setup player for new game
void ResetPlayerData (bool bNewGame, bool bSecret, bool bRestore, int nPlayer)
{
CPlayerData* playerP = gameData.multiplayer.players + ((nPlayer < 0) ? N_LOCALPLAYER : nPlayer);

playerP->numKillsLevel = 0;
playerP->numRobotsLevel = CountRobotsInLevel ();
playerP->Setup ();
if (bNewGame) {
	playerP->score = 0;
	playerP->lastScore = 0;
	playerP->lives = gameStates.gameplay.nInitialLives;
	playerP->level = 1;
	playerP->timeTotal = 0;
	playerP->hoursLevel = 0;
	playerP->hoursTotal = 0;
	playerP->SetEnergy (gameStates.gameplay.InitialEnergy ());
	playerP->SetShield (gameStates.gameplay.InitialShield ());
	playerP->nKillerObj = -1;
	playerP->netKilledTotal = 0;
	playerP->netKillsTotal = 0;
	playerP->numKillsLevel = 0;
	playerP->numKillsTotal = 0;
	playerP->numRobotsTotal = 0;
	playerP->nScoreGoalCount = 0;
	playerP->hostages.nRescued = 0;
	playerP->numRobotsTotal = playerP->numRobotsLevel;
	playerP->hostages.nLevel = CountHostagesInLevel ();
	playerP->hostages.nTotal += playerP->hostages.nLevel;
	playerP->hostages.nOnBoard = 0;
	playerP->SetLaserLevels (0, 0);
	playerP->flags = 0;
	playerP->nCloaks =
	playerP->nInvuls = 0;
	if (nPlayer == N_LOCALPLAYER)
		ResetShipData (bRestore);
	if (IsMultiGame && !IsCoopGame) {
		if (IsTeamGame && gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bTeamDoors)
			playerP->flags |= KEY_GOLD | TEAMKEY (N_LOCALPLAYER);
		else
			playerP->flags |= (KEY_BLUE | KEY_RED | KEY_GOLD);
		}
	else
		playerP->timeLevel = 0;
	if ((nPlayer = N_LOCALPLAYER))
		gameStates.app.bFirstSecretVisit = 1;
	}
else {
	playerP->lastScore = playerP->score;
	playerP->level = missionManager.nCurrentLevel;
	if (!networkData.nJoinState) {
		playerP->timeLevel = 0;
		playerP->hoursLevel = 0;
		}
	playerP->nKillerObj = -1;
	playerP->hostages.nOnBoard = 0;
	if (!bSecret) {
		InitAmmoAndEnergy ();
		playerP->flags &=	~(PLAYER_FLAGS_INVULNERABLE | PLAYER_FLAGS_CLOAKED | KEY_BLUE | KEY_RED | KEY_GOLD);
		if (!(extraGameInfo [IsMultiGame].loadout.nDevice & PLAYER_FLAGS_FULLMAP))
			playerP->flags &=	~PLAYER_FLAGS_FULLMAP;
		playerP->cloakTime = 0;
		playerP->invulnerableTime = 0;
		if (IsMultiGame && !IsCoopGame) {
			if (IsTeamGame && gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bTeamDoors)
				playerP->flags |= KEY_GOLD | TEAMKEY (N_LOCALPLAYER);
			else
				playerP->flags |= (KEY_BLUE | KEY_RED | KEY_GOLD);
			}
		}
	else if (gameStates.app.bD1Mission)
		InitAmmoAndEnergy ();
	gameStates.app.bPlayerIsDead = 0; // Added by RH
	playerP->homingObjectDist = -I2X (1); // Added by RH
	gameData.laser.xLastFiredTime =
	gameData.laser.xNextFireTime =
	gameData.missiles.xLastFiredTime =
	gameData.missiles.xNextFireTime = gameData.time.xGame; // added by RH, solved demo playback bug
	controls [0].afterburnerState = 0;
	gameStates.gameplay.bLastAfterburnerState = 0;
	audio.DestroyObjectSound (playerP->nObject);
#ifdef TACTILE
	if (TactileStick)
		tactile_set_button_jolt ();
#endif
	gameData.objs.missileViewerP = NULL;
	}
gameData.bosses.ResetHitTimes ();
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
	else if (IsBuiltinWeapon (LASER_INDEX)) {
		LOCALPLAYER.SetStandardLaser (MAX_LASER_LEVEL);
		}
	if (!bRestore && IsBuiltinWeapon (VULCAN_INDEX) | IsBuiltinWeapon (GAUSS_INDEX))
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

// Setup player for a brand-new ship
void ResetShipData (bool bRestore)
{
	int	i;

gameStates.app.bChangingShip = 0;
if (gameData.demo.nState == ND_STATE_RECORDING) {
	NDRecordLaserLevel (LOCALPLAYER.LaserLevel (), 0);
	NDRecordPlayerWeapon (0, 0);
	NDRecordPlayerWeapon (1, 0);
	}

if (gameOpts->gameplay.nShip [1] < 0)
	gameOpts->gameplay.nShip [1] = (missionConfig.m_playerShip < 0) ? missionConfig.SelectShip (gameOpts->gameplay.nShip [0]) : missionConfig.SelectShip (missionConfig.m_playerShip);
if (gameOpts->gameplay.nShip [1] > -1) {
	gameOpts->gameplay.nShip [0] = missionConfig.SelectShip (gameOpts->gameplay.nShip [1]);
	gameOpts->gameplay.nShip [1] = -1;
	}
gameData.multiplayer.weaponStates [N_LOCALPLAYER].nShip = gameOpts->gameplay.nShip [0];

LOCALPLAYER.Setup ();
LOCALPLAYER.SetEnergy (gameStates.gameplay.InitialEnergy ());
LOCALPLAYER.SetShield (gameStates.gameplay.InitialShield ());
LOCALPLAYER.SetLaserLevels (0, 0);
LOCALPLAYER.nKillerObj = -1;
LOCALPLAYER.hostages.nOnBoard = 0;

for (i = 0; i < MAX_PRIMARY_WEAPONS; i++) {
	LOCALPLAYER.primaryAmmo [i] = 0;
	bLastPrimaryWasSuper [i] = 0;
	}
for (i = 1; i < MAX_SECONDARY_WEAPONS; i++) {
	LOCALPLAYER.secondaryAmmo [i] = 0;
	bLastSecondaryWasSuper [i] = 0;
	}
gameData.multiplayer.nBuiltinMissiles = BUILTIN_MISSILES;
LOCALPLAYER.secondaryAmmo [0] = BUILTIN_MISSILES;
LOCALPLAYER.primaryWeaponFlags = HAS_LASER_FLAG;
LOCALPLAYER.secondaryWeaponFlags = HAS_CONCUSSION_FLAG;
gameData.weapons.nOverridden = 0;
gameData.weapons.nPrimary = 0;
gameData.weapons.nSecondary = 0;
gameData.multiplayer.weaponStates [N_LOCALPLAYER].nAmmoUsed = 0;
LOCALPLAYER.flags &= ~
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
gameData.physics.xAfterburnerCharge = (LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER) ? I2X (1) : 0;
LOCALPLAYER.cloakTime = 0;
LOCALPLAYER.invulnerableTime = 0;
gameStates.app.bPlayerIsDead = 0;		//player no longer dead
LOCALPLAYER.homingObjectDist = -I2X (1); // Added by RH
controls [0].afterburnerState = 0;
gameStates.gameplay.bLastAfterburnerState = 0;
audio.DestroyObjectSound (N_LOCALPLAYER);
gameData.objs.missileViewerP = NULL;		///reset missile camera if out there
#ifdef TACTILE
	if (TactileStick)
 {
	tactile_set_button_jolt ();
	}
#endif
// When the ship got blown up, its root submodel had been converted to a debris object.
// Make it a player object again.
CObject* objP = OBJECTS.Buffer () ? OBJECTS + LOCALPLAYER.nObject : NULL;
if (objP) {
	OBJECTS [N_LOCALPLAYER].ResetDamage ();
	AddPlayerLoadout (bRestore);
	objP->info.nType = OBJ_PLAYER;
	objP->SetLife (IMMORTAL_TIME);
	objP->info.nFlags = 0;
	objP->rType.polyObjInfo.nSubObjFlags = 0;
	objP->mType.physInfo.flags = PF_TURNROLL | PF_LEVELLING | PF_WIGGLE | PF_USES_THRUST;
	}
InitAIForShip ();
}

//------------------------------------------------------------------------------

//do whatever needs to be done when a player dies in multiplayer
void DoGameOver (void)
{
//	MsgBox (TXT_GAME_OVER, 1, TXT_OK, "");
if (missionManager.nCurrentMission == missionManager.nBuiltInMission [0])
	MaybeAddPlayerScore (0);
SetFunctionMode (FMODE_MENU);
gameData.app.SetGameMode (GM_GAME_OVER);
longjmp (gameExitPoint, 0);		// Exit out of game loop
}

//------------------------------------------------------------------------------

//update various information about the player
void UpdatePlayerStats (void)
{
LOCALPLAYER.timeLevel += gameData.time.xFrame;	//the never-ending march of time...
if (LOCALPLAYER.timeLevel > I2X (3600)) {
	LOCALPLAYER.timeLevel -= I2X (3600);
	LOCALPLAYER.hoursLevel++;
	}
LOCALPLAYER.timeTotal += gameData.time.xFrame;	//the never-ending march of time...
if (LOCALPLAYER.timeTotal > I2X (3600)) {
	LOCALPLAYER.timeTotal -= I2X (3600);
	LOCALPLAYER.hoursTotal++;
	}
}

//	------------------------------------------------------------------------------

void SetVertigoRobotFlags (void)
{
	CObject	*objP;
	//int		i;

gameData.objs.nVertigoBotFlags = 0;
FORALL_ROBOT_OBJS (objP, i)
	if ((objP->info.nId >= 66) && !IS_BOSS (objP))
		gameData.objs.nVertigoBotFlags |= (1 << (objP->info.nId - 64));
}


//------------------------------------------------------------------------------

char *LevelName (int nLevel)
{
return (gameStates.app.bAutoRunMission && *szAutoMission) ? szAutoMission : (nLevel < 0) ?
		 missionManager.szSecretLevelNames [-nLevel-1] :
		 missionManager.szLevelNames [nLevel-1];
}

//------------------------------------------------------------------------------

char *MakeLevelFilename (int nLevel, char *pszFilename, const char *pszFileExt)
{
CFile::ChangeFilenameExtension (pszFilename, strlwr (LevelName (nLevel)), pszFileExt);
return pszFilename;
}

//------------------------------------------------------------------------------

char *LevelSongName (int nLevel)
{
	char szNoSong[] = "";

return gameStates.app.bAutoRunMission ? szNoSong : missionManager.szSongNames [nLevel];
}

//------------------------------------------------------------------------------

void UnloadLevelData (int bRestore, bool bQuit)
{
paletteManager.EnableEffect (true);
if (bQuit)
	EndRenderThreads ();
else {
	EndEffectsThread ();
	}
ResetModFolders ();
textureManager.Destroy ();
gameStates.render.bOmegaModded = 0;
gameOpts->render.textures.bUseHires [0] = gameOpts->render.textures.bUseHires [1];
gameOpts->render.bHiresModels [0] = gameOpts->render.bHiresModels [1];
if (gameOpts->UseHiresSound () != gameOpts->sound.bHires [1]) {
	gameOpts->sound.bHires [0] = gameOpts->sound.bHires [1];
	audio.Reset ();
	}
/*---*/PrintLog (1, "unloading mine rendering data\n");
gameData.render.mine.Destroy ();
PrintLog (-1);

/*---*/PrintLog (1, "stopping sounds\n");
audio.DestroyObjectSound (LOCALPLAYER.nObject);
audio.StopAllChannels ();
songManager.FreeUserSongs (1);
PrintLog (-1);

/*---*/PrintLog (1, "reconfiguring audio\n");
if (!bRestore) {
	gameStates.gameplay.slowmo [0].fSpeed =
	gameStates.gameplay.slowmo [1].fSpeed = 1;
	gameStates.gameplay.slowmo [0].nState =
	gameStates.gameplay.slowmo [1].nState = 0;
	if (audio.SlowDown () != 1.0f) {
		audio.Shutdown ();
		audio.Setup (1);
		}
	}
PrintLog (-1);
/*---*/PrintLog (1, "unloading lightmaps\n");
lightmapManager.Destroy ();
PrintLog (-1);

/*---*/PrintLog (1, "unloading hoard data\n");
/*---*/FreeHoardData ();
PrintLog (-1);

/*---*/PrintLog (1, "unloading textures\n");
UnloadTextures ();
PrintLog (-1);

/*---*/PrintLog (1, "unloading custom sounds\n");
FreeSoundReplacements ();
PrintLog (-1);

/*---*/PrintLog (1, "unloading addon sounds\n");
FreeAddonSounds ();
PrintLog (-1);

/*---*/PrintLog (1, "unloading hardware lights\n");
lightManager.Reset ();
PrintLog (-1);

/*---*/PrintLog (1, "unloading hires models\n");
FreeHiresModels (1);
PrintLog (-1);

/*---*/PrintLog (1, "unloading cambot\n");
UnloadCamBot ();
PrintLog (-1);

/*---*/PrintLog (1, "unloading additional models\n");
FreeModelExtensions ();
PrintLog (-1);

/*---*/PrintLog (1, "unloading additional model textures\n");
FreeObjExtensionBitmaps ();
PrintLog (-1);

/*---*/PrintLog (1, "unloading additional model textures\n");
UnloadHiresAnimations ();
PrintLog (-1);

/*---*/PrintLog (1, "freeing spark effect buffers\n");
sparkManager.Destroy ();
PrintLog (-1);

/*---*/PrintLog (1, "freeing auxiliary poly model data\n");
gameData.models.Destroy ();
PrintLog (-1);

/*---*/PrintLog (1, "Destroying camera objects\n");
cameraManager.Destroy ();
PrintLog (-1);

/*---*/PrintLog (1, "Destroying omega lightnings\n");
omegaLightning.Destroy (-1);
PrintLog (-1);

/*---*/PrintLog (1, "Destroying monsterball\n");
RemoveMonsterball ();
PrintLog (-1);

/*---*/PrintLog (1, "Unloading way points\n");
wayPointManager.Destroy ();
PrintLog (-1);

/*---*/PrintLog (1, "Unloading mod text messages\n");
FreeModTexts ();
PrintLog (-1);
}

//------------------------------------------------------------------------------

int LoadModData (char* pszLevelName, int bLoadTextures, int nStage)
{
	int	nLoadRes = 0;
	char	szFile [FILENAME_LEN];

// try to read mod files, and load default files if that fails
PrintLog (1, "loading mod data (state %d)\n", nStage);
if (nStage == 0) {
	SetD1Sound ();
	SetDataVersion (-1);
	if (ReadHamFile (false))
		gameStates.app.bCustomData = true;
	else if (gameStates.app.bCustomData) {
		ReadHamFile ();
		gameStates.app.bCustomData = false;
		}
#if 0
	LoadD2Sounds (true);
#else
	if (gameStates.app.bD1Mission)
		LoadD1Sounds (false);
	else
		LoadD2Sounds (false);
	LoadAddonSounds ();
	gameStates.app.bCustomSounds = false;
	if (gameStates.app.bHaveMod && (gameStates.app.bD1Mission ? LoadD1Sounds (true) : LoadD2Sounds (true))) {
		gameStates.app.bCustomSounds = true;
		if (gameOpts->UseHiresSound () != gameOpts->sound.bHires [1]) {
			WaitForSoundThread ();
			audio.Reset ();
			songManager.PlayLevelSong (missionManager.nCurrentLevel, 1);
			}
		}
#endif
	if (missionManager.nEnhancedMission) {
		sprintf (szFile, "d2x.ham");
		/*---*/PrintLog (1, "trying vertigo custom robots (d2x.ham)\n");
		nLoadRes = LoadRobotExtensions ("d2x.ham", gameFolders.szMissionDir, missionManager.nEnhancedMission);
		PrintLog (-1);
		}
	if (gameStates.app.bHaveMod) {
		/*---*/PrintLog (1, "trying custom robots (hxm) from mod '%s'\n", gameFolders.szModName);
		if (LoadRobotReplacements (gameFolders.szModName, gameFolders.szModDir [1], 0, 0, true, false))
			gameStates.app.bCustomData = true;
		PrintLog (-1);
		}
	if (missionManager.nEnhancedMission) {
		/*---*/PrintLog (1, "reading additional robots\n");
		if (missionManager.nEnhancedMission < 3)
			nLoadRes = 0;
		else {
			sprintf (szFile, "%s.vham", gameFolders.szModName);
			/*---*/PrintLog (1, "trying custom robots (vham) from mod '%s'\n", gameFolders.szModName);
			nLoadRes = LoadRobotExtensions (szFile, gameFolders.szModDir [1], missionManager.nEnhancedMission);
			PrintLog (-1);
			}
		if (nLoadRes == 0) {
			sprintf (szFile, "%s.ham", gameStates.app.szCurrentMissionFile);
			/*---*/PrintLog (1, "trying custom robots (ham) from level '%s'\n", gameStates.app.szCurrentMissionFile);
			nLoadRes = LoadRobotExtensions (szFile, gameFolders.szMissionDirs [0], missionManager.nEnhancedMission);
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
		LoadTextData (pszLevelName, ".snd", &gameData.sounds);
		LoadReplacementBitmaps (pszLevelName);
		LoadSoundReplacements (pszLevelName);
		particleImageManager.LoadAll ();
		}

	/*---*/PrintLog (1, "loading cambot\n");
	gameData.bots.nCamBotId = (LoadRobotReplacements ("cambot.hxm", NULL, 1, 0) > 0) ? gameData.bots.nTypes [0] - 1 : -1;
	PrintLog (-1);
	gameData.bots.nCamBotModel = gameData.models.nPolyModels - 1;
	/*---*/PrintLog (1, "loading replacement robots\n");
	if (0 > LoadRobotReplacements (pszLevelName, NULL, 0, 0, true)) {
		PrintLog (-1);
		return -1;
		}
	PrintLog (-1);

	/*---*/PrintLog (1, "loading replacement models\n");
	if (*gameFolders.szModelDir [1]) {
		if (missionManager.nCurrentLevel < 0) {
			sprintf (gameFolders.szModelDir [2], "%s/slevel%02d", gameFolders.szModelDir [1], -missionManager.nCurrentLevel);
			sprintf (gameFolders.szModelCacheDir [2], "%s/slevel%02d", gameFolders.szModelCacheDir [1], -missionManager.nCurrentLevel);
			}
		else {
			sprintf (gameFolders.szModelDir [2], "%s/level%02d", gameFolders.szModelDir [1], missionManager.nCurrentLevel);
			sprintf (gameFolders.szModelCacheDir [2], "%s/level%02d", gameFolders.szModelCacheDir [1], missionManager.nCurrentLevel);
			}
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
PrintLog (-1);
return nLoadRes;
}

//------------------------------------------------------------------------------

static void CleanupBeforeGame (int nLevel, int bRestore)
{
/*---*/PrintLog (1, "cleaning up...\n");
EndRenderThreads ();
transparencyRenderer.ResetBuffers ();
gameData.Destroy ();
srand (SDL_GetTicks ());
gameData.time.tLast = 0;
gameStates.render.nLightingMethod = gameStates.app.bNostalgia ? 0 : gameOpts->render.nLightingMethod;
gameStates.app.bBetweenLevels = 1;
gameStates.render.bFreeCam = (gameStates.render.bEnableFreeCam ? 0 : -1);
gameStates.app.bGameRunning = 0;
gameData.multiplayer.players [N_LOCALPLAYER].m_bExploded = 0;
gameData.physics.side.nSegment = -1;
gameData.physics.side.nSide = -1;
markerManager.Init ();
gameStates.gameplay.bKillBossCheat = 0;
gameStates.render.nFlashScale = I2X (1);
gameOpts->app.nScreenShotInterval = 0;	//better reset this every time a level is loaded
automap.m_bFull = 0;
ogl.m_data.nHeadlights = -1;
gameData.render.nColoredFaces = 0;
gameData.app.nFrameCount = 0;
gameData.app.nMineRenderCount = 0;
gameData.objs.lists.Init ();
memset (gameData.app.semaphores, 0, sizeof (gameData.app.semaphores));
transparencyRenderer.Init ();
#if PROFILING
PROF_INIT
#endif
memset (gameData.stats.player, 0, sizeof (tPlayerStats));
gameData.render.mine.bObjectRendered.Clear (char (0xff));
gameData.render.mine.bRenderSegment.Clear (char (0xff));
gameData.render.mine.bCalcVertexColor.Clear (0);
memset (gameData.multiplayer.weaponStates, 0xff, sizeof (gameData.multiplayer.weaponStates));
memset (gameData.multiplayer.bWasHit, 0, sizeof (gameData.multiplayer.bWasHit));
memset (gameData.multiplayer.nLastHitTime, 0, sizeof (gameData.multiplayer.nLastHitTime));
memset (gameData.weapons.firing, 0, sizeof (gameData.weapons.firing));
gameData.objs.objects.Clear ();
lightClusterManager.Init ();
gameData.render.faceIndex.Init ();
lightManager.ResetIndex ();
memset (gameData.objs.guidedMissile, 0, sizeof (gameData.objs.guidedMissile));
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
PrintLog (-1);
paletteManager.EnableEffect (true);
PrintLog (-1);
}

//------------------------------------------------------------------------------
//load a level off disk. level numbers start at 1.  Secret levels are -1,-2,-3

extern char szAutoMission [255];

int LoadLevel (int nLevel, bool bLoadTextures, bool bRestore)
{
	char*			pszLevelName;
	char			szHogName [FILENAME_LEN];
	CPlayerInfo	savePlayer;
	int			nRooms, bRetry = 0, nLoadRes, nCurrentLevel = missionManager.nCurrentLevel;

	static char szDefaultPlayList [] = "playlist.txt";

strlwr (pszLevelName = LevelName (nLevel));
/*---*/PrintLog (1, "loading level '%s'\n", pszLevelName);
CleanupBeforeGame (nLevel, bRestore);
gameStates.app.bD1Mission = gameStates.app.bAutoRunMission ? (strstr (szAutoMission, "rdl") != NULL) :
									 (missionManager [missionManager.nCurrentMission].nDescentVersion == 1);
MakeModFolders (hogFileManager.MissionName (), nLevel);
if (!(gameStates.app.bHaveMod || missionManager.IsBuiltIn (hogFileManager.MissionName ())))
	 MakeModFolders (gameStates.app.bD1Mission ? "Descent: First Strike" : "Descent 2: Counterstrike!", nLevel);
if (gameStates.app.bHaveMod)
	songManager.LoadPlayList (szDefaultPlayList, 1);
songManager.PlayLevelSong (missionManager.nCurrentLevel, 1);
lightManager.SetMethod ();
#if 1
if (LoadModData (NULL, 0, 0) < 0) {
	gameStates.app.bBetweenLevels = 0;
	missionManager.nCurrentLevel = nCurrentLevel;
	PrintLog (-1);
	return -1;
	}
#endif
/*---*/PrintLog (1, "Initializing particle manager\n");
InitObjectSmoke ();
PrintLog (-1);
gameData.pig.tex.bitmapColors.Clear ();
gameData.models.thrusters.Clear ();
savePlayer = LOCALPLAYER;
#if 0
Assert (gameStates.app.bAutoRunMission ||
		  ((nLevel <= missionManager.nLastLevel) &&
		   (nLevel >= missionManager.nLastSecretLevel) &&
			(nLevel != 0)));
#endif
if (!gameStates.app.bAutoRunMission &&
	 (!nLevel || (nLevel > missionManager.nLastLevel) || (nLevel < missionManager.nLastSecretLevel))) {
	gameStates.app.bBetweenLevels = 0;
	missionManager.nCurrentLevel = nCurrentLevel;
	Warning ("Invalid level number!");
	PrintLog (-1);
	return 0;
	}
#if 0
CCanvas::SetCurrent (NULL);
GrClearCanvas (BLACK_RGBA);		//so palette switching is less obvious
#endif
nLastMsgYCrd = -1;		//so we don't restore backgound under msg
if (!gameStates.app.bProgressBars)
	messageBox.Show (TXT_LOADING);
/*---*/PrintLog (1, "loading texture brightness and color info\n");
SetDataVersion (-1);

memcpy (gameData.pig.tex.brightness.Buffer (),
		  gameData.pig.tex.defaultBrightness [gameStates.app.bD1Mission].Buffer (),
		  gameData.pig.tex.brightness. Size ());
LoadTextureBrightness (pszLevelName, NULL);
gameData.render.color.textures = gameData.render.color.defaultTextures [gameStates.app.bD1Mission];
LoadTextureColors (pszLevelName, NULL);
PrintLog (-1);

/*---*/PrintLog (1, "loading mission configuration info\n");
missionConfig.Init ();
missionConfig.Load ();
missionConfig.Load (pszLevelName);
missionConfig.Apply ();
PrintLog (-1);

InitTexColors ();

for (;;) {
	if (!(nLoadRes = LoadLevelData (pszLevelName, nLevel)))
		break;	//actually load the data from disk!
	nLoadRes = 1;
	if (bRetry)
		break;
	if (strstr (hogFileManager.AltFiles ().szName, ".hog"))
		break;
	sprintf (szHogName, "%s%s%s%s",
				gameFolders.szMissionDir, *gameFolders.szMissionDir ? "/" : "",
				gameFolders.szMsnSubDir, pszLevelName);
	if (!hogFileManager.UseMission (szHogName))
		break;
	bRetry = 1;
	};
if (nLoadRes) {
	/*---*/PrintLog (0, "Couldn't load '%s' (%d)\n", pszLevelName, nLoadRes);
	gameStates.app.bBetweenLevels = 0;
	missionManager.nCurrentLevel = nCurrentLevel;
	Warning (TXT_LOAD_ERROR, pszLevelName);
	PrintLog (-1);
	return 0;
	}

if (!gameStates.app.bProgressBars)
	messageBox.Show (TXT_LOADING);
paletteManager.SetGame (paletteManager.Load (szCurrentLevelPalette, pszLevelName, 1, 1, 1));		//don't change screen

#if 1
if (LoadModData (pszLevelName, bLoadTextures, 1) < 0) {
	gameStates.app.bBetweenLevels = 0;
	missionManager.nCurrentLevel = nCurrentLevel;
	PrintLog (-1);
	return -1;
	}
#endif
lightManager.Setup (nLevel); 
/*---*/PrintLog (1, "loading endlevel data\n");
LoadEndLevelData (nLevel);
PrintLog (-1);

ResetNetworkObjects ();
ResetChildObjects ();
externalView.Reset (-1, -1);
ResetPlayerPaths ();
FixObjectSizes ();
wayPointManager.Setup (!bRestore);
/*---*/PrintLog (1, "counting entropy rooms\n");
nRooms = CountRooms ();
if (IsEntropyGame) {
	if (!nRooms) {
		Warning (TXT_NO_ENTROPY);
		gameData.app.nGameMode &= ~GM_ENTROPY;
		gameData.app.nGameMode |= GM_TEAM;
		}
	}
else if ((gameData.app.GameMode (GM_CAPTURE | GM_HOARD)) ||
			((gameData.app.GameMode (GM_MONSTERBALL)) == GM_MONSTERBALL)) {
/*---*/PrintLog (1, "gathering CTF+ flag goals\n");
	if (GatherFlagGoals () != 3) {
		Warning (TXT_NO_CTF);
		gameData.app.nGameMode &= ~GM_CAPTURE;
		gameData.app.nGameMode |= GM_TEAM;
		}
	}
PrintLog (-1);

gameData.render.lights.segDeltas.Clear ();
/*---*/PrintLog (1, "initializing door animations\n");
InitDoorAnims ();
PrintLog (-1);

(CPlayerInfo&) LOCALPLAYER = savePlayer;
gameData.hoard.nMonsterballSeg = -1;
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
gameData.render.color.vertices.Clear ();
gameData.render.color.segments.Clear ();
PrintLog (-1);
/*---*/PrintLog (1, "resetting speed boost information\n");
gameData.objs.speedBoost.Clear ();
PrintLog (-1);
if (!ogl.m_features.bStencilBuffer)
	extraGameInfo [0].bShadows = 0;
D2SetCaption ();
if (!bRestore) {
	gameData.render.lights.bInitDynColoring = 1;
	gameData.omega.xCharge [IsMultiGame] = MAX_OMEGA_CHARGE;
	SetMaxOmegaCharge ();
	ConvertObjects ();
	SetEquipGenStates ();
	SetupWalls ();
	SetupEffects ();
//	lightManager.Setup (nLevel);
	gameData.time.nPaused = 0;
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
ControlEffectsThread ();
ControlSoundThread ();
gameStates.render.bDepthSort = 1;
gameStates.app.bBetweenLevels = 0;
PrintLog (-1);
return 1;
}

//------------------------------------------------------------------------------

//starts a new game on the given level
int StartNewGame (int nLevel)
{
gameData.app.SetGameMode (GM_NORMAL);
SetFunctionMode (FMODE_GAME);
missionManager.SetNextLevel (0);
missionManager.SetNextLevel (-1, 1);
gameData.multiplayer.nPlayers = 1;
gameData.objs.nLastObject [0] = 0;
networkData.bNewGame = 0;
missionManager.DeleteLevelStates ();
missionManager.SaveLevelStates ();
InitMultiPlayerObject (0);
if (!StartNewLevel (nLevel, true)) {
	gameStates.app.bAutoRunMission = 0;
	return 0;
	}
LOCALPLAYER.startingLevel = nLevel;		// Mark where they started
GameDisableCheats ();
InitSeismicDisturbances ();
return 1;
}

//------------------------------------------------------------------------------

#ifndef _NETWORK_H
extern int NetworkEndLevelPoll2 (CMenu& menu, int& key, int nCurItem); // network.c
#endif

//	Does the bonus scoring.
//	Call with deadFlag = 1 if player died, but deserves some portion of bonus (only skill points), anyway.
void DoEndLevelScoreGlitz (int network)
{
	#define N_GLITZITEMS 11

	int			nLevelPoints, nSkillPoints, nEnergyPoints, nShieldPoints, nHostagePoints, nAllHostagePoints, nEndGamePoints;
	char			szAllHostages [64];
	char			szEndGame [64];
	char			szMenu [N_GLITZITEMS + 1][40];
	CMenu			m (N_GLITZITEMS + 1);
	int			i, c;
	char			szTitle [128];
	int			bIsLastLevel = 0;
	int			nMineLevel = 0;

audio.DestroyObjectSound (LOCALPLAYER.nObject);
audio.StopAllChannels ();
SetScreenMode (SCREEN_MENU);		//go into menu mode
if (gameStates.app.bHaveExtraData)

#ifdef TACTILE
if (TactileStick)
	ClearForces ();
#endif

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
if (network && IsNetworkGame)
	m.Menu (NULL, szTitle, NetworkEndLevelPoll2, NULL, BackgroundName (BG_STARS));
else
// NOTE LINK TO ABOVE!!!
gameStates.app.bGameRunning = 0;
backgroundManager.SetShadow (false);
gameStates.render.bRenderIndirect = -1;
ogl.ChooseDrawBuffer ();
m.Menu (NULL, szTitle, NULL, NULL, BackgroundName (BG_STARS));
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
int PSecretLevelDestroyed (void)
{
if (gameStates.app.bFirstSecretVisit)
	return 0;		//	Never been there, can't have been destroyed.
if (CFile::Exist (SECRETC_FILENAME, gameFolders.szSaveDir, 0))
	return 0;
return 1;
}

//	-----------------------------------------------------------------------------------------------------

void DoSecretMessage (const char *msg)
{
	int fMode = gameStates.app.nFunctionMode;

StopTime ();
SetFunctionMode (FMODE_MENU);
MsgBox (NULL, BackgroundName (BG_STARS), 1, TXT_OK, msg);
SetFunctionMode (fMode);
StartTime (0);
}

//	-----------------------------------------------------------------------------------------------------

void InitSecretLevel (int nLevel)
{
Assert (missionManager.nCurrentLevel == nLevel);	//make sure level set right
Assert (gameStates.app.nFunctionMode == FMODE_GAME);
meshBuilder.ComputeFaceKeys ();
GameStartInitNetworkPlayers (); // Initialize the gameData.multiplayer.players array for this level
HUDClearMessages ();
automap.ClearVisited ();
ResetPlayerData (false, true, false);
gameData.objs.viewerP = OBJECTS + LOCALPLAYER.nObject;
GameStartRemoveUnusedPlayers ();
if (gameStates.app.bGameSuspended & SUSP_TEMPORARY)
	gameStates.app.bGameSuspended &= ~(SUSP_ROBOTS | SUSP_TEMPORARY);
gameData.reactor.bDestroyed = 0;
cockpit->Init ();
paletteManager.ResetEffect ();
audio.Prepare ();
audio.SetupRouter ();
}

//------------------------------------------------------------------------------

//	Called from trigger.cpp when player is on a secret level and hits exit to return to base level.
void ExitSecretLevel (void)
{
if ((gameData.demo.nState == ND_STATE_RECORDING) || (gameData.demo.nState == ND_STATE_PAUSED))
	NDStopRecording ();
if (!(gameStates.app.bD1Mission || gameData.reactor.bDestroyed))
	saveGameManager.Save (0, 2, 0, SECRETC_FILENAME);
if (!gameStates.app.bD1Mission && CFile::Exist (SECRETB_FILENAME, gameFolders.szSaveDir, 0)) {
	int pwSave = gameData.weapons.nPrimary;
	int swSave = gameData.weapons.nSecondary;

	ReturningToLevelMessage ();
	saveGameManager.Load (1, 1, 0, SECRETB_FILENAME);
	SetD1Sound ();
	SetDataVersion (-1);
	SetPosFromReturnSegment (1);
	//LOCALPLAYER.lives--;	//	re-lose the life, LOCALPLAYER.lives got written over in restore.
	gameData.weapons.nPrimary = pwSave;
	gameData.weapons.nSecondary = swSave;
	}
else { // File doesn't exist, so can't return to base level.  Advance to next one.
	if (missionManager.nEntryLevel == missionManager.nLastLevel)
		DoEndGame ();
	else {
		if (!gameStates.app.bD1Mission)
			AdvancingToLevelMessage ();
		DoEndLevelScoreGlitz (0);
		StartNewLevel (missionManager.nEntryLevel + 1, false);
		}
	}
}

//------------------------------------------------------------------------------

//	Called from trigger.cpp when player is exiting via a directed exit
int ReenterLevel (void)
{
	char nState = missionManager.GetLevelState (missionManager.NextLevel ());

if (nState < 0) 
	return 0;

	char	szFile [FILENAME_LEN] = {'\0'};

if ((gameData.demo.nState == ND_STATE_RECORDING) || (gameData.demo.nState == ND_STATE_PAUSED))
	NDStopRecording ();
missionManager.LevelStateName (szFile);
if (!(gameStates.app.bD1Mission || gameData.reactor.bDestroyed))
	saveGameManager.Save (0, -1, 0, szFile);
else if (CFile::Exist (szFile, gameFolders.szCacheDir, 0))
	CFile::Delete (szFile, gameFolders.szSaveDir);
szFile [0] = '\x02';
missionManager.LevelStateName (szFile + 1, missionManager.NextLevel ());
if (!gameStates.app.bD1Mission && (nState > 0) && CFile::Exist (szFile, gameFolders.szCacheDir, 0)) {
	int pwSave = gameData.weapons.nPrimary;
	int swSave = gameData.weapons.nSecondary;
	saveGameManager.Load (1, -1, 0, szFile + 1);
	SetD1Sound ();
	SetDataVersion (-1);
	InitPlayerPosition (1);
	int nMaxPlayers = IsCoopGame ? MAX_COOP_PLAYERS + 1 : MAX_PLAYERS;
	for (int i = 0; i < nMaxPlayers; i++)
		gameData.multiplayer.bAdjustPowerupCap [i] = true;
	//LOCALPLAYER.lives--;	//	re-lose the life, LOCALPLAYER.lives got written over in restore.
	gameData.weapons.nPrimary = pwSave;
	gameData.weapons.nSecondary = swSave;
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
	LOCALPLAYER.invulnerableTime = gameData.time.xGame - time_used;
	}

if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
		fix	time_used;

	time_used = xOldGameTime - LOCALPLAYER.cloakTime;
	LOCALPLAYER.cloakTime = gameData.time.xGame - time_used;
	}
}

//------------------------------------------------------------------------------
//called when the player has finished a level
void PlayerFinishedLevel (int bSecret)
{
	Assert (!bSecret);

LOCALPLAYER.hostages.nRescued += LOCALPLAYER.hostages.nOnBoard;
if (IsNetworkGame)
	CONNECT (N_LOCALPLAYER, CONNECT_WAITING); // Finished but did not die
gameStates.render.cockpit.nLastDrawn [0] =
gameStates.render.cockpit.nLastDrawn [1] = -1;
if (missionManager.nCurrentLevel < 0)
	ExitSecretLevel ();
else
	AdvanceLevel (0,0);
}

//------------------------------------------------------------------------------

void PlayLevelMovie (const char *pszExt, int nLevel)
{
	char szFilename [FILENAME_LEN];

movieManager.Play (MakeLevelFilename (nLevel, szFilename, pszExt), MOVIE_OPTIONAL, 0, gameOpts->movies.bResize);
}

//------------------------------------------------------------------------------

void PlayLevelIntroMovie (int nLevel)
{
PlayLevelMovie (".mvi", nLevel);
}

//------------------------------------------------------------------------------

void PlayLevelExtroMovie (int nLevel)
{
PlayLevelMovie (".mvx", nLevel);
}

//------------------------------------------------------------------------------

#define MOVIE_REQUIRED 1

//called when the player has finished the last level
void DoEndGame (void)
{
SetFunctionMode (FMODE_MENU);
if ((gameData.demo.nState == ND_STATE_RECORDING) || (gameData.demo.nState == ND_STATE_PAUSED))
	NDStopRecording ();
SetScreenMode (SCREEN_MENU);
CCanvas::SetCurrent (NULL);
KeyFlush ();
if (!IsMultiGame) {
	if (missionManager.nCurrentMission == (gameStates.app.bD1Mission ? missionManager.nBuiltInMission [1] : missionManager.nBuiltInMission [0])) {
		int bPlayed = MOVIE_NOT_PLAYED;	//default is not bPlayed

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
		creditsManager.Show (szBriefing);
		}
	}
KeyFlush ();
if (IsMultiGame)
	MultiEndLevelScore ();
else
	// NOTE LINK TO ABOVE
	DoEndLevelScoreGlitz (0);

if ((missionManager.nCurrentMission == missionManager.nBuiltInMission [0]) && !(IsMultiGame || IsCoopGame)) {
	CCanvas::SetCurrent (NULL);
	CCanvas::Current ()->Clear (BLACK_RGBA);
	paletteManager.ResetEffect ();
	//paletteManager.Load (D2_DEFAULT_PALETTE, NULL, 0, 1, 0);
	MaybeAddPlayerScore (0);
	}
SetFunctionMode (FMODE_MENU);
if (gameData.app.GameMode (GM_SERIAL | GM_MODEM))
	gameData.app.SetGameMode (gameData.app.GameMode () | GM_GAME_OVER);		//preserve modem setting so go back into modem menu
else
	gameData.app.SetGameMode (GM_GAME_OVER);
longjmp (gameExitPoint, 0);		// Exit out of game loop
}

//------------------------------------------------------------------------------
//from which level each do you get to each secret level
//called to go to the next level (if there is one)
//if bSecret is true, advance to secret level, else next Normal one
//	Return true if game over.
void AdvanceLevel (int bSecret, int bFromSecret)
{
	int result;

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
if ((gameData.demo.nState == ND_STATE_RECORDING) || (gameData.demo.nState == ND_STATE_PAUSED))
	NDStopRecording ();
if (IsMultiGame) {
	if ((result = MultiEndLevel (&bSecret))) { // Wait for other players to reach this point
		gameStates.app.bBetweenLevels = 0;
		if (missionManager.nCurrentLevel != missionManager.nLastLevel)		//player has finished the game!
			return;
		longjmp (gameExitPoint, 0);		// Exit out of game loop
		}
	}

if (gameData.reactor.bDestroyed)
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
	if (!ReenterLevel ())
		StartNewLevel (missionManager.NextLevel (), false);
	}
gameStates.app.bBetweenLevels = 0;
}

//------------------------------------------------------------------------------

void DiedInMineMessage (void)
{
if (IsMultiGame)
	return;
paletteManager.DisableEffect ();
//SetScreenMode (SCREEN_MENU);		//go into menu mode
//CCanvas::SetCurrent (NULL);
//int funcMode = gameStates.app.nFunctionMode;
//SetFunctionMode (FMODE_MENU);
//gameStates.app.bGameRunning = 0;
//gameStates.app.bGameRunning = 1;
//SetFunctionMode (funcMode);
gameStates.app.bGameRunning = 0;
backgroundManager.SetShadow (false);
gameStates.render.bRenderIndirect = -1;
ogl.ChooseDrawBuffer ();
MsgBox (NULL, BackgroundName (BG_STARS), 1, TXT_OK, TXT_DIED_IN_MINE);
backgroundManager.SetShadow (true);
gameStates.app.bGameRunning = 1;
}

//------------------------------------------------------------------------------
//	Called when player dies on secret level.
void ReturningToLevelMessage (void)
{
	char	msg [128];

	int old_fmode;

if (IsMultiGame)
	return;
StopTime ();
paletteManager.DisableEffect ();
SetScreenMode (SCREEN_MENU);		//go into menu mode
CCanvas::SetCurrent (NULL);
old_fmode = gameStates.app.nFunctionMode;
SetFunctionMode (FMODE_MENU);
if (missionManager.nEntryLevel < 0)
	sprintf (msg, TXT_SECRET_LEVEL_RETURN);
else
	sprintf (msg, TXT_RETURN_LVL, missionManager.nEntryLevel);
MsgBox (NULL, BackgroundName (BG_STARS), 1, TXT_OK, msg);
SetFunctionMode (old_fmode);
StartTime (0);
}

//------------------------------------------------------------------------------
//	Called when player dies on secret level.
void AdvancingToLevelMessage (void)
{
	char	msg [128];

	int old_fmode;

	//	Only supposed to come here from a secret level.
Assert (missionManager.nCurrentLevel < 0);
if (IsMultiGame)
	return;
paletteManager.DisableEffect ();
SetScreenMode (SCREEN_MENU);		//go into menu mode
CCanvas::SetCurrent (NULL);
old_fmode = gameStates.app.nFunctionMode;
SetFunctionMode (FMODE_MENU);
sprintf (msg, "Base level destroyed.\nAdvancing to level %i", missionManager.nEntryLevel + 1);
MsgBox (NULL, BackgroundName (BG_STARS), 1, TXT_OK, msg);
SetFunctionMode (old_fmode);
}

//	-----------------------------------------------------------------------------------
//	Set the player's position from the globals gameData.segs.secret.nReturnSegment and gameData.segs.secret.returnOrient.
void SetPosFromReturnSegment (int bRelink)
{
	int	nPlayerObj = LOCALPLAYER.nObject;

OBJECTS [nPlayerObj].info.position.vPos = SEGMENTS [gameData.segs.secret.nReturnSegment].Center ();
if (bRelink)
	OBJECTS [nPlayerObj].RelinkToSeg (gameData.segs.secret.nReturnSegment);
ResetPlayerObject ();
OBJECTS [nPlayerObj].info.position.mOrient = gameData.segs.secret.returnOrient;
}

//------------------------------------------------------------------------------
//called when the player is starting a level (new game or new ship)
void StartLevel (int nLevel, int bRandom)
{
if (nLevel == 0x7fffffff)
	PrintLog (1, "restarting level\n");
else
	PrintLog (1, "starting level %d\n", nLevel);
Assert (!gameStates.app.bPlayerIsDead);
VerifyConsoleObject ();
PrintLog (0, "initializing player position\n");
InitPlayerPosition (bRandom);
gameData.objs.consoleP->info.controlType = CT_FLYING;
gameData.objs.consoleP->info.movementType = MT_PHYSICS;
MultiSendShield ();
DisableMatCens ();
PrintLog (0, "clearing transient objects\n");
ClearTransientObjects (0);		//0 means leave proximity bombs
// gameData.objs.consoleP->CreateAppearanceEffect ();
gameStates.render.bDoAppearanceEffect = 1;
if (IsMultiGame) {
	if (IsCoopGame)
		MultiSendScore ();
	MultiSendPosition (LOCALPLAYER.nObject);
	MultiSendReappear ();
	}
PrintLog (0, "keeping network busy\n");
if (IsNetworkGame)
	NetworkDoFrame (1, 1);
PrintLog (0, "resetting AI paths\n");
AIResetAllPaths ();
gameData.bosses.ResetHitTimes ();
PrintLog (0, "clearing stuck objects\n");
ClearStuckObjects ();
if (nLevel != 0x7fffffff)
	meshBuilder.ComputeFaceKeys ();
ResetTime ();
ResetRearView ();
gameData.fusion.xAutoFireTime = 0;
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

int PrepareLevel (int nLevel, bool bLoadTextures, bool bSecret, bool bRestore, bool bNewGame)
{
	int funcRes;

gameStates.multi.bTryAutoDL = 1;

reloadLevel:

if (!IsMultiGame) {
	gameStates.render.cockpit.nLastDrawn [0] =
	gameStates.render.cockpit.nLastDrawn [1] = -1;
	}
gameStates.render.cockpit.bBigWindowSwitch = 0;
if (gameData.demo.nState == ND_STATE_PAUSED)
	gameData.demo.nState = ND_STATE_RECORDING;
if (gameData.demo.nState == ND_STATE_RECORDING) {
	NDSetNewLevel (nLevel);
	NDRecordStartFrame (gameData.app.nFrameCount, gameData.time.xFrame);
	}
if (IsMultiGame)
	SetFunctionMode (FMODE_MENU); // Cheap fix to prevent problems with errror dialogs in loadlevel.
SetWarnFunc (ShowInGameWarning);
try {
	funcRes = LoadLevel (nLevel, bLoadTextures, bRestore);
	}
catch (int e) {
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
if (!funcRes) {
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
//	gameData.objs.viewerP = OBJECTS + LOCALPLAYER.nObject;
if (gameData.multiplayer.nPlayers > gameData.multiplayer.nPlayerPositions) {
	MsgBox (NULL, NULL, 1, TXT_OK, "Too many players for this level.");
#if 1
	while (gameData.multiplayer.nPlayers > gameData.multiplayer.nPlayerPositions) {
		--gameData.multiplayer.nPlayers;
		memset (gameData.multiplayer.players + gameData.multiplayer.nPlayers, 0, sizeof (gameData.multiplayer.players [gameData.multiplayer.nPlayers]));
		}
#else
	return 0;
#endif
	}
Assert (gameData.multiplayer.nPlayers <= gameData.multiplayer.nPlayerPositions);
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

for (int i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	ResetPlayerData (bNewGame, bSecret, bRestore, i);
if (IsCoopGame && networkData.nJoinState) {
	for (int i = 0; i < gameData.multiplayer.nPlayers; i++)
		gameData.multiplayer.players [i].flags |= netGame.PlayerFlags () [i];
	}

if (IsMultiGame)
	MultiPrepLevel (); // Removes robots from level if necessary
else
	ResetMonsterball (false); //will simply remove all Monsterballs
GameStartRemoveUnusedPlayers ();
if (gameStates.app.bGameSuspended & SUSP_TEMPORARY)
	gameStates.app.bGameSuspended &= ~(SUSP_ROBOTS | SUSP_TEMPORARY);
gameData.reactor.bDestroyed = 0;
gameStates.render.glFOV = DEFAULT_FOV;
SetScreenMode (SCREEN_GAME);
cockpit->Init ();
audio.Prepare ();
audio.SetupRouter ();
InitRobotsForLevel ();
InitShakerDetonates ();
MorphInit ();
InitAllMatCens ();
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
	gameData.models.Prepare ();
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
gameData.objs.consoleP->info.controlType = CT_FLYING;
gameData.objs.consoleP->info.movementType = MT_PHYSICS;
// -- WHY? -- DisableMatCens ();
ClearTransientObjects (0);		//0 means leave proximity bombs
// gameData.objs.consoleP->CreateAppearanceEffect ();
gameStates.render.bDoAppearanceEffect = 1;
AIResetAllPaths ();
ResetTime ();
ResetRearView ();
gameData.fusion.xAutoFireTime = 0;
gameData.SetFusionCharge (0);
gameStates.app.cheats.bRobotsFiring = 1;
SetD1Sound ();
if (gameStates.app.bD1Mission) {
	LOCALPLAYER.ResetEnergy (INITIAL_ENERGY);
	LOCALPLAYER.ResetShield (INITIAL_SHIELD);
	}
}

//------------------------------------------------------------------------------

static int PrepareSecretLevel (int nLevel, bool bLoadTextures)
{
	CMenu	menu (1);

gameStates.app.xThisLevelTime = 0;
menu.AddText ("", " ");
gameStates.render.cockpit.nLastDrawn [0] = -1;
gameStates.render.cockpit.nLastDrawn [1] = -1;

if (gameData.demo.nState == ND_STATE_PAUSED)
	gameData.demo.nState = ND_STATE_RECORDING;

if (gameData.demo.nState == ND_STATE_RECORDING) {
	NDSetNewLevel (nLevel);
	NDRecordStartFrame (gameData.app.nFrameCount, gameData.time.xFrame);
	}
else if (gameData.demo.nState != ND_STATE_PLAYBACK) {
	paletteManager.DisableEffect ();
	SetScreenMode (SCREEN_MENU);		//go into menu mode
	if (gameStates.app.bFirstSecretVisit)
		DoSecretMessage (gameStates.app.bD1Mission ? TXT_ALTERNATE_EXIT : TXT_SECRET_EXIT);
	else if (CFile::Exist (SECRETC_FILENAME,gameFolders.szSaveDir,0))
		DoSecretMessage (gameStates.app.bD1Mission ? TXT_ALTERNATE_EXIT : TXT_SECRET_EXIT);
	else {
		char	text_str [128];

		sprintf (text_str, TXT_ADVANCE_LVL, missionManager.nCurrentLevel+1);
		DoSecretMessage (text_str);
		}
	}

if (gameStates.app.bFirstSecretVisit || (gameData.demo.nState == ND_STATE_PLAYBACK)) {
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
	InitAllMatCens ();
	ResetSpecialEffects ();
	StartSecretLevel ();
	LOCALPLAYER.flags &= ~PLAYER_FLAGS_ALL_KEYS;
	}
else {
	if (CFile::Exist (SECRETC_FILENAME, gameFolders.szSaveDir, 0)) {
		int	pw_save, sw_save, nCurrentLevel;

		pw_save = gameData.weapons.nPrimary;
		sw_save = gameData.weapons.nSecondary;
		nCurrentLevel = missionManager.nCurrentLevel;
		saveGameManager.Load (1, 1, 0, SECRETC_FILENAME);
		missionManager.nEntryLevel = nCurrentLevel;
		gameData.weapons.nPrimary = pw_save;
		gameData.weapons.nSecondary = sw_save;
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
return 1;
}

//------------------------------------------------------------------------------
//	Called from switch.c when player passes through secret exit.  That means he was on a non-secret level and he
//	is passing to the secret level.
//	Do a savegame.
void EnterSecretLevel (void)
{
	fix	xOldGameTime;
	int	i;

Assert (!IsMultiGame);
missionManager.nEntryLevel = missionManager.nCurrentLevel;
if (gameData.reactor.bDestroyed)
	DoEndLevelScoreGlitz (0);
if (gameData.demo.nState != ND_STATE_PLAYBACK)
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
xOldGameTime = gameData.time.xGame;
PrepareSecretLevel (missionManager.NextLevel (), true);
// do_cloak_invul_stuff ();
}

//------------------------------------------------------------------------------

void CObject::Bash (ubyte nId)
{
gameData.multiplayer.powerupsInMine [info.nId] =
gameData.multiplayer.maxPowerupsAllowed [info.nId] = 0;
SetType (OBJ_POWERUP);
info.nId = nId;
info.renderType = RT_POWERUP;
info.controlType = CT_POWERUP;
SetSizeFromPowerup ();
rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [nId].nClipIndex;
rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][rType.vClipInfo.nClipIndex].xFrameTime;
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
  //int 		i;
	CObject	*objP;

FORALL_POWERUP_OBJS (objP, i)
	objP->BashToShield ((objP->info.nId == POW_REDFLAG) || (objP->info.nId == POW_BLUEFLAG));
}

//------------------------------------------------------------------------------

struct tIntroMovieInfo {
	int	nLevel;
	char	szMovieName [FILENAME_LEN];
} szIntroMovies [] = {{ 1,"pla"},
							 { 5,"plb"},
							 { 9,"plc"},
							 {13,"pld"},
							 {17,"ple"},
							 {21,"plf"},
							 {24,"plg"}};

#define NUM_INTRO_MOVIES (sizeof (szIntroMovies) / sizeof (*szIntroMovies))

void ShowLevelIntro (int nLevel)
{
//if shareware, show a briefing?
if (!IsMultiGame) {
	uint i, bPlayed = 0;

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
					bPlayed=1;
					break;
					}
				}
			if (movieManager.m_nRobots) {
				int hires_save = gameStates.menus.bHiresAvailable;
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

void MaybeSetFirstSecretVisit (int nLevel)
{
for (int i = 0; i < missionManager.nSecretLevels; i++) {
	if (missionManager.secretLevelTable [i] == nLevel) {
		gameStates.app.bFirstSecretVisit = 1;
		}
	}
}

//------------------------------------------------------------------------------
//called when the player is starting a new level for Normal game model
//	bSecret if came from a secret level
int StartNewLevel (int nLevel, bool bNewGame)
{
	gameStates.app.xThisLevelTime = 0;

gameData.reactor.bDestroyed = 0;
MakeModFolders (hogFileManager.MissionName (), nLevel);
if (nLevel < 0)
	return PrepareSecretLevel (nLevel, false);
MaybeSetFirstSecretVisit (nLevel);
if (!gameStates.app.bAutoRunMission)
	ShowLevelIntro (nLevel);
return PrepareLevel (nLevel, 1, false, false, bNewGame);
}

//------------------------------------------------------------------------------

typedef struct tSpawnMap {
	int	i;
	fix	xDist;
	} tSpawnMap;

void SortSpawnMap (tSpawnMap *spawnMap, int left, int right)
{
	int	l = left,
			r = right;
	fix	m = spawnMap [(l + r) / 2].xDist;

do {
	while (spawnMap [l].xDist > m)
		l++;
	while (spawnMap [r].xDist < m)
		r--;
	if (l <= r) {
		if (l < r) {
			tSpawnMap h = spawnMap [l];
			spawnMap [l] = spawnMap [r];
			spawnMap [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	SortSpawnMap (spawnMap, l, right);
if (left < r)
	SortSpawnMap (spawnMap, left, r);
}

//------------------------------------------------------------------------------

int GetRandomPlayerPosition (void)
{
	CObject		*objP;
	tSpawnMap	spawnMap [MAX_PLAYERS];
	int			nSpawnPos = 0;
	int			nSpawnSegs = 0;
	int			i, j, bRandom;
	fix			xDist;

// find the smallest distance between each spawn point and any player in the mine
d_srand (gameStates.app.nRandSeed = short (clock ()));
for (int h = 0; h < 100; h++)
for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++) {
	spawnMap [i].i = i;
	spawnMap [i].xDist = 0x7fffffff;
	for (j = 0; j < gameData.multiplayer.nPlayers; j++) {
		if (j != N_LOCALPLAYER) {
			objP = OBJECTS + gameData.multiplayer.players [j].nObject;
			if ((objP->info.nType == OBJ_PLAYER)) {
				xDist = simpleRouter [0].PathLength (objP->info.position.vPos, objP->info.nSegment,
																 gameData.multiplayer.playerInit [i].position.vPos, gameData.multiplayer.playerInit [i].nSegment,
																 10, WID_PASSABLE_FLAG, -1);	//	Used to be 5, search up to 10 segments
				if (xDist < 0)
					continue;
				if (spawnMap [i].xDist > xDist)
					spawnMap [i].xDist = xDist;
				}
			}
		}
	}
nSpawnSegs = gameData.multiplayer.nPlayerPositions;
SortSpawnMap (spawnMap, 0, nSpawnSegs - 1);
bRandom = (spawnMap [0].xDist >= SPAWN_MIN_DIST);

j = 0;
for (;;) {
	i = bRandom ? RandShort () % nSpawnSegs : j++;
	nSpawnPos = spawnMap [i].i;
	if (IsTeamGame) {
		switch (gameData.multiplayer.playerInit [nSpawnPos].nSegType) {
			case SEGMENT_FUNC_GOAL_RED:
			case SEGMENT_FUNC_TEAM_RED:
				if (GetTeam (N_LOCALPLAYER) != TEAM_RED)
					continue;
				break;
			case SEGMENT_FUNC_GOAL_BLUE:
			case SEGMENT_FUNC_TEAM_BLUE:
				if (GetTeam (N_LOCALPLAYER) != TEAM_BLUE)
					continue;
				break;
			default:
				break;
			}
		}
	if (!bRandom || (spawnMap [i].xDist > SPAWN_MIN_DIST))
		break;
	if (i < --nSpawnSegs)
		memcpy (spawnMap + i, spawnMap + i + 1, nSpawnSegs - i);
	}
return nSpawnPos;
}

//------------------------------------------------------------------------------
//initialize the player CObject position & orientation (at start of game, or new ship)
void InitPlayerPosition (int bRandom)
{
	int nSpawnPos = 0;

if (!(IsMultiGame || IsCoopGame)) // If not deathmatch
	nSpawnPos = N_LOCALPLAYER;
else if (bRandom == 1) {
	nSpawnPos = GetRandomPlayerPosition ();
	}
else {
	goto done; // If deathmatch and not Random, positions were already determined by sync packet
	}
Assert (nSpawnPos >= 0);
Assert (nSpawnPos < gameData.multiplayer.nPlayerPositions);

GetPlayerSpawn (nSpawnPos, gameData.objs.consoleP);

done:

ResetPlayerObject ();
controls.ResetCruise ();
}

//------------------------------------------------------------------------------

fix RobotDefaultShield (CObject *objP)
{
	tRobotInfo*	botInfoP;
	int			objId, i;
	fix			shield;

if (objP->info.nType != OBJ_ROBOT)
	return (objP->info.nType == OBJ_REACTOR) ? ReactorStrength () : 0;
objId = objP->info.nId;
//Assert (objId < gameData.bots.nTypes [0]);
i = gameStates.app.bD1Mission && (objId < gameData.bots.nTypes [1]);
botInfoP = gameData.bots.info [i] + objId;
//	Boost shield for Thief and Buddy based on level.
shield = botInfoP->strength;
if (botInfoP->thief || botInfoP->companion) {
	shield = (shield * (abs (missionManager.nCurrentLevel) + 7)) / 8;
	if (botInfoP->companion) {
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
else if (botInfoP->bossFlag) {	//	MK, 01/16/95, make boss shield lower on lower diff levels.
	shield = shield / (NDL + 3) * (gameStates.app.nDifficultyLevel + 4);
//	Additional wimpification of bosses at Trainee
	if (gameStates.app.nDifficultyLevel == 0)
		shield /= 2;
	}
return shield;
}

//------------------------------------------------------------------------------
//	Initialize default parameters for one robot, copying from gameData.bots.infoP to *objP.
//	What about setting size!?  Where does that come from?
void CopyDefaultsToRobot (CObject *objP)
{
objP->SetShield (RobotDefaultShield (objP));
}

//------------------------------------------------------------------------------
//	Copy all values from the robot info structure to all instances of robots.
//	This allows us to change bitmaps.tbl and have these changes manifested in existing robots.
//	This function should be called at level load time.
void CopyDefaultsToRobotsAll (void)
{
	//int		i;
	CObject	*objP;

FORALL_ROBOT_OBJS (objP, i)
	CopyDefaultsToRobot (objP);

}

//------------------------------------------------------------------------------

void DoPlayerDead (void)
{
	int bSecret = (missionManager.nCurrentLevel < 0);

gameStates.app.bGameRunning = 0;
StopTriggeredSounds ();
audio.PauseAll ();		//kill any continuing sounds (eg. forcefield hum)
DeadPlayerEnd ();		//terminate death sequence (if playing)
paletteManager.ResetEffect ();
if (IsCoopGame && gameStates.app.bHaveExtraGameInfo [1])
	LOCALPLAYER.score =
	(LOCALPLAYER.score * (100 - nCoopPenalties [(int) extraGameInfo [1].nCoopPenalty])) / 100;
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
if (gameData.reactor.bDestroyed) {
	//clear out stuff so no bonus
	LOCALPLAYER.hostages.nOnBoard = 0;
	LOCALPLAYER.SetEnergy (0);
	LOCALPLAYER.SetShield (0);
	CONNECT (N_LOCALPLAYER, CONNECT_DIED_IN_MINE);
	DiedInMineMessage (); // Give them some indication of what happened
	}
// if player dead, always leave D2 secret level, but only leave D1 secret level if reactor destroyed
if (bSecret && (!gameStates.app.bD1Mission || gameData.reactor.bDestroyed)) {
	ExitSecretLevel ();
	ResetShipData ();
	gameStates.render.cockpit.nLastDrawn [0] =
	gameStates.render.cockpit.nLastDrawn [1] = -1;
	}
else {
	if (gameData.reactor.bDestroyed) {
		FindNextLevel ();
		AdvanceLevel (0, 0);
		ResetShipData ();
		}
	else if (!gameStates.entropy.bExitSequence) {
		ResetShipData ();
		StartLevel (0x7fffffff, 1);
		}
	}
audio.ResumeAll ();
}

//------------------------------------------------------------------------------
//eof
