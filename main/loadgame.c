/* $Id: gameseq.c,v 1.33 2003/11/26 12:26:30 btb Exp $ */
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

#ifdef RCS
char gameseq_rcsid [] = "$Id: gameseq.c,v 1.33 2003/11/26 12:26:30 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <time.h>

#include "ogl_defs.h"
#include "ogl_lib.h"

#include "console.h"
#include "inferno.h"
#include "game.h"
#include "player.h"
#include "key.h"
#include "object.h"
#include "objrender.h"
#include "lightning.h"
#include "physics.h"
#include "error.h"
#include "joy.h"
#include "mono.h"
#include "iff.h"
#include "pcx.h"
#include "timer.h"
#include "cameras.h"
#include "render.h"
#include "laser.h"
#include "screens.h"
#include "textures.h"
#include "slew.h"
#include "gauges.h"
#include "texmap.h"
#include "3d.h"
#include "effects.h"
#include "menu.h"
#include "gameseg.h"
#include "wall.h"
#include "ai.h"
#include "fuelcen.h"
#include "switch.h"
#include "digi.h"
#include "gamesave.h"
#include "scores.h"
#include "ibitblt.h"
#include "u_mem.h"
#include "palette.h"
#include "morph.h"
#include "light.h"
#include "newdemo.h"
#include "briefings.h"
#include "collide.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "loadgame.h"
#include "gamefont.h"
#include "newmenu.h"
#include "endlevel.h"
#include "interp.h"
#include "multi.h"
#include "network.h"
#include "netmisc.h"
#include "modem.h"
#include "playsave.h"
#include "ctype.h"
#include "fireball.h"
#include "kconfig.h"
#include "config.h"
#include "robot.h"
#include "automap.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "text.h"
#include "cfile.h"
#include "piggy.h"
#include "texmerge.h"
#include "paging.h"
#include "mission.h"
#include "state.h"
#include "songs.h"
#include "gamepal.h"
#include "movie.h"
#include "controls.h"
#include "credits.h"
#include "gamemine.h"
#include "lightmap.h"
#include "polyobj.h"
#include "movie.h"
#include "particles.h"
#include "interp.h"
#include "sphere.h"

#if defined (TACTILE)
 #include "tactile.h"
#endif

#ifdef EDITOR
#include "editor/editor.h"
#endif

#include "strutil.h"
#include "rle.h"
#include "input.h"

//------------------------------------------------------------------------------

void ShowLevelIntro (int nLevel);
int StartNewLevelSecret (int nLevel, int bPageInTextures);
void InitPlayerPosition (int bRandom);
void LoadStars (bkg *bg, int bRedraw);
void ReturningToLevelMessage (void);
void AdvancingToLevelMessage (void);
void DoEndGame (void);
void AdvanceLevel (int bSecret, int bFromSecret);
void FilterObjectsFromLevel ();

// From allender -- you'll find these defines in state.c and cntrlcen.c
// since I couldn't think of a good place to put them and i wanted to
// fix this stuff fast!  Sorry about that...

#define SECRETB_FILENAME	"secret.sgb"
#define SECRETC_FILENAME	"secret.sgc"

//gameData.missions.nCurrentLevel starts at 1 for the first level
//-1,-2,-3 are secret levels
//0 means not a real level loaded
// Global variables telling what sort of game we have

void GrRemapMonoFonts ();
void SetFunctionMode (int);
void InitHoardData ();

extern fix ThisLevelTime;

// Extern from game.c to fix a bug in the cockpit!

extern int nLastLevelPathCreated;
extern int nTimeLastMoved;

//	HUDClearMessages external, declared in gauges.h
#ifndef _GAUGES_H
extern void HUDClearMessages (); // From hud.c
#endif

//	Extra prototypes declared for the sake of LINT
void InitPlayerStatsNewShip (void);
void CopyDefaultsToRobotsAll (void);

extern int nDescentCriticalError;

extern int nLastMsgYCrd;

//--------------------------------------------------------------------

void VerifyConsoleObject ()
{
	Assert (gameData.multiplayer.nLocalPlayer > -1);
	Assert (LOCALPLAYER.nObject > -1);
	gameData.objs.console = gameData.objs.objects + LOCALPLAYER.nObject;
	Assert (gameData.objs.console->nType == OBJ_PLAYER);
	Assert (gameData.objs.console->id==gameData.multiplayer.nLocalPlayer);
}

//------------------------------------------------------------------------------

int count_number_ofRobots ()
{
	int robotCount;
	int i;

	robotCount = 0;
	for (i=0;i<=gameData.objs.nLastObject;i++) {
		if (gameData.objs.objects [i].nType == OBJ_ROBOT)
			robotCount++;
	}

	return robotCount;
}

//------------------------------------------------------------------------------

int count_number_of_hostages ()
{
	int count;
	int i;

	count = 0;
	for (i=0;i<=gameData.objs.nLastObject;i++) {
		if (gameData.objs.objects [i].nType == OBJ_HOSTAGE)
			count++;
	}

	return count;
}

//------------------------------------------------------------------------------
//added 10/12/95: delete buddy bot if coop game.  Probably doesn't really belong here. -MT
void GameSeqInitNetworkPlayers ()
{
	int		i, j, t,
				segNum, segType, 
				playerObjs [MAX_PLAYERS], startSegs [MAX_PLAYERS],
				nPlayers;
	tObject	*objP;

	// Initialize network tPlayer start locations and tObject numbers

memset (gameStates.multi.bPlayerIsTyping, 0, sizeof (gameStates.multi.bPlayerIsTyping));
//VerifyConsoleObject ();
nPlayers = 0;
j = 0;
for (i = 0, objP = gameData.objs.objects; i <= gameData.objs.nLastObject; i++, objP++) {
	t = objP->nType;
	if ((t == OBJ_PLAYER) || (t == OBJ_GHOST) || (t == OBJ_COOP)) {
		if (IsCoopGame ? (j && (t != OBJ_COOP)) : (t == OBJ_COOP))
			ReleaseObject ((short) i);
		else {
			playerObjs [nPlayers] = i;
			startSegs [nPlayers] = gameData.objs.objects [i].nSegment;
			nPlayers++;
			}
		j++;
		}
	else if (t == OBJ_ROBOT) {
		if (ROBOTINFO (objP->id).companion && (gameData.app.nGameMode & GM_MULTI))
			ReleaseObject ((short) i);		//kill the buddy in netgames
		}
	}

// the following code takes care of team players being assigned the proper start locations
// in enhanced CTF
for (i = 0; i < nPlayers; i++) {
// find a tPlayer tObject that resides in a tSegment of proper nType for the current
// tPlayer start info 
	for (j = 0; j < nPlayers; j++) {
		segNum = startSegs [j];
		if (segNum < 0)
			continue;
		segType = (gameData.app.nGameMode & GM_MULTI_COOP) ? gameData.segs.segment2s [segNum].special : SEGMENT_IS_NOTHING;
#if 0		
		switch (segType) {
			case SEGMENT_IS_GOAL_RED:
			case SEGMENT_IS_TEAM_RED:
				if (i < nPlayers / 2) // (GetTeam (i) != TEAM_RED)
					continue;
				gameData.segs.segment2s [segNum].special = SEGMENT_IS_NOTHING;
				break;
			case SEGMENT_IS_GOAL_BLUE:
			case SEGMENT_IS_TEAM_BLUE:
				if (i >= nPlayers / 2) //GetTeam (i) != TEAM_BLUE)
					continue;
				gameData.segs.segment2s [segNum].special = SEGMENT_IS_NOTHING;
				break;
			default:
				break;
			}
#endif		
		objP = gameData.objs.objects + playerObjs [j];
		objP->nType = OBJ_PLAYER;
		gameData.multiplayer.playerInit [i].position = objP->position;
		gameData.multiplayer.playerInit [i].nSegment = objP->nSegment;
		gameData.multiplayer.playerInit [i].nSegType = segType;
		gameData.multiplayer.players [i].nObject = playerObjs [j];
		objP->id = i;
		startSegs [j] = -1;
		break;
		}
	}
gameData.objs.viewer = gameData.objs.console = gameData.objs.objects; // + LOCALPLAYER.nObject;
gameData.multiplayer.nPlayerPositions = nPlayers;

#ifdef _DEBUG
if (( (gameData.app.nGameMode & GM_MULTI_COOP) && (gameData.multiplayer.nPlayerPositions != 4)) ||
	  (!(gameData.app.nGameMode & GM_MULTI_COOP) && (gameData.multiplayer.nPlayerPositions != 8)))
{
#if TRACE	
	//con_printf (CON_VERBOSE, "--NOT ENOUGH MULTIPLAYER POSITIONS IN THIS MINE!--\n");
#endif
	//Int3 (); // Not enough positions!!
}
#endif
if (IS_D2_OEM && (gameData.app.nGameMode & GM_MULTI) && gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission && gameData.missions.nCurrentLevel==8) {
	for (i = 0; i < nPlayers; i++)
		if (gameData.multiplayer.players [i].connected && !(netPlayers.players [i].version_minor & 0xF0)) {
			ExecMessageBox ("Warning!",NULL,1,TXT_OK,"This special version of Descent II\nwill disconnect after this level.\nPlease purchase the full version\nto experience all the levels!");
			return;
			}
	}

if (IS_MAC_SHARE && (gameData.app.nGameMode & GM_MULTI) && gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission && gameData.missions.nCurrentLevel == 4) {
	for (i = 0; i < nPlayers; i++)
		if (gameData.multiplayer.players [i].connected && !(netPlayers.players [i].version_minor & 0xF0)) {
			ExecMessageBox ("Warning!",NULL , 1 ,TXT_OK, "This shareware version of Descent II\nwill disconnect after this level.\nPlease purchase the full version\nto experience all the levels!");
			return;
			}
	}
}

//------------------------------------------------------------------------------

void GameSeqRemoveUnusedPlayers ()
{
	int i;

	// 'Remove' the unused players

	if (gameData.app.nGameMode & GM_MULTI)
	{
		for (i=0; i < gameData.multiplayer.nPlayerPositions; i++)
		{
			if ((!gameData.multiplayer.players [i].connected) || (i >= gameData.multiplayer.nPlayers))
			{
			MultiMakePlayerGhost (i);
			}
		}
	}
	else
	{		// Note link to above if!!!
		for (i=1; i < gameData.multiplayer.nPlayerPositions; i++)
		{
			ReleaseObject ((short) gameData.multiplayer.players [i].nObject);
		}
	}
}

fix xStartingShields=INITIAL_SHIELDS;

// Setup tPlayer for new game
void InitPlayerStatsGame ()
{
	LOCALPLAYER.score = 0;
	LOCALPLAYER.last_score = 0;
	LOCALPLAYER.lives = INITIAL_LIVES;
	LOCALPLAYER.level = 1;

	LOCALPLAYER.timeLevel = 0;
	LOCALPLAYER.timeTotal = 0;
	LOCALPLAYER.hoursLevel = 0;
	LOCALPLAYER.hoursTotal = 0;

	LOCALPLAYER.energy = INITIAL_ENERGY;
	LOCALPLAYER.shields = xStartingShields;
	LOCALPLAYER.nKillerObj = -1;

	LOCALPLAYER.netKilledTotal = 0;
	LOCALPLAYER.netKillsTotal = 0;

	LOCALPLAYER.numKillsLevel = 0;
	LOCALPLAYER.numKillsTotal = 0;
	LOCALPLAYER.numRobotsLevel = 0;
	LOCALPLAYER.numRobotsTotal = 0;
	LOCALPLAYER.nKillGoalCount = 0;

	LOCALPLAYER.hostages_rescuedTotal = 0;
	LOCALPLAYER.hostagesLevel = 0;
	LOCALPLAYER.hostagesTotal = 0;

	LOCALPLAYER.laserLevel = 0;
	LOCALPLAYER.flags = 0;

	InitPlayerStatsNewShip ();

	gameStates.app.bFirstSecretVisit = 1;
}

//------------------------------------------------------------------------------

#define TEAMKEY(_p)	((GetTeam (_p) == TEAM_RED) ? KEY_RED : KEY_BLUE)

void InitAmmoAndEnergy (void)
{
	if (LOCALPLAYER.energy < INITIAL_ENERGY)
		LOCALPLAYER.energy = INITIAL_ENERGY;
	if (LOCALPLAYER.shields < xStartingShields)
		LOCALPLAYER.shields = xStartingShields;

//	for (i=0; i<MAX_PRIMARY_WEAPONS; i++)
//		if (LOCALPLAYER.primaryAmmo [i] < DefaultPrimaryAmmoLevel [i])
//			LOCALPLAYER.primaryAmmo [i] = DefaultPrimaryAmmoLevel [i];

//	for (i=0; i<MAX_SECONDARY_WEAPONS; i++)
//		if (LOCALPLAYER.secondaryAmmo [i] < DefaultSecondaryAmmoLevel [i])
//			LOCALPLAYER.secondaryAmmo [i] = DefaultSecondaryAmmoLevel [i];
	if (LOCALPLAYER.secondaryAmmo [0] < 2 + NDL - gameStates.app.nDifficultyLevel)
		LOCALPLAYER.secondaryAmmo [0] = 2 + NDL - gameStates.app.nDifficultyLevel;
}

extern	ubyte	bLastAfterburnerState;

//------------------------------------------------------------------------------

// Setup tPlayer for new level (After completion of previous level)
void InitPlayerStatsLevel (int bSecret)
{
	// int	i;
LOCALPLAYER.last_score = LOCALPLAYER.score;
LOCALPLAYER.level = gameData.missions.nCurrentLevel;
if (!networkData.bRejoined) {
	LOCALPLAYER.timeLevel = 0;
	LOCALPLAYER.hoursLevel = 0;
	}
LOCALPLAYER.nKillerObj = -1;
LOCALPLAYER.numKillsLevel = 0;
LOCALPLAYER.numRobotsLevel = count_number_ofRobots ();
LOCALPLAYER.numRobotsTotal += LOCALPLAYER.numRobotsLevel;
LOCALPLAYER.hostagesLevel = count_number_of_hostages ();
LOCALPLAYER.hostagesTotal += LOCALPLAYER.hostagesLevel;
LOCALPLAYER.hostages_on_board = 0;
if (!bSecret) {
	InitAmmoAndEnergy ();
	LOCALPLAYER.flags &=  
		~(PLAYER_FLAGS_INVULNERABLE | PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_MAP_ALL | KEY_BLUE | KEY_RED | KEY_GOLD);
	LOCALPLAYER.cloakTime = 0;
	LOCALPLAYER.invulnerableTime = 0;
	if (IsMultiGame && !IsCoopGame) {
		if ((gameData.app.nGameMode & GM_TEAM) && gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bTeamDoors)
			LOCALPLAYER.flags |= KEY_GOLD | TEAMKEY (gameData.multiplayer.nLocalPlayer);
		else
			LOCALPLAYER.flags |= (KEY_BLUE | KEY_RED | KEY_GOLD);
		}
	}
else if (gameStates.app.bD1Mission)
	InitAmmoAndEnergy ();
gameStates.app.bPlayerIsDead = 0; // Added by RH
LOCALPLAYER.homingObjectDist = -F1_0; // Added by RH
gameData.laser.xLastFiredTime = 
gameData.laser.xNextFireTime = 
gameData.missiles.xLastFiredTime = 
gameData.missiles.xNextFireTime = gameData.time.xGame; // added by RH, solved demo playback bug
Controls [0].afterburnerState = 0;
bLastAfterburnerState = 0;
DigiKillSoundLinkedToObject (LOCALPLAYER.nObject);
InitGauges ();
#ifdef TACTILE
if (TactileStick)
	tactile_set_button_jolt ();
#endif
gameData.objs.missileViewer = NULL;
}

//------------------------------------------------------------------------------

extern	void InitAIForShip (void);

// Setup tPlayer for a brand-new ship
void InitPlayerStatsNewShip ()
{
	int	i;

if (gameData.demo.nState == ND_STATE_RECORDING) {
	NDRecordLaserLevel (LOCALPLAYER.laserLevel, 0);
	NDRecordPlayerWeapon (0, 0);
	NDRecordPlayerWeapon (1, 0);
	}

LOCALPLAYER.energy = INITIAL_ENERGY;
LOCALPLAYER.shields = xStartingShields;
LOCALPLAYER.laserLevel = 0;
LOCALPLAYER.nKillerObj = -1;
LOCALPLAYER.hostages_on_board = 0;

gameData.physics.xAfterburnerCharge = 0;

for (i = 0; i < MAX_PRIMARY_WEAPONS; i++) {
	LOCALPLAYER.primaryAmmo [i] = 0;
	bLastPrimaryWasSuper [i] = 0;
	}
for (i = 1; i < MAX_SECONDARY_WEAPONS; i++) {
	LOCALPLAYER.secondaryAmmo [i] = 0;
	bLastSecondaryWasSuper [i] = 0;
	}
LOCALPLAYER.secondaryAmmo [0] = 2 + NDL - gameStates.app.nDifficultyLevel;
LOCALPLAYER.primaryWeaponFlags = HAS_LASER_FLAG;
LOCALPLAYER.secondaryWeaponFlags = HAS_CONCUSSION_FLAG;
gameData.weapons.nOverridden = 0;
gameData.weapons.nPrimary = 0;
gameData.weapons.nSecondary = 0;
LOCALPLAYER.flags &= ~ 
	(PLAYER_FLAGS_QUAD_LASERS |
	 PLAYER_FLAGS_AFTERBURNER |
	 PLAYER_FLAGS_CLOAKED |
	 PLAYER_FLAGS_INVULNERABLE |
	 PLAYER_FLAGS_MAP_ALL |
	 PLAYER_FLAGS_CONVERTER |
	 PLAYER_FLAGS_AMMO_RACK |
	 PLAYER_FLAGS_HEADLIGHT |
	 PLAYER_FLAGS_HEADLIGHT_ON |
	 PLAYER_FLAGS_FLAG);
if (IsMultiGame && gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bDarkness)
	LOCALPLAYER.flags |= PLAYER_FLAGS_HEADLIGHT;
LOCALPLAYER.cloakTime = 0;
LOCALPLAYER.invulnerableTime = 0;
gameStates.app.bPlayerIsDead = 0;		//tPlayer no longer dead
LOCALPLAYER.homingObjectDist = -F1_0; // Added by RH
Controls [0].afterburnerState = 0;
bLastAfterburnerState = 0;
DigiKillSoundLinkedToObject (LOCALPLAYER.nObject);
gameData.objs.missileViewer=NULL;		///reset missile camera if out there
#ifdef TACTILE
	if (TactileStick)
	{
	tactile_set_button_jolt ();
	}
#endif
InitAIForShip ();
}

//------------------------------------------------------------------------------

extern void InitStuckObjects (void);

#ifdef EDITOR

extern int gameData.segs.bHaveSlideSegs;

//reset stuff so game is semi-normal when playing from editor
void editor_reset_stuff_onLevel ()
{
	GameSeqInitNetworkPlayers ();
	InitPlayerStatsLevel (0);
	gameData.objs.viewer = gameData.objs.console;
	gameData.objs.console = gameData.objs.viewer = gameData.objs.objects + LOCALPLAYER.nObject;
	gameData.objs.console->id=gameData.multiplayer.nLocalPlayer;
	gameData.objs.console->controlType = CT_FLYING;
	gameData.objs.console->movementType = MT_PHYSICS;
	gameStates.app.bGameSuspended = 0;
	VerifyConsoleObject ();
	gameData.reactor.bDestroyed = 0;
	if (gameData.demo.nState != ND_STATE_PLAYBACK)
		GameSeqRemoveUnusedPlayers ();
	InitCockpit ();
	InitRobotsForLevel ();
	InitAIObjects ();
	MorphInit ();
	InitAllMatCens ();
	InitPlayerStatsNewShip ();
	InitReactorForLevel (0);
	AutomapClearVisited ();
	InitStuckObjects ();
	InitThiefForLevel ();

	gameData.segs.bHaveSlideSegs = 0;
}
#endif

//------------------------------------------------------------------------------

//do whatever needs to be done when a tPlayer dies in multiplayer
void DoGameOver ()
{
//	ExecMessageBox (TXT_GAME_OVER, 1, TXT_OK, "");
if (gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission)
	MaybeAddPlayerScore (0);
SetFunctionMode (FMODE_MENU);
gameData.app.nGameMode = GM_GAME_OVER;
longjmp (gameExitPoint, 0);		// Exit out of game loop

}

//------------------------------------------------------------------------------

extern void do_save_game_menu ();

//update various information about the tPlayer
void UpdatePlayerStats ()
{
LOCALPLAYER.timeLevel += gameData.time.xFrame;	//the never-ending march of time...
if (LOCALPLAYER.timeLevel > i2f (3600))	{
	LOCALPLAYER.timeLevel -= i2f (3600);
	LOCALPLAYER.hoursLevel++;
	}
LOCALPLAYER.timeTotal += gameData.time.xFrame;	//the never-ending march of time...
if (LOCALPLAYER.timeTotal > i2f (3600))	{
	LOCALPLAYER.timeTotal -= i2f (3600);
	LOCALPLAYER.hoursTotal++;
	}
}

//------------------------------------------------------------------------------

//go through this level and start any tEffectClip sounds
void SetSoundSources (void)
{
	short			nSegment, nSide, nConnSeg, nConnSide, nSound;
	tSegment		*segP, *connSegP;
	tObject		*objP;
	vmsVector	v;
	int			i, nOvlTex, nEffect;

gameOpts->sound.bD1Sound = gameStates.app.bD1Mission && gameStates.app.bHaveD1Data && gameOpts->sound.bUseD1Sounds;
DigiInitSounds ();		//clear old sounds
gameStates.sound.bDontStartObjects = 1;
for (segP = gameData.segs.segments, nSegment = 0; nSegment <= gameData.segs.nLastSegment; segP++, nSegment++)
	for (nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
		if (!(WALL_IS_DOORWAY (segP,nSide, NULL) & WID_RENDER_FLAG))
			continue;
		nEffect = (nOvlTex = segP->sides [nSide].nOvlTex) ? gameData.pig.tex.pTMapInfo [nOvlTex].eclip_num : -1;
		if (nEffect < 0)
			nEffect = gameData.pig.tex.pTMapInfo [segP->sides [nSide].nBaseTex].eclip_num;
		if (nEffect < 0)
			continue;
		if ((nSound = gameData.eff.pEffects [nEffect].nSound) == -1)
			continue;
		nConnSeg = segP->children [nSide];

		//check for sound on other tSide of tWall.  Don't add on
		//both walls if sound travels through tWall.  If sound
		//does travel through tWall, add sound for lower-numbered
		//tSegment.

		if (IS_CHILD (nConnSeg) && (nConnSeg < nSegment) &&
			 (WALL_IS_DOORWAY (segP, nSide, NULL) & (WID_FLY_FLAG | WID_RENDPAST_FLAG))) {
			connSegP = gameData.segs.segments + segP->children [nSide];
			nConnSide = FindConnectedSide (segP, connSegP);
			if (connSegP->sides [nConnSide].nOvlTex == segP->sides [nSide].nOvlTex)
				continue;		//skip this one
			}
		COMPUTE_SIDE_CENTER (&v,segP,nSide);
		DigiLinkSoundToPos (nSound, nSegment, nSide, &v, 1, F1_0 / 2);
		}

if (0 <= (nSound = DigiGetSoundByName ("explode2"))) {
	for (i = 0, objP = OBJECTS; i <= gameData.objs.nLastObject; i++, objP++) 
		if (objP->nType == OBJ_EXPLOSION) {
			objP->rType.vClipInfo.nClipIndex = objP->id;
			DigiSetObjectSound (i, nSound, NULL);
			}
	}
gameOpts->sound.bD1Sound = 0;
gameStates.sound.bDontStartObjects = 0;
}

//------------------------------------------------------------------------------

//fix flashDist=i2f (1);
fix flashDist=fl2f (.9);
//create flash for tPlayer appearance
void CreatePlayerAppearanceEffect (tObject *playerObjP)
{
	vmsVector	pos;
	tObject		*effectObjP;
#ifdef _DEBUG
	int			nObject = OBJ_IDX (playerObjP);

	if ((nObject < 0) || (nObject > gameData.objs.nLastObject))
		Int3 (); // See Rob, trying to track down weird network bug
#endif
if (playerObjP == gameData.objs.viewer)
	VmVecScaleAdd (&pos, &playerObjP->position.vPos, &playerObjP->position.mOrient.fVec, FixMul (playerObjP->size,flashDist));
else
	pos = playerObjP->position.vPos;
effectObjP = ObjectCreateExplosion (playerObjP->nSegment, &pos, playerObjP->size, VCLIP_PLAYER_APPEARANCE);
if (effectObjP) {
	effectObjP->position.mOrient = playerObjP->position.mOrient;
	if (gameData.eff.vClips [0][VCLIP_PLAYER_APPEARANCE].nSound > -1)
		DigiLinkSoundToObject (gameData.eff.vClips [0][VCLIP_PLAYER_APPEARANCE].nSound, OBJ_IDX (effectObjP), 0, F1_0);
	}
}

//------------------------------------------------------------------------------

//
// New Game sequencing functions
//

//pairs of chars describing ranges
char playername_allowed_chars [] = "azAZ09__--";

int MakeNewPlayerFile (int allow_abort)
{
	int x;
	char filename [FILENAME_LEN];
	tMenuItem m;
	char text [CALLSIGN_LEN+1]="";
#if 0
	FILE *fp;
#endif

strncpy (text, LOCALPLAYER.callsign,CALLSIGN_LEN);

try_again:

memset (&m, 0, sizeof (m));
m.nType=NM_TYPE_INPUT; 
m.text_len = 8; 
m.text = text;

nmAllowedChars = playername_allowed_chars;
x = ExecMenu (NULL, TXT_ENTER_PILOT_NAME, 1, &m, NULL, NULL);
nmAllowedChars = NULL;
if (x < 0) {
	if (allow_abort) return 0;
	goto try_again;
	}
if (text [0]==0)	//null string
	goto try_again;
sprintf (filename, "%s.plr", text);

if (CFExist (filename,gameFolders.szProfDir,0)) {
	ExecMessageBox (NULL, NULL, 1, TXT_OK, "%s '%s' %s", TXT_PLAYER, text, TXT_ALREADY_EXISTS);
	goto try_again;
	}
if (!NewPlayerConfig ())
	goto try_again;			// They hit Esc during New tPlayer config
strncpy (LOCALPLAYER.callsign, text, CALLSIGN_LEN);
WritePlayerFile ();
return 1;
}

//------------------------------------------------------------------------------

//Inputs the tPlayer's name, without putting up the background screen
int SelectPlayer ()
{
	static int bStartup = 1;
	int 	i,j, bAutoPlr;
	char 	filename [FILENAME_LEN];
	char	filespec [FILENAME_LEN];
	int 	bAllowAbort = !bStartup;

if (LOCALPLAYER.callsign [0] == 0)	{
	//---------------------------------------------------------------------
	// Set default config options in case there is no config file
	// kcKeyboard, kc_joystick, kcMouse are statically defined.
	gameOpts->input.joystick.sensitivity [0] =
	gameOpts->input.joystick.sensitivity [1] =
	gameOpts->input.joystick.sensitivity [2] =
	gameOpts->input.joystick.sensitivity [3] = 8;
	gameOpts->input.mouse.sensitivity [0] =
	gameOpts->input.mouse.sensitivity [1] =
	gameOpts->input.mouse.sensitivity [2] = 8;
	gameConfig.nControlType = CONTROL_NONE;
	for (i = 0; i < CONTROL_MAX_TYPES; i++)
		for (j = 0; j < MAX_CONTROLS; j++)
			controlSettings.custom [i][j] = controlSettings.defaults [i][j];
	KCSetControls (0);
	//----------------------------------------------------------------

	// Read the last tPlayer's name from config file, not lastplr.txt
	strncpy (LOCALPLAYER.callsign, gameConfig.szLastPlayer, CALLSIGN_LEN);
	if (gameConfig.szLastPlayer [0] == 0)
		bAllowAbort = 0;
	}
if ((bAutoPlr = gameData.multiplayer.autoNG.bValid))
	strncpy (filename, gameData.multiplayer.autoNG.szPlayer, 8);
else if ((bAutoPlr = bStartup && (i = FindArg ("-player")) && *Args [++i]))
	strncpy (filename, Args [i], 8);
if (bAutoPlr) {
	char *psz;
	strlwr (filename);
	if (!(psz = strchr (filename, '.')))
		for (psz = filename; psz - filename < 8; psz++)
			if (!*psz || isspace (*psz))
				break;
		*psz = '\0';
	goto got_player;
	}

do_menu_again:

bStartup = 0;
sprintf (filespec, "%s%s*.plr", gameFolders.szProfDir, *gameFolders.szProfDir ? "/" : ""); 
if (!ExecMenuFileSelector (TXT_SELECT_PILOT, filespec, filename, bAllowAbort))	{
	if (bAllowAbort) {
		return 0;
		}
	goto do_menu_again; //return 0;		// They hit Esc in file selector
	}

got_player:

bStartup = 0;
if (filename [0] == '<')	{
	// They selected 'create new pilot'
	if (!MakeNewPlayerFile (1))
		//return 0;		// They hit Esc during enter name stage
		goto do_menu_again;
	}
else
	strncpy (LOCALPLAYER.callsign, filename, CALLSIGN_LEN);
if (ReadPlayerFile (0) != EZERO)
	goto do_menu_again;
KCSetControls (0);
SetDisplayMode (gameStates.video.nDefaultDisplayMode, 1);
WriteConfigFile ();		// Update lastplr
D2SetCaption ();
return 1;
}


int NetworkVerifyObjects (int nRemoteObjNum, int nLocalObjs);

//	------------------------------------------------------------------------------

void SetVertigoRobotFlags (void)
{
	tObject	*objP;
	int		i;

gameData.objs.nVertigoBotFlags = 0;
for (i = 0, objP = gameData.objs.objects; i <= gameData.objs.nLastObject; i++, objP++)
	if ((objP->nType == OBJ_ROBOT) && (objP->id >= 66) && !IS_BOSS (objP))
		gameData.objs.nVertigoBotFlags |= (1 << (objP->id - 64));
}

//------------------------------------------------------------------------------

char *LevelName (int nLevel)
{
return gameStates.app.bAutoRunMission ? szAutoMission : (nLevel < 0) ? 
		 gameData.missions.szSecretLevelNames [-nLevel-1] : 
		 gameData.missions.szLevelNames [nLevel-1];
}

//------------------------------------------------------------------------------

char *MakeLevelFilename (int nLevel, char *pszFilename, char *pszFileExt)
{
ChangeFilenameExtension (pszFilename, strlwr (LevelName (nLevel)), pszFileExt);
return pszFilename;
}

//------------------------------------------------------------------------------
//load a level off disk. level numbers start at 1.  Secret levels are -1,-2,-3

extern char szAutoMission [255];

int LoadLevel (int nLevel, int bPageInTextures, int bRestore)
{
	char		*pszLevelName;
	char		szHogName [FILENAME_LEN];
	tPlayer	save_player;
	int		nRooms, bRetry = 0, nLoadRes;

/*---*/LogErr ("Loading level...\n");
gameStates.app.bFreeCam = 0;
gameStates.app.bGameRunning = 0;
gameData.physics.side.nSegment = -1;
gameData.physics.side.nSide = -1;
gameStates.gameplay.bKillBossCheat = 0;
gameStates.render.nFlashScale = F1_0;
gameOpts->app.nScreenShotInterval = 0;	//better reset this every time a level is loaded
memset (gameData.stats.player, 0, sizeof (tPlayerStats));
memset (gameData.render.mine.bObjectRendered, 0xff, sizeof (gameData.render.mine.bObjectRendered));
memset (gameData.render.mine.bRenderSegment, 0xff, sizeof (gameData.render.mine.bRenderSegment));
memset (gameData.render.mine.bCalcVertexColor, 0, sizeof (gameData.render.mine.bCalcVertexColor));
#if 1
/*---*/LogErr ("   stopping music\n");
SongsStopAll ();
/*---*/LogErr ("   stopping sounds\n");
DigiStopAllChannels ();
/*---*/LogErr ("   unloading textures\n");
PiggyBitmapPageOutAll (0);
/*---*/LogErr ("   unloading custom sounds\n");
FreeSoundReplacements ();
/*---*/LogErr ("   unloading hardware lights\n");
RemoveDynLights ();
/*---*/LogErr ("   unloading cambot\n");
UnloadCamBot ();
/*---*/LogErr ("   unloading additional models\n");
BMFreeExtraModels ();
/*---*/LogErr ("   unloading additional model textures\n");
BMFreeExtraObjBitmaps ();
/*---*/LogErr ("   unloading additional model textures\n");
PiggyFreeHiresAnimations ();
/*---*/LogErr ("   freeing sound buffers\n");
DigiFreeSoundBufs ();
/*---*/LogErr ("   freeing auxiliary poly model data\n");
G3FreeAllPolyModelItems ();
/*---*/LogErr ("   restoring default robot settings\n");
RestoreDefaultRobots ();
if (gameData.bots.bReplacementsLoaded) {
	/*---*/LogErr ("   loading default robot settings\n");
	ReadHamFile ();		//load original data
	gameData.bots.bReplacementsLoaded = 0;
	}
if (gameData.missions.nEnhancedMission) {
	char t [FILENAME_LEN];

	sprintf (t,"%s.ham", gameStates.app.szCurrentMissionFile);
	/*---*/LogErr ("   reading additional robots\n");
	switch (BMReadExtraRobots (t, gameFolders.szMissionDirs [0], gameData.missions.nEnhancedMission)) {
		case -1:
			return 0;
		case 1:
			break;
		default:
			if (0 > BMReadExtraRobots ("d2x.ham", gameFolders.szMissionDir, gameData.missions.nEnhancedMission))
				return 0;
		}
	strncpy (t, gameStates.app.szCurrentMissionFile, 6);
	strcat (t, "-l.mvl");
	/*---*/LogErr ("   initializing additional robot movies\n");
	InitExtraRobotMovie (t);
	}
#endif
/*---*/LogErr ("   Destroying camera objects\n");
DestroyCameras ();
/*---*/LogErr ("   Destroying particle data\n");
DestroyAllSmoke ();
/*---*/LogErr ("   Destroying lightning data\n");
DestroyAllLightnings (1);
DestroyOmegaLightnings ();
/*---*/LogErr ("   Initializing smoke manager\n");
InitObjectSmoke ();
memset (gameData.pig.tex.bitmapColors, 0, sizeof (gameData.pig.tex.bitmapColors));
memset (gameData.models.thrusters, 0, sizeof (gameData.models.thrusters));
gameData.render.lights.flicker.nLights = 0;
save_player = LOCALPLAYER;
Assert (gameStates.app.bAutoRunMission || 
		  ((nLevel <= gameData.missions.nLastLevel) && 
		   (nLevel >= gameData.missions.nLastSecretLevel) && 
			(nLevel != 0)));
strlwr (pszLevelName = LevelName (nLevel));
/*---*/LogErr ("   loading level '%s'\n", pszLevelName);
#if 0
GrSetCurrentCanvas (NULL);
GrClearCanvas (BLACK_RGBA);		//so palette switching is less obvious
#endif
nLastMsgYCrd = -1;		//so we don't restore backgound under msg
/*---*/LogErr ("   loading palette\n");
GrPaletteStepLoad (NULL);
 //LoadPalette ("groupa.256", NULL, 0, 0, 1);		//don't change screen
if (!gameOpts->menus.nStyle)
	NMLoadBackground (NULL, NULL, 0);
ShowBoxedMessage (TXT_LOADING);
/*---*/LogErr ("   loading level data\n");
gameStates.app.bD1Mission = gameStates.app.bAutoRunMission ? (strstr (szAutoMission, "rdl") != NULL) :
									 (gameData.missions.list [gameData.missions.nCurrentMission].nDescentVersion == 1);
memset (gameData.segs.xSegments, 0xff, sizeof (*gameData.segs.xSegments) * MAX_SEGMENTS);
memset (gameData.objs.xCreationTime, 0, sizeof (*gameData.objs.xCreationTime) * MAX_OBJECTS);
/*---*/LogErr ("   loading texture brightness info\n");
SetDataVersion (-1);
LoadTextureBrightness (pszLevelName);
for (;;) {
	if (!(nLoadRes = LoadLevelSub (pszLevelName)))
		break;	//actually load the data from disk!
	nLoadRes = 1;
	if (bRetry)
		break;
	if (strstr (gameHogFiles.AltHogFiles.szName, ".hog"))
		break;
	sprintf (szHogName, "%s%s%s%s", 
				gameFolders.szMissionDir, *gameFolders.szMissionDir ? "/" : "", 
				gameFolders.szMsnSubDir, pszLevelName);
	if (!CFUseAltHogFile (szHogName))
		break;
	bRetry = 1;
	};
if (nLoadRes) {
	/*---*/LogErr ("Couldn't load '%s' (%d)\n", pszLevelName, nLoadRes);
	Warning (TXT_LOAD_ERROR, pszLevelName);
	return 0;
	}

gameData.missions.nCurrentLevel = nLevel;
gamePalette = LoadPalette (szCurrentLevelPalette, pszLevelName, 1, 1, 1);		//don't change screen
InitGaugeCanvases ();
ResetPogEffects ();
if (gameStates.app.bD1Mission) {
	/*---*/LogErr ("   loading Descent 1 textures\n");
	LoadD1BitmapReplacements ();
	if (bPageInTextures)
		PiggyLoadLevelData ();
	}
else {
	if (bPageInTextures)
		PiggyLoadLevelData ();
	LoadBitmapReplacements (pszLevelName);
	LoadSoundReplacements (pszLevelName);
	}
/*---*/LogErr ("   loading endlevel data\n");
LoadEndLevelData (nLevel);
/*---*/LogErr ("   loading cambot\n");
gameData.bots.nCamBotId = (LoadRobotReplacements ("cambot.hxm", 1, 0) > 0) ? gameData.bots.nTypes [0] - 1 : -1;
gameData.bots.nCamBotModel = gameData.models.nPolyModels - 1;
/*---*/LogErr ("   loading replacement robots\n");
if (0 > LoadRobotReplacements (pszLevelName, 0, 0))
	return 0;
/*---*/LogErr ("   initializing cambot\n");
InitCamBots (0);
networkData.nMySegsCheckSum = NetMiscCalcCheckSum (gameData.segs.segments, sizeof (tSegment) * gameData.segs.nSegments);
ResetNetworkObjects ();
ResetChildObjects ();
ResetFlightPath (&externalView, -1, -1);
ResetPlayerPaths ();
FixObjectSizes ();
/*---*/LogErr ("   counting entropy rooms\n");
nRooms = CountRooms ();
if (gameData.app.nGameMode & GM_ENTROPY) {
	if (!nRooms) {
		Warning (TXT_NO_ENTROPY);
		gameData.app.nGameMode &= ~GM_ENTROPY;
		gameData.app.nGameMode |= GM_TEAM;
		}
	}
else if ((gameData.app.nGameMode & (GM_CAPTURE | GM_HOARD)) || 
			((gameData.app.nGameMode & GM_MONSTERBALL) == GM_MONSTERBALL)) {
/*---*/LogErr ("   gathering CTF+ flag goals\n");
	if (GatherFlagGoals () != 3) {
		Warning (TXT_NO_CTF);
		gameData.app.nGameMode &= ~GM_CAPTURE;
		gameData.app.nGameMode |= GM_TEAM;
		}
	}
memset (gameData.render.lights.segDeltas, 0, sizeof (*gameData.render.lights.segDeltas) * MAX_SEGMENTS * 6);
/*---*/LogErr ("   initializing door animations\n");
InitDoorAnims ();
LOCALPLAYER = save_player;
gameData.hoard.nMonsterballSeg = -1;
/*---*/LogErr ("   initializing sound sources\n");
SetSoundSources ();
if (!IsMultiGame)
	InitEntropySettings (0);	//required for repair centers
PlayLevelSong (gameData.missions.nCurrentLevel, 1);
ClearBoxedMessage ();		//remove message before new palette loaded
GrPaletteStepLoad (NULL);		//actually load the palette
/*---*/LogErr ("   rebuilding OpenGL texture data\n");
/*---*/LogErr ("      rebuilding effects\n");
if (!bRestore)
	RebuildRenderContext (1);
ResetPingStats ();
gameStates.gameplay.nDirSteps = 0;
gameStates.gameplay.bMineMineCheat = 0;
gameStates.render.bAllVisited = 0;
gameStates.render.bViewDist = 1;
gameStates.render.bHaveSkyBox = -1;
gameStates.app.cheats.nUnlockLevel = 0;
gameStates.render.nFrameFlipFlop = 0;
gameStates.app.bUsingConverter = 0;
memset (gameData.render.color.vertices, 0, sizeof (*gameData.render.color.vertices) * MAX_VERTICES);
memset (gameData.render.color.segments, 0, sizeof (*gameData.render.color.segments) * MAX_SEGMENTS);
memset (gameData.objs.speedBoost, 0, sizeof (*gameData.objs.speedBoost) * MAX_SEGMENTS);
if (!gameStates.render.bHaveStencilBuffer)
	extraGameInfo [0].bShadows = 0;
D2SetCaption ();
if (!bRestore) {
	gameData.render.lights.bInitDynColoring = 1;
	gameStates.gameplay.slowmo [0].fSpeed =
	gameStates.gameplay.slowmo [1].fSpeed = 1;
	gameStates.gameplay.slowmo [0].nState =
	gameStates.gameplay.slowmo [1].nState = 0;
	ConvertObjects ();
	ComputeNearestLights (nLevel);
	ComputeStaticDynLighting ();
	SetEquipGenStates ();
	}
LoadExtraImages ();
CreateShieldSphere ();
SetupEffects ();
SetVertigoRobotFlags ();
SetDebrisCollisions ();
if (gameOpts->render.nPath)
	gameOpts->render.bDepthSort = 1;
return 1;
}

//------------------------------------------------------------------------------

//sets up gameData.multiplayer.nLocalPlayer & gameData.objs.console
void InitMultiPlayerObject ()
{
Assert ((gameData.multiplayer.nLocalPlayer >= 0) && (gameData.multiplayer.nLocalPlayer < MAX_PLAYERS));
if (gameData.multiplayer.nLocalPlayer != 0)	{
	gameData.multiplayer.players [0] = LOCALPLAYER;
	gameData.multiplayer.nLocalPlayer = 0;
	}
LOCALPLAYER.nObject = 0;
LOCALPLAYER.nInvuls =
LOCALPLAYER.nCloaks = 0;
gameData.objs.console = gameData.objs.objects + LOCALPLAYER.nObject;
gameData.objs.console->nType = OBJ_PLAYER;
gameData.objs.console->id = gameData.multiplayer.nLocalPlayer;
gameData.objs.console->controlType	= CT_FLYING;
gameData.objs.console->movementType = MT_PHYSICS;
gameStates.entropy.nTimeLastMoved = -1;
}

//------------------------------------------------------------------------------

extern void GameDisableCheats ();
extern void TurnCheatsOff ();
extern void InitSeismicDisturbances (void);

//starts a new game on the given level
int StartNewGame (int nStartLevel)
{
	int result;

gameData.app.nGameMode = GM_NORMAL;
SetFunctionMode (FMODE_GAME);
gameData.missions.nNextLevel = 0;
InitMultiPlayerObject ();				//make sure tPlayer's tObject set up
InitPlayerStatsGame ();		//clear all stats
gameData.multiplayer.nPlayers = 1;
networkData.bNewGame = 0;
if (nStartLevel < 0)
	result = StartNewLevelSecret (nStartLevel, 0);
else
	result = StartNewLevel (nStartLevel, 0);
if (result) {
	LOCALPLAYER.startingLevel = nStartLevel;		// Mark where they started
	GameDisableCheats ();
	InitSeismicDisturbances ();
	}
return result;
}

//@@//starts a resumed game loaded from disk
//@@void ResumeSavedGame (int nStartLevel)
//@@{
//@@	gameData.app.nGameMode = GM_NORMAL;
//@@	SetFunctionMode (FMODE_GAME);
//@@
//@@	gameData.multiplayer.nPlayers = 1;
//@@	networkData.bNewGame = 0;
//@@
//@@	InitPlayerObject ();				//make sure tPlayer's tObject set up
//@@
//@@	StartNewLevel (nStartLevel, 0);
//@@
//@@	GameDisableCheats ();
//@@}

//------------------------------------------------------------------------------

#ifndef _NETWORK_H
extern int NetworkEndLevelPoll2 (int nitems, tMenuItem * menus, int * key, int citem); // network.c
#endif

//	Does the bonus scoring.
//	Call with deadFlag = 1 if tPlayer died, but deserves some portion of bonus (only skill points), anyway.
void DoEndLevelScoreGlitz (int network)
{
	#define N_GLITZITEMS 11

	int			nLevelPoints, nSkillPoints, nEnergyPoints, nShieldPoints, nHostagePoints, nAllHostagePoints, nEndGamePoints;
	char			szAllHostages [64];
	char			szEndGame [64];
	char			szMenu [N_GLITZITEMS][30];
	tMenuItem	m [N_GLITZITEMS+1];
	int			i,c;
	char			szTitle [128];
	int			bIsLastLevel;
	int			nMineLevel = 0;

DigiStopAllChannels ();
SetScreenMode (SCREEN_MENU);		//go into menu mode
if (gameStates.app.bHaveExtraData)

#ifdef TACTILE
if (TactileStick)
	ClearForces ();
#endif

	//	Compute level tPlayer is on, deal with secret levels (negative numbers)
nMineLevel = LOCALPLAYER.level;
if (nMineLevel < 0)
	nMineLevel *= - (gameData.missions.nLastLevel/gameData.missions.nSecretLevels);
nLevelPoints = LOCALPLAYER.score-LOCALPLAYER.last_score;
if (!gameStates.app.cheats.bEnabled) {
	if (gameStates.app.nDifficultyLevel > 1) {
		nSkillPoints = nLevelPoints* (gameStates.app.nDifficultyLevel)/4;
		nSkillPoints -= nSkillPoints % 100;
		}
	else
		nSkillPoints = 0;
	nShieldPoints = f2i (LOCALPLAYER.shields) * 5 * nMineLevel;
	nEnergyPoints = f2i (LOCALPLAYER.energy) * 2 * nMineLevel;
	nHostagePoints = LOCALPLAYER.hostages_on_board * 500 * (gameStates.app.nDifficultyLevel+1);
	nShieldPoints -= nShieldPoints % 50;
	nEnergyPoints -= nEnergyPoints % 50;
	}
else {
	nSkillPoints = 0;
	nShieldPoints = 0;
	nEnergyPoints = 0;
	nHostagePoints = 0;
	}
szAllHostages [0] = 0;
szEndGame [0] = 0;
if (!gameStates.app.cheats.bEnabled && (LOCALPLAYER.hostages_on_board == LOCALPLAYER.hostagesLevel)) {
	nAllHostagePoints = LOCALPLAYER.hostages_on_board * 1000 * (gameStates.app.nDifficultyLevel+1);
	sprintf (szAllHostages, "%s%i\n", TXT_FULL_RESCUE_BONUS, nAllHostagePoints);
	}
else
	nAllHostagePoints = 0;
if (!gameStates.app.cheats.bEnabled && !IsMultiGame && LOCALPLAYER.lives && 
	 (gameData.missions.nCurrentLevel == gameData.missions.nLastLevel)) {		//tPlayer has finished the game!
	nEndGamePoints = LOCALPLAYER.lives * 10000;
	sprintf (szEndGame, "%s%i\n", TXT_SHIP_BONUS, nEndGamePoints);
	bIsLastLevel=1;
	}
else
	nEndGamePoints = bIsLastLevel = 0;
AddBonusPointsToScore (nSkillPoints + nEnergyPoints + nShieldPoints + nHostagePoints + nAllHostagePoints + nEndGamePoints);
c = 0;
sprintf (szMenu [c++], "%s%i", TXT_SHIELD_BONUS, nShieldPoints);		// Return at start to lower menu...
sprintf (szMenu [c++], "%s%i", TXT_ENERGY_BONUS, nEnergyPoints);
sprintf (szMenu [c++], "%s%i", TXT_HOSTAGE_BONUS, nHostagePoints);
sprintf (szMenu [c++], "%s%i", TXT_SKILL_BONUS, nSkillPoints);
sprintf (szMenu [c++], "%s", szAllHostages);
if (!(gameData.app.nGameMode & GM_MULTI) && (LOCALPLAYER.lives) && (gameData.missions.nCurrentLevel == gameData.missions.nLastLevel))
	sprintf (szMenu [c++], "%s", szEndGame);
sprintf (szMenu [c++], "%s%i\n", TXT_TOTAL_BONUS, nShieldPoints+nEnergyPoints+nHostagePoints+nSkillPoints+nAllHostagePoints+nEndGamePoints);
sprintf (szMenu [c++], "%s%i", TXT_TOTAL_SCORE, LOCALPLAYER.score);
memset (m, 0, sizeof (m));
for (i=0; i<c; i++) {
	m [i].nType = NM_TYPE_TEXT;
	m [i].text = szMenu [i];
	}
sprintf (szTitle,
			"%s%s %d %s\n%s %s",
			gameOpts->menus.nStyle ? "" : bIsLastLevel ? "\n\n\n":"\n",
			 (gameData.missions.nCurrentLevel < 0) ? TXT_SECRET_LEVEL : TXT_LEVEL, 
			 (gameData.missions.nCurrentLevel < 0) ? -gameData.missions.nCurrentLevel : gameData.missions.nCurrentLevel, 
			TXT_COMPLETE, 
			gameData.missions.szCurrentLevel, 
			TXT_DESTROYED);
Assert (c <= N_GLITZITEMS);
GrPaletteFadeOut (NULL, 32, 0);
PA_DFX (pa_alpha_always ());
if (network && (gameData.app.nGameMode & GM_NETWORK))
	ExecMenu2 (NULL, szTitle, c, m, (void (*))NetworkEndLevelPoll2, 0, STARS_BACKGROUND);
else
// NOTE LINK TO ABOVE!!!
gameStates.app.bGameRunning = 0;
ExecMenu2 (NULL, szTitle, c, m, NULL, 0, STARS_BACKGROUND);
}

//	-----------------------------------------------------------------------------------------------------

//give the tPlayer the opportunity to save his game
void DoEndlevelMenu ()
{
//No between level saves......!!!	StateSaveAll (1);
}

//	-----------------------------------------------------------------------------------------------------
//called when the tPlayer is starting a level (new game or new ship)
void StartSecretLevel ()
{
Assert (!gameStates.app.bPlayerIsDead);
InitPlayerPosition (0);
VerifyConsoleObject ();
gameData.objs.console->controlType	= CT_FLYING;
gameData.objs.console->movementType	= MT_PHYSICS;
// -- WHY? -- DisableMatCens ();
ClearTransientObjects (0);		//0 means leave proximity bombs
// CreatePlayerAppearanceEffect (gameData.objs.console);
gameStates.render.bDoAppearanceEffect = 1;
AIResetAllPaths ();
ResetTime ();
ResetRearView ();
gameData.fusion.xAutoFireTime = 0;
gameData.fusion.xCharge = 0;
gameStates.app.cheats.bRobotsFiring = 1;
gameOpts->sound.bD1Sound = gameStates.app.bD1Mission && gameStates.app.bHaveD1Data && gameOpts->sound.bUseD1Sounds;
if (gameStates.app.bD1Mission) {
	if (LOCALPLAYER.energy < INITIAL_ENERGY)
		LOCALPLAYER.energy = INITIAL_ENERGY;
	if (LOCALPLAYER.shields < INITIAL_SHIELDS)
		LOCALPLAYER.shields = INITIAL_SHIELDS;
	}
}

//------------------------------------------------------------------------------

//	Returns true if secret level has been destroyed.
int PSecretLevelDestroyed (void)
{
if (gameStates.app.bFirstSecretVisit)
	return 0;		//	Never been there, can't have been destroyed.
if (CFExist (SECRETC_FILENAME, gameFolders.szSaveDir, 0))
	return 0;
return 1;
}

//	-----------------------------------------------------------------------------------------------------

void DoSecretMessage (char *msg)
{
	int	fMode = gameStates.app.nFunctionMode;

StopTime ();
SetFunctionMode (FMODE_MENU);
ExecMessageBox (NULL, STARS_BACKGROUND, 1, TXT_OK, msg);
SetFunctionMode (fMode);
StartTime ();
}

//	-----------------------------------------------------------------------------------------------------

void InitSecretLevel (int nLevel)
{
Assert (gameData.missions.nCurrentLevel == nLevel);	//make sure level set right
Assert (gameStates.app.nFunctionMode == FMODE_GAME);
GameSeqInitNetworkPlayers (); // Initialize the gameData.multiplayer.players array for this level
HUDClearMessages ();
AutomapClearVisited ();
// --	InitPlayerStatsLevel ();
gameData.objs.viewer = gameData.objs.objects + LOCALPLAYER.nObject;
GameSeqRemoveUnusedPlayers ();
gameStates.app.bGameSuspended = 0;
gameData.reactor.bDestroyed = 0;
InitCockpit ();
ResetPaletteAdd ();
}

//	-----------------------------------------------------------------------------------------------------
// called when the tPlayer is starting a new level for normal game mode and restore state
//	Need to deal with whether this is the first time coming to this level or not.  If not the
//	first time, instead of initializing various things, need to do a game restore for all the
//	robots, powerups, walls, doors, etc.
int StartNewLevelSecret (int nLevel, int bPageInTextures)
{
	tMenuItem	m [1];
  //int i;

ThisLevelTime=0;

m [0].nType = NM_TYPE_TEXT;
m [0].text = " ";

gameStates.render.cockpit.nLastDrawn [0] = -1;
gameStates.render.cockpit.nLastDrawn [1] = -1;

if (gameData.demo.nState == ND_STATE_PAUSED)
	gameData.demo.nState = ND_STATE_RECORDING;

if (gameData.demo.nState == ND_STATE_RECORDING) {
	NDSetNewLevel (nLevel);
	NDRecordStartFrame (gameData.app.nFrameCount, gameData.time.xFrame);
	} 
else if (gameData.demo.nState != ND_STATE_PLAYBACK) {
	GrPaletteFadeOut (NULL, 32, 0);
	SetScreenMode (SCREEN_MENU);		//go into menu mode
	if (gameStates.app.bFirstSecretVisit)
		DoSecretMessage (gameStates.app.bD1Mission ? TXT_ALTERNATE_EXIT : TXT_SECRET_EXIT);
	else if (CFExist (SECRETC_FILENAME,gameFolders.szSaveDir,0))
		DoSecretMessage (gameStates.app.bD1Mission ? TXT_ALTERNATE_EXIT : TXT_SECRET_EXIT);
	else {
		char	text_str [128];

		sprintf (text_str, TXT_ADVANCE_LVL, gameData.missions.nCurrentLevel+1);
		DoSecretMessage (text_str);
		}
	}

if (gameStates.app.bFirstSecretVisit || (gameData.demo.nState == ND_STATE_PLAYBACK)) {
	if (!LoadLevel (nLevel, bPageInTextures, 0))
		return 0;
	InitSecretLevel (nLevel);
	if (!gameStates.app.bAutoRunMission && gameStates.app.bD1Mission)
		ShowLevelIntro (nLevel);
	PlayLevelSong (gameData.missions.nCurrentLevel, 0);
	InitRobotsForLevel ();
	InitAIObjects ();
	InitShakerDetonates ();
	MorphInit ();
	InitAllMatCens ();
	ResetSpecialEffects ();
	StartSecretLevel ();
	LOCALPLAYER.flags &= ~PLAYER_FLAGS_ALL_KEYS;
	}
else {
	if (CFExist (SECRETC_FILENAME, gameFolders.szSaveDir, 0)) {
		int	pw_save, sw_save;

		pw_save = gameData.weapons.nPrimary;
		sw_save = gameData.weapons.nSecondary;
		StateRestoreAll (1, 1, SECRETC_FILENAME);
		gameData.weapons.nPrimary = pw_save;
		gameData.weapons.nSecondary = sw_save;
		ResetSpecialEffects ();
		StartSecretLevel ();
		}
	else {
		char	text_str [128];

		sprintf (text_str, TXT_ADVANCE_LVL, gameData.missions.nCurrentLevel+1);
		DoSecretMessage (text_str);
		if (!LoadLevel (nLevel, bPageInTextures, 0))
			return 0;
		InitSecretLevel (nLevel);
		return 1;

		// -- //	If file doesn't exist, it's because reactor was destroyed.
		// -- // -- StartNewLevel (gameData.missions.secretLevelTable [-gameData.missions.nCurrentLevel-1]+1, 0);
		// -- StartNewLevel (gameData.missions.secretLevelTable [-gameData.missions.nCurrentLevel-1]+1, 0);
		// -- return;
		}
	}

if (gameStates.app.bFirstSecretVisit)
	CopyDefaultsToRobotsAll ();
TurnCheatsOff ();
InitReactorForLevel (0);
//	Say tPlayer can use FLASH cheat to mark path to exit.
nLastLevelPathCreated = -1;
gameStates.app.bFirstSecretVisit = 0;
return 1;
}

//------------------------------------------------------------------------------

//	Called from switch.c when tPlayer is on a secret level and hits exit to return to base level.
void ExitSecretLevel (void)
{
if (gameData.demo.nState == ND_STATE_PLAYBACK)
	return;
if (!(gameStates.app.bD1Mission || gameData.reactor.bDestroyed))
	StateSaveAll (0, 2, SECRETC_FILENAME);
if (!gameStates.app.bD1Mission && CFExist (SECRETB_FILENAME, gameFolders.szSaveDir, 0)) {
	int pw_save = gameData.weapons.nPrimary;
	int sw_save = gameData.weapons.nSecondary;

	ReturningToLevelMessage ();
	StateRestoreAll (1, 1, SECRETB_FILENAME);
	gameOpts->sound.bD1Sound = gameStates.app.bD1Mission && gameStates.app.bHaveD1Data && gameOpts->sound.bUseD1Sounds;
	SetDataVersion (-1);
	gameData.weapons.nPrimary = pw_save;
	gameData.weapons.nSecondary = sw_save;
	}
else {
	// File doesn't exist, so can't return to base level.  Advance to next one.
	if (gameData.missions.nEnteredFromLevel == gameData.missions.nLastLevel)
		DoEndGame ();
	else {
		if (!gameStates.app.bD1Mission)
			AdvancingToLevelMessage ();
		DoEndLevelScoreGlitz (0);
		StartNewLevel (gameData.missions.nEnteredFromLevel + 1, 0);
		}
	}
}

//------------------------------------------------------------------------------
//	Set invulnerableTime and cloakTime in tPlayer struct to preserve amount of time left to
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
//	Called from switch.c when tPlayer passes through secret exit.  That means he was on a non-secret level and he
//	is passing to the secret level.
//	Do a savegame.
void EnterSecretLevel (void)
{
	fix	xOldGameTime;
	int	i;

Assert (!(gameData.app.nGameMode & GM_MULTI));
gameData.missions.nEnteredFromLevel = gameData.missions.nCurrentLevel;
if (gameData.reactor.bDestroyed)
	DoEndLevelScoreGlitz (0);
if (gameData.demo.nState != ND_STATE_PLAYBACK)
	StateSaveAll (0, 1, NULL);	//	Not between levels (ie, save all), IS a secret level, NO filename override
//	Find secret level number to go to, stuff in gameData.missions.nNextLevel.
for (i = 0; i < -gameData.missions.nLastSecretLevel; i++)
	if (gameData.missions.secretLevelTable [i] == gameData.missions.nCurrentLevel) {
		gameData.missions.nNextLevel = -i - 1;
		break;
		} 
	else if (gameData.missions.secretLevelTable [i] > gameData.missions.nCurrentLevel) {	//	Allows multiple exits in same group.
		gameData.missions.nNextLevel = -i;
		break;
		}
if (i >= -gameData.missions.nLastSecretLevel)		//didn't find level, so must be last
	gameData.missions.nNextLevel = gameData.missions.nLastSecretLevel;
xOldGameTime = gameData.time.xGame;
StartNewLevelSecret (gameData.missions.nNextLevel, 1);
// do_cloak_invul_stuff ();
}

//------------------------------------------------------------------------------
//called when the tPlayer has finished a level
void PlayerFinishedLevel (int bSecret)
{
	Assert (!bSecret);

	//credit the tPlayer for hostages
LOCALPLAYER.hostages_rescuedTotal += LOCALPLAYER.hostages_on_board;
if (gameData.app.nGameMode & GM_NETWORK)
	LOCALPLAYER.connected = 2; // Finished but did not die
gameStates.render.cockpit.nLastDrawn [0] = -1;
gameStates.render.cockpit.nLastDrawn [1] = -1;
if (gameData.missions.nCurrentLevel < 0)
	ExitSecretLevel ();
else
	AdvanceLevel (0, 0);				//now go on to the next one (if one)
}

//------------------------------------------------------------------------------

#define MOVIE_REQUIRED 1

void show_order_form ();
extern void com_hangup (void);

//called when the tPlayer has finished the last level
void DoEndGame (void)
{
	char  szFilename [FILENAME_LEN];

SetFunctionMode (FMODE_MENU);
if ((gameData.demo.nState == ND_STATE_RECORDING) || (gameData.demo.nState == ND_STATE_PAUSED))
	NDStopRecording ();
SetScreenMode (SCREEN_MENU);
GrSetCurrentCanvas (NULL);
KeyFlush ();
if (!IsMultiGame) {
	if (gameData.missions.nCurrentMission == (gameStates.app.bD1Mission ? gameData.missions.nD1BuiltinMission : gameData.missions.nBuiltinMission)) {
		int bPlayed = MOVIE_NOT_PLAYED;	//default is not bPlayed

		if (!gameStates.app.bD1Mission) {
			InitSubTitles (ENDMOVIE ".tex");
			bPlayed = PlayMovie (ENDMOVIE, MOVIE_REQUIRED, 0, gameOpts->movies.bResize);
			CloseSubTitles ();
			}
		else if (gameStates.app.bHaveExtraMovies) {
			//InitSubTitles (ENDMOVIE ".tex");	//ingore errors
			bPlayed = PlayMovie (D1_ENDMOVIE, MOVIE_REQUIRED, 0, gameOpts->movies.bResize);
			CloseSubTitles ();
			}
		if (!bPlayed) {
			if (IS_D2_OEM) {
				SongsPlaySong (SONG_TITLE, 0);
				DoBriefingScreens ("end2oem.tex",1);
				}
			else {
				SongsPlaySong (SONG_ENDGAME, 0);
				DoBriefingScreens (gameStates.app.bD1Mission ? "endreg.tex" : "ending2.tex", gameStates.app.bD1Mission ? 0x7e : 1);
				}
			}
		}
	else {    //not multi
		char tname [FILENAME_LEN];
		PlayMovie (MakeLevelFilename (gameData.missions.nCurrentLevel, szFilename, ".mvx"), MOVIE_OPTIONAL, 0, gameOpts->movies.bResize);
		sprintf (tname, "%s.tex", gameStates.app.szCurrentMissionFile);
		DoBriefingScreens (tname, gameData.missions.nLastLevel + 1);   //level past last is endgame breifing

		//try doing special credits
		sprintf (tname,"%s.ctb",gameStates.app.szCurrentMissionFile);
		ShowCredits (tname);
		}
	}
KeyFlush ();
if (IsMultiGame)
	MultiEndLevelScore ();
else
	// NOTE LINK TO ABOVE
	DoEndLevelScoreGlitz (0);

if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) && 
	 !(gameData.app.nGameMode & (GM_MULTI | GM_MULTI_COOP))) {
	GrSetCurrentCanvas (NULL);
	GrClearCanvas (BLACK_RGBA);
	GrPaletteStepClear ();
	//LoadPalette (D2_DEFAULT_PALETTE, NULL, 0, 1, 0);
	MaybeAddPlayerScore (0);
	}
SetFunctionMode (FMODE_MENU);
if ((gameData.app.nGameMode & GM_SERIAL) || (gameData.app.nGameMode & GM_MODEM))
	gameData.app.nGameMode |= GM_GAME_OVER;		//preserve modem setting so go back into modem menu
else
	gameData.app.nGameMode = GM_GAME_OVER;
longjmp (gameExitPoint, 0);		// Exit out of game loop
}

//------------------------------------------------------------------------------
//from which level each do you get to each secret level
//called to go to the next level (if there is one)
//if bSecret is true, advance to secret level, else next normal one
//	Return true if game over.
void AdvanceLevel (int bSecret, int bFromSecret)
{
	int result;

Assert (!bSecret);
if ((!bFromSecret/* && gameStates.app.bD1Mission*/) &&
	 ((gameData.missions.nCurrentLevel != gameData.missions.nLastLevel) || 
	  extraGameInfo [IsMultiGame].bRotateLevels)) {
	if (IsMultiGame)
		MultiEndLevelScore ();	
	else
	// NOTE LINK TO ABOVE!!!
	DoEndLevelScoreGlitz (0);		//give bonuses
	}
gameData.reactor.bDestroyed = 0;
#ifdef EDITOR
if (gameData.missions.nCurrentLevel == 0)
	return;		//not a real level
#endif
if (gameData.app.nGameMode & GM_MULTI)	{
	if ((result = MultiEndLevel (&bSecret))) { // Wait for other players to reach this point
		if (gameData.missions.nCurrentLevel != gameData.missions.nLastLevel)		//tPlayer has finished the game!
			return;
		longjmp (gameExitPoint, 0);		// Exit out of game loop
		}
	}
if ((gameData.missions.nCurrentLevel == gameData.missions.nLastLevel) && 
	!extraGameInfo [IsMultiGame].bRotateLevels) //tPlayer has finished the game!
	DoEndGame ();
else {
	gameData.missions.nNextLevel = gameData.missions.nCurrentLevel + 1;		//assume go to next normal level
	if (gameData.missions.nNextLevel > gameData.missions.nLastLevel) {
		if (extraGameInfo [IsMultiGame].bRotateLevels)
			gameData.missions.nNextLevel = 1;
		else
			gameData.missions.nNextLevel = gameData.missions.nLastLevel;
		}
	if (!IsMultiGame)
		DoEndlevelMenu (); // Let user save their game
	StartNewLevel (gameData.missions.nNextLevel, 0);
	}
}

//------------------------------------------------------------------------------

void LoadStars (bkg *bg, int bRedraw)
{
NMLoadBackground (STARS_BACKGROUND, bg, bRedraw);
starsPalette = gameData.render.pal.pCurPal;
}

//------------------------------------------------------------------------------

void DiedInMineMessage (void)
{
	// Tell the tPlayer he died in the mine, explain why
	int old_fmode;

if (gameData.app.nGameMode & GM_MULTI)
	return;
GrPaletteFadeOut (NULL, 32, 0);
SetScreenMode (SCREEN_MENU);		//go into menu mode
GrSetCurrentCanvas (NULL);
old_fmode = gameStates.app.nFunctionMode;
SetFunctionMode (FMODE_MENU);
ExecMessageBox (NULL, STARS_BACKGROUND, 1, TXT_OK, TXT_DIED_IN_MINE);
SetFunctionMode (old_fmode);
}

//------------------------------------------------------------------------------
//	Called when tPlayer dies on secret level.
void ReturningToLevelMessage (void)
{
	char	msg [128];

	int old_fmode;

if (gameData.app.nGameMode & GM_MULTI)
	return;
StopTime ();
GrPaletteFadeOut (NULL, 32, 0);
SetScreenMode (SCREEN_MENU);		//go into menu mode
GrSetCurrentCanvas (NULL);
old_fmode = gameStates.app.nFunctionMode;
SetFunctionMode (FMODE_MENU);
if (gameData.missions.nEnteredFromLevel < 0)
	sprintf (msg, TXT_SECRET_LEVEL_RETURN);
else
	sprintf (msg, TXT_RETURN_LVL, gameData.missions.nEnteredFromLevel);
ExecMessageBox (NULL, STARS_BACKGROUND, 1, TXT_OK, msg);
SetFunctionMode (old_fmode);
StartTime ();
}

//------------------------------------------------------------------------------
//	Called when tPlayer dies on secret level.
void AdvancingToLevelMessage (void)
{
	char	msg [128];

	int old_fmode;

	//	Only supposed to come here from a secret level.
Assert (gameData.missions.nCurrentLevel < 0);
if (gameData.app.nGameMode & GM_MULTI)
	return;
GrPaletteFadeOut (NULL, 32, 0);
SetScreenMode (SCREEN_MENU);		//go into menu mode
GrSetCurrentCanvas (NULL);
old_fmode = gameStates.app.nFunctionMode;
SetFunctionMode (FMODE_MENU);
sprintf (msg, "Base level destroyed.\nAdvancing to level %i", gameData.missions.nEnteredFromLevel+1);
ExecMessageBox (NULL, STARS_BACKGROUND, 1, TXT_OK, msg);
SetFunctionMode (old_fmode);
}

//------------------------------------------------------------------------------

void DigiStopDigiSounds ();

void DoPlayerDead ()
{
	int bSecret = (gameData.missions.nCurrentLevel < 0);

gameStates.app.bGameRunning = 0;
ResetPaletteAdd ();
GrPaletteStepLoad (NULL);
DigiStopDigiSounds ();		//kill any continuing sounds (eg. forcefield hum)
DeadPlayerEnd ();		//terminate death sequence (if playing)
if (IsCoopGame && gameStates.app.bHaveExtraGameInfo [1])
	LOCALPLAYER.score = 
	(LOCALPLAYER.score * (100 - nCoopPenalties [extraGameInfo [1].nCoopPenalty])) / 100;
if (gameStates.multi.bPlayerIsTyping [gameData.multiplayer.nLocalPlayer] && (gameData.app.nGameMode & GM_MULTI))
	MultiSendMsgQuit ();
gameStates.entropy.bConquering = 0;
#ifdef EDITOR
if (gameData.app.nGameMode == GM_EDITOR) {			//test mine, not real level
	tObject * playerobj = gameData.objs.objects + LOCALPLAYER.nObject;
	//ExecMessageBox ("You're Dead!", 1, "Continue", "Not a real game, though.");
	LoadLevelSub ("gamesave.lvl");
	InitPlayerStatsNewShip ();
	playerobjP->flags &= ~OF_SHOULD_BE_DEAD;
	StartLevel (0);
	return;
}
#endif

if (gameData.app.nGameMode & GM_MULTI)
	MultiDoDeath (LOCALPLAYER.nObject);
else {				//Note link to above else!
	if (!--LOCALPLAYER.lives) {
		DoGameOver ();
		return;
		}
	}
if (gameData.reactor.bDestroyed) {
	//clear out stuff so no bonus
	LOCALPLAYER.hostages_on_board = 0;
	LOCALPLAYER.energy = 0;
	LOCALPLAYER.shields = 0;
	LOCALPLAYER.connected = 3;
	DiedInMineMessage (); // Give them some indication of what happened
	}
if (bSecret && !gameStates.app.bD1Mission) {
	ExitSecretLevel ();
	SetPosFromReturnSegment (1);
	LOCALPLAYER.lives--;	//	re-lose the life, LOCALPLAYER.lives got written over in restore.
	InitPlayerStatsNewShip ();
	gameStates.render.cockpit.nLastDrawn [0] =
	gameStates.render.cockpit.nLastDrawn [1] = -1;
	}
else {
	if (gameData.reactor.bDestroyed) {
		//AdvancingToLevelMessage ();
		AdvanceLevel (0, bSecret);
		//StartNewLevel (gameData.missions.nCurrentLevel + 1, 0);
		InitPlayerStatsNewShip ();
		}
	else if (!gameStates.entropy.bExitSequence) {
		InitPlayerStatsNewShip ();
		StartLevel (1);
		}
	}
SetSoundSources ();
DigiSyncSounds ();
}

//------------------------------------------------------------------------------

//called when the tPlayer is starting a new level for normal game mode and restore state
//	bSecret set if came from a secret level
int StartNewLevelSub (int nLevel, int bPageInTextures, int bSecret, int bRestore)
{
	int funcRes;

	gameStates.multi.bTryAutoDL = 1;

reloadLevel:

if (!IsMultiGame) {
	gameStates.render.cockpit.nLastDrawn [0] =
	gameStates.render.cockpit.nLastDrawn [1] = -1;
	}
 gameStates.render.cockpit.bBigWindowSwitch=0;
if (gameData.demo.nState == ND_STATE_PAUSED)
	gameData.demo.nState = ND_STATE_RECORDING;
if (gameData.demo.nState == ND_STATE_RECORDING) {
	NDSetNewLevel (nLevel);
	NDRecordStartFrame (gameData.app.nFrameCount, gameData.time.xFrame);
	}
if (gameData.app.nGameMode & GM_MULTI)
	SetFunctionMode (FMODE_MENU); // Cheap fix to prevent problems with errror dialogs in loadlevel.
SetWarnFunc (ShowInGameWarning);
funcRes = LoadLevel (nLevel, bPageInTextures, bRestore);
ClearWarnFunc (ShowInGameWarning);
if (!funcRes)
	return 0;
Assert (gameStates.app.bAutoRunMission || (gameData.missions.nCurrentLevel == nLevel));	//make sure level set right
GameSeqInitNetworkPlayers (); // Initialize the gameData.multiplayer.players array for
#ifdef _DEBUG										  // this level
InitHoardData ();
SetMonsterballForces ();
#endif
//	gameData.objs.viewer = gameData.objs.objects + LOCALPLAYER.nObject;
if (gameData.multiplayer.nPlayers > gameData.multiplayer.nPlayerPositions) {
	ExecMessageBox (NULL, NULL, 1, TXT_OK, "Too many players for this level.");
	return 0;
	}
Assert (gameData.multiplayer.nPlayers <= gameData.multiplayer.nPlayerPositions);
	//If this assert fails, there's not enough start positions

if (gameData.app.nGameMode & GM_NETWORK) {
	switch (NetworkLevelSync ()) { // After calling this, gameData.multiplayer.nLocalPlayer is set
		case -1:
			return 0;
		case 0:
			if (gameData.multiplayer.nLocalPlayer < 0)
				return 0;
			GrRemapMonoFonts ();
			break;
		case 1:
			networkData.nStatus = 2;
			goto reloadLevel;
		}
	}
if ((gameData.app.nGameMode & GM_SERIAL) || (gameData.app.nGameMode & GM_MODEM))
	if (comLevel_sync ())
		return 1;

Assert (gameStates.app.nFunctionMode == FMODE_GAME);
HUDClearMessages ();
AutomapClearVisited ();
if (networkData.bNewGame == 1) {
	networkData.bNewGame = 0;
	InitPlayerStatsNewShip ();
}
InitPlayerStatsLevel (bSecret);
if ((gameData.app.nGameMode & GM_MULTI_COOP) && networkData.bRejoined) {
	int i;
	for (i = 0; i < gameData.multiplayer.nPlayers; i++)
		gameData.multiplayer.players [i].flags |= netGame.playerFlags [i];
	}
if (IsMultiGame)
	MultiPrepLevel (); // Removes robots from level if necessary
else
	FindMonsterball (); //will simply remove all Monsterballs
GameSeqRemoveUnusedPlayers ();
gameStates.app.bGameSuspended = 0;
gameData.reactor.bDestroyed = 0;
gameStates.render.glFOV = DEFAULT_FOV;
SetScreenMode (SCREEN_GAME);
InitCockpit ();
InitRobotsForLevel ();
InitShakerDetonates ();
MorphInit ();
InitAllMatCens ();
ResetPaletteAdd ();
InitThiefForLevel ();
InitStuckObjects ();
GameFlushInputs ();		// clear out the keyboard
if (!IsMultiGame)
	FilterObjectsFromLevel ();
TurnCheatsOff ();
if (!(IsMultiGame || gameStates.app.cheats.bEnabled)) {
	SetHighestLevel (gameData.missions.nCurrentLevel);
	}
else
	ReadPlayerFile (1);		//get window sizes
ResetSpecialEffects ();
if (networkData.bRejoined == 1){
	networkData.bRejoined = 0;
	StartLevel (1);
	}
else {
	StartLevel (0);		// Note link to above if!
	}
CopyDefaultsToRobotsAll ();
if (!bRestore)
	InitReactorForLevel (0);
InitAIObjects ();
#if 0
LOCALPLAYER.nInvuls =
LOCALPLAYER.nCloaks = 0;
#endif
//	Say tPlayer can use FLASH cheat to mark path to exit.
nLastLevelPathCreated = -1;
return 1;
}

//------------------------------------------------------------------------------

void BashToShield (int i, char *s)
{
	tObject *objP = gameData.objs.objects + i;
	int id = objP->id;

gameData.multiplayer.powerupsInMine [id] =
gameData.multiplayer.maxPowerupsAllowed [id] = 0;
objP->nType = OBJ_POWERUP;
objP->id = POW_SHIELD_BOOST;
objP->renderType = RT_POWERUP;
objP->controlType = CT_POWERUP;
objP->size = gameData.objs.pwrUp.info [POW_SHIELD_BOOST].size;
objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [POW_SHIELD_BOOST].nClipIndex;
objP->rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][objP->rType.vClipInfo.nClipIndex].xFrameTime;
}

//------------------------------------------------------------------------------

void BashToEnergy (int i,char *s)
{
	tObject *objP = gameData.objs.objects + i;
	int id = objP->id;

gameData.multiplayer.powerupsInMine [id] =
gameData.multiplayer.maxPowerupsAllowed [id] = 0;
objP->nType = OBJ_POWERUP;
objP->id = POW_ENERGY;
objP->renderType = RT_POWERUP;
objP->controlType = CT_POWERUP;
objP->size = gameData.objs.pwrUp.info [POW_ENERGY].size;
objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [POW_ENERGY].nClipIndex;
objP->rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][objP->rType.vClipInfo.nClipIndex].xFrameTime;
}

//------------------------------------------------------------------------------

void FilterObjectsFromLevel ()
 {
  int i;

for (i = 0; i <= gameData.objs.nLastObject; i++) {
	if (gameData.objs.objects [i].nType==OBJ_POWERUP)
		if (gameData.objs.objects [i].id==POW_REDFLAG || gameData.objs.objects [i].id==POW_BLUEFLAG)
			BashToShield (i,"Flag!!!!");
		}
  }

//------------------------------------------------------------------------------

struct {
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
	int i, bPlayed = 0;
	char szFilename [FILENAME_LEN];

	if (PlayMovie (MakeLevelFilename (nLevel, szFilename, ".mvi"), MOVIE_OPTIONAL, 0, gameOpts->movies.bResize))
		return;
	if (!gameStates.app.bD1Mission && (gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission)) {
		if (IS_SHAREWARE) {
			if (nLevel == 1)
				DoBriefingScreens ("brief2.tex", 1);
			}
		else if (IS_D2_OEM) {
			if ((nLevel == 1) && !gameStates.movies.bIntroPlayed)
				DoBriefingScreens ("brief2o.tex", 1);
			}
		else { // full version
			if (gameStates.app.bHaveExtraMovies && (nLevel == 1)) {
				PlayIntroMovie ();
				}
			for (i = 0; i < NUM_INTRO_MOVIES; i++) {
				if (szIntroMovies [i].nLevel == nLevel) {
					gameStates.video.nScreenMode = -1;
					PlayMovie (szIntroMovies [i].szMovieName, MOVIE_REQUIRED, 0, gameOpts->movies.bResize);
					bPlayed=1;
					break;
					}
				}
			if (gameStates.movies.nRobots) {
				int hires_save = gameStates.menus.bHiresAvailable;
				if (gameStates.movies.nRobots == 1) {		//lowres only
					gameStates.menus.bHiresAvailable = 0;		//pretend we can't do highres
					if (hires_save != gameStates.menus.bHiresAvailable)
						gameStates.video.nScreenMode = -1;		//force reset
					}
				DoBriefingScreens ("robot.tex", nLevel);
				gameStates.menus.bHiresAvailable = hires_save;
				}
			}
		}
	else {	//not the built-in mission.  check for add-on briefing
		if ((gameData.missions.list [gameData.missions.nCurrentMission].nDescentVersion == 1) &&
			 (gameData.missions.nCurrentMission == gameData.missions.nD1BuiltinMission)) {
			if (gameStates.app.bHaveExtraMovies && (nLevel == 1)) {
				if (PlayMovie ("briefa.mve", MOVIE_REQUIRED, 0, gameOpts->movies.bResize) != MOVIE_ABORTED)
					 PlayMovie ("briefb.mve", MOVIE_REQUIRED, 0, gameOpts->movies.bResize);
				}		
			DoBriefingScreens (gameData.missions.szBriefingFilename, nLevel);
			}
		else {
			char tname [FILENAME_LEN];
			sprintf (tname, "%s.tex", gameStates.app.szCurrentMissionFile);
			DoBriefingScreens (tname, nLevel);
			}
		}
	}
}

//	---------------------------------------------------------------------------
//	If starting a level which appears in the gameData.missions.secretLevelTable, then set gameStates.app.bFirstSecretVisit.
//	Reason: On this level, if tPlayer goes to a secret level, he will be going to a different
//	secret level than he's ever been to before.
//	Sets the global gameStates.app.bFirstSecretVisit if necessary.  Otherwise leaves it unchanged.
void MaybeSetFirstSecretVisit (int nLevel)
{
	int	i;

	for (i=0; i<gameData.missions.nSecretLevels; i++) {
		if (gameData.missions.secretLevelTable [i] == nLevel) {
			gameStates.app.bFirstSecretVisit = 1;
		}
	}
}

//------------------------------------------------------------------------------
//called when the tPlayer is starting a new level for normal game model
//	bSecret if came from a secret level
int StartNewLevel (int nLevel, int bSecret)
{
	ThisLevelTime=0;

if ((nLevel > 0) && !bSecret)
	MaybeSetFirstSecretVisit (nLevel);
if (!gameStates.app.bAutoRunMission)
	ShowLevelIntro (nLevel);
return StartNewLevelSub (nLevel, 1, bSecret, 0);
}

//------------------------------------------------------------------------------
//initialize the tPlayer tObject position & orientation (at start of game, or new ship)
void InitPlayerPosition (int bRandom)
{
	int bNewPlayer=0;

	if (!((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode&GM_MULTI_COOP))) // If not deathmatch
		bNewPlayer = gameData.multiplayer.nLocalPlayer;
	else if (bRandom == 1) {
		tObject *pObj;
		int spawnMap [MAX_NUM_NET_PLAYERS];
		int nSpawnSegs = 0;
		int i, closest = -1, trys = 0;
		fix closestDist = 0x7ffffff, dist;


		for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++)
			spawnMap [i] = i;

		d_srand (SDL_GetTicks ());

		do {
			trys++;
			if (gameData.app.nGameMode & GM_TEAM) {
				if (!nSpawnSegs) {
					for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++)
						spawnMap [i] = i;
					nSpawnSegs = gameData.multiplayer.nPlayerPositions;
					}
				if (nSpawnSegs) {		//try to find a spawn location owned by the tPlayer's team
					closestDist = 0;
					i = d_rand () % nSpawnSegs;
					bNewPlayer = spawnMap [i];
					if (i < --nSpawnSegs)
						spawnMap [i] = spawnMap [nSpawnSegs];
					switch (gameData.multiplayer.playerInit [bNewPlayer].nSegType) {
						case SEGMENT_IS_GOAL_RED:
						case SEGMENT_IS_TEAM_RED:
							if (GetTeam (gameData.multiplayer.nLocalPlayer) != TEAM_RED)
								continue;
							break;
						case SEGMENT_IS_GOAL_BLUE:
						case SEGMENT_IS_TEAM_BLUE:
							if (GetTeam (gameData.multiplayer.nLocalPlayer) != TEAM_BLUE)
								continue;
							break;
						default:
							break;
						}
					}
				}
			else {
				bNewPlayer = d_rand () % gameData.multiplayer.nPlayerPositions;
				}
			closest = -1;
			closestDist = 0x7fffffff;
			for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
				if (i == gameData.multiplayer.nLocalPlayer)
					continue;
				pObj = gameData.objs.objects + gameData.multiplayer.players [i].nObject; 
				if ((pObj->nType == OBJ_PLAYER))	{
					dist = FindConnectedDistance (&pObj->position.vPos, 
															 pObj->nSegment, 
															 &gameData.multiplayer.playerInit [bNewPlayer].position.vPos, 
															 gameData.multiplayer.playerInit [bNewPlayer].nSegment, 
															 10, WID_FLY_FLAG);	//	Used to be 5, search up to 10 segments
					if ((dist < closestDist) && (dist >= 0))	{
						closestDist = dist;
						closest = i;
					}
				}
			}
		} while ((closestDist < i2f (15 * 20)) && (trys < MAX_NUM_NET_PLAYERS * 2));
	}
	else {
		goto done; // If deathmatch and not random, positions were already determined by sync packet
	}
	Assert (bNewPlayer >= 0);
	Assert (bNewPlayer < gameData.multiplayer.nPlayerPositions);

	gameData.objs.console->position.vPos = gameData.multiplayer.playerInit [bNewPlayer].position.vPos;
	gameData.objs.console->position.mOrient = gameData.multiplayer.playerInit [bNewPlayer].position.mOrient;
 	RelinkObject (OBJ_IDX (gameData.objs.console), gameData.multiplayer.playerInit [bNewPlayer].nSegment);
done:
	ResetPlayerObject ();
	ResetCruise ();
}

//------------------------------------------------------------------------------

fix RobotDefaultShields (tObject *objP)
{
	tRobotInfo	*botInfoP;
	int			objId, i;
	fix			shields;

Assert (objP->nType == OBJ_ROBOT);
objId = objP->id;
//Assert (objId < gameData.bots.nTypes [0]);
i = gameStates.app.bD1Mission && (objId < gameData.bots.nTypes [1]);
botInfoP = gameData.bots.info [i] + objId;
//	Boost shield for Thief and Buddy based on level.
shields = botInfoP->strength;
if (botInfoP->thief || botInfoP->companion) {
	shields = (shields * (abs (gameData.missions.nCurrentLevel) + 7)) / 8;
	if (botInfoP->companion) {
		//	Now, scale guide-bot hits by skill level
		switch (gameStates.app.nDifficultyLevel) {
			case 0:
				shields = i2f (20000);
				break;		//	Trainee, basically unkillable
			case 1:
				shields *= 3;			
				break;		//	Rookie, pretty dang hard
			case 2:
				shields *= 2;			
				break;		//	Hotshot, a bit tough
			default:
				break;
			}
		}
	} 
else if (botInfoP->bossFlag) {	//	MK, 01/16/95, make boss shields lower on lower diff levels.
	shields = shields / (NDL + 3) * (gameStates.app.nDifficultyLevel + 4);
//	Additional wimpification of bosses at Trainee
	if (gameStates.app.nDifficultyLevel == 0)
		shields /= 2;
	}
return shields;
}

//------------------------------------------------------------------------------
//	Initialize default parameters for one robot, copying from gameData.bots.pInfo to *objP.
//	What about setting size!?  Where does that come from?
void CopyDefaultsToRobot (tObject *objP)
{
objP->shields = RobotDefaultShields (objP);
}

//------------------------------------------------------------------------------
//	Copy all values from the robot info structure to all instances of robots.
//	This allows us to change bitmaps.tbl and have these changes manifested in existing robots.
//	This function should be called at level load time.
void CopyDefaultsToRobotsAll ()
{
	int	i;

	for (i=0; i<=gameData.objs.nLastObject; i++)
		if (gameData.objs.objects [i].nType == OBJ_ROBOT)
			CopyDefaultsToRobot (&gameData.objs.objects [i]);

}

extern void ClearStuckObjects (void);

//------------------------------------------------------------------------------
//called when the tPlayer is starting a level (new game or new ship)
void StartLevel (int bRandom)
{
Assert (!gameStates.app.bPlayerIsDead);
InitPlayerPosition (bRandom);
VerifyConsoleObject ();
gameData.objs.console->controlType = CT_FLYING;
gameData.objs.console->movementType = MT_PHYSICS;
MultiSendShields ();
DisableMatCens ();
ClearTransientObjects (0);		//0 means leave proximity bombs
// CreatePlayerAppearanceEffect (gameData.objs.console);
gameStates.render.bDoAppearanceEffect = 1;
if (gameData.app.nGameMode & GM_MULTI) {
	if (gameData.app.nGameMode & GM_MULTI_COOP)
		MultiSendScore ();
	MultiSendPosition (LOCALPLAYER.nObject);
	MultiSendReappear ();
	}	
if (gameData.app.nGameMode & GM_NETWORK)
	NetworkDoFrame (1, 1);
AIResetAllPaths ();
AIInitBossForShip ();
ClearStuckObjects ();
#ifdef EDITOR
//	Note, this is only done if editor builtin.  Calling this from here
//	will cause it to be called after the tPlayer dies, resetting the
//	hits for the buddy and thief.  This is ok, since it will work ok
//	in a shipped version.
InitAIObjects ();
#endif
ResetTime ();
ResetRearView ();
gameData.fusion.xAutoFireTime = 0;
gameData.fusion.xCharge = 0;
gameStates.app.cheats.bRobotsFiring = 1;
gameStates.app.cheats.bD1CheatsEnabled = 0;
//DigiClose ();
gameOpts->sound.bD1Sound = gameStates.app.bD1Mission && gameStates.app.bHaveD1Data && gameOpts->sound.bUseD1Sounds;
SetDataVersion (-1);
//DigiInit ();
}

//------------------------------------------------------------------------------
//eof
