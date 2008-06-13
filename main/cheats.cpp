/* $Id: gamecntl.c, v 1.23 2003/11/07 06:30:06 btb Exp $ */
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
 * Game Controls Stuff
 *
 */

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#include <string.h>

#include "pstypes.h"
#include "inferno.h"
#include "collide.h"
#include "gameseg.h"
#include "laser.h"
#include "omega.h"
#include "network.h"
#include "multi.h"
#include "newdemo.h"
#include "scores.h"
#include "text.h"
#include "gauges.h"
#include "hudmsg.h"
#include "sphere.h"
#include "escort.h"
#include "switch.h"
#include "key.h"
#include "slowmotion.h"

//	Cheat functions ------------------------------------------------------------

char nInterpolationMethodSave;
char bOldHomingStates [20];

char szCheatBuf[] = "AAAAAAAAAAAAAAA";

//------------------------------------------------------------------------------

void DoCheatPenalty ()
{
#ifndef _DEBUG
DigiPlaySampleClass (SOUND_CHEATER, NULL, F1_0, SOUNDCLASS_PLAYER);
gameStates.app.cheats.bEnabled = 1;
LOCALPLAYER.score = 0;
#endif
}

//------------------------------------------------------------------------------

void MultiDoCheatPenalty ()
{
DoCheatPenalty ();
LOCALPLAYER.shields = i2f (1);
MultiSendShields ();
LOCALPLAYER.energy = i2f (1);
if (gameData.app.nGameMode & GM_MULTI) {
	gameData.multigame.msg.nReceiver = 100;		// Send to everyone...
	sprintf (gameData.multigame.msg.szMsg, TXT_CRIPPLED, LOCALPLAYER.callsign);
	}
HUDInitMessage (TXT_TAKE_THAT);
}

//------------------------------------------------------------------------------

inline fix BoostVal (fix *curVal, fix maxVal)
{
if (*curVal < maxVal) {
	fix boost = 3 * F1_0 + 3 * F1_0 * (NDL - gameStates.app.nDifficultyLevel);
	if (gameStates.app.nDifficultyLevel == 0)
		boost += boost / 2;
	*curVal += boost;
	if (*curVal > maxVal)
		*curVal = maxVal;
	return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

int MenuGetValues (const char *pszMsg, int *valueP, int nValues)
{
	tMenuItem	m;
	char			text [20] = "", *psz;
	int			i = 0;

memset (&m, 0, sizeof (m));
m.nType = NM_TYPE_INPUT; 
m.text_len = 20; 
m.text = text;
if (ExecMenu (NULL, pszMsg, 1, &m, NULL, NULL) >= 0) {
	valueP [0] = atoi (m.text);
	for (i = 1, psz = m.text; --nValues && (psz = strchr (psz, ',')); i++)
		valueP [i] = atoi (++psz);
	}
return i;
}

//------------------------------------------------------------------------------

int KillAllBuddyBots (int bVerbose)
{
	int	i, nKilled = 0;
	tObject *objP;
	//int	boss_index = -1;

for (i = 0, objP = OBJECTS; i <= gameData.objs.nLastObject; i++, objP++)
	if ((objP->nType == OBJ_ROBOT) && ROBOTINFO (objP->id).companion) {
		if (gameStates.app.bNostalgia)
			objP->flags |= OF_EXPLODING | OF_SHOULD_BE_DEAD;
		else 
			ApplyDamageToRobot (objP, objP->shields + 1, -1);
		if (bVerbose)
			HUDInitMessage (TXT_BUDDY_TOASTED);
		nKilled++;
		}
if (bVerbose)
	HUDInitMessage (TXT_BOTS_TOASTED, nKilled);
return nKilled;
}

//------------------------------------------------------------------------------

void KillAllRobots (int bVerbose)
{
	int	i, nKilled = 0;
	tObject *objP;
	//int	boss_index = -1;

// Kill all bots except for Buddy bot and boss.  However, if only boss and buddy left, kill boss.
for (i = 0, objP = OBJECTS; i <= gameData.objs.nLastObject; i++, objP++)
	if ((objP->nType == OBJ_ROBOT) &&
		 !(ROBOTINFO (objP->id).companion || ROBOTINFO (objP->id).bossFlag)) {
		nKilled++;
		if (gameStates.app.bNostalgia)
			objP->flags |= OF_EXPLODING | OF_SHOULD_BE_DEAD;
		else {
			ApplyDamageToRobot (objP, objP->shields + 1, -1);
			objP->flags |= OF_ARMAGEDDON;
			}
		}
// Toast the buddy if nothing else toasted!
if (!nKilled)
	nKilled += KillAllBuddyBots (bVerbose);
if (bVerbose)
	HUDInitMessage (TXT_BOTS_TOASTED, nKilled);
}

//------------------------------------------------------------------------------

void KillAllBossRobots (int bVerbose)
{
	int		i, nKilled = 0;
	tObject	*objP;

if (gameStates.gameplay.bKillBossCheat)
	gameStates.gameplay.bKillBossCheat = 0;
else {
	for (i = 0, objP = OBJECTS; i<=gameData.objs.nLastObject; i++, objP++)
		if ((objP->nType == OBJ_ROBOT) && ROBOTINFO (objP->id).bossFlag) {
			nKilled++;
			if (gameStates.app.bNostalgia)
				objP->flags |= OF_EXPLODING | OF_SHOULD_BE_DEAD;
			else {
				ApplyDamageToRobot (objP, objP->shields + 1, -1);
				objP->flags |= OF_ARMAGEDDON;
				}
			gameStates.gameplay.bKillBossCheat = 1;
			}
	}
if (bVerbose)
	HUDInitMessage (TXT_BOTS_TOASTED, nKilled);
}

//	--------------------------------------------------------------------------
//	Detonate reactor.
//	Award tPlayer all powerups in mine.
//	Place tPlayer just outside exit.
//	Kill all bots in mine.
//	Yippee!!
void KillEverything (int bVerbose)
{
	int     i, j;

if (bVerbose)
	HUDInitMessage (TXT_KILL_ETC);
for (i = 0; i <= gameData.objs.nLastObject; i++) {
	switch (OBJECTS [i].nType) {
		case OBJ_ROBOT:
		case OBJ_REACTOR:
			OBJECTS [i].flags |= OF_EXPLODING | OF_SHOULD_BE_DEAD;
			break;
		case OBJ_POWERUP:
			DoPowerup (OBJECTS + i, -1);
			break;
		}
	}
extraGameInfo [0].nBossCount =
extraGameInfo [1].nBossCount = 0;
DoReactorDestroyedStuff (NULL);
for (i = 0; i < gameData.trigs.nTriggers; i++) {
	if (gameData.trigs.triggers [i].nType == TT_EXIT) {
		for (j = 0; j < gameData.walls.nWalls; j++) {
			if (gameData.walls.walls [j].nTrigger == i) {
				short nSegment = gameData.walls.walls [j].nSegment;
				COMPUTE_SEGMENT_CENTER_I (&gameData.objs.console->position.vPos, nSegment);
				RelinkObject (OBJ_IDX (gameData.objs.console), nSegment);
				gameData.objs.console->position.mOrient.fVec = gameData.segs.segments [nSegment].sides [gameData.walls.walls [j].nSide].normals [0];
				VmVecNegate (&gameData.objs.console->position.mOrient.fVec);
				return;
				}
			}
		}
	}
// make sure exit gets opened
gameStates.gameplay.bKillBossCheat = 0;
}

//------------------------------------------------------------------------------

void KillThief (int bVerbose)
{
	int     i;
	tObject *objP;

for (i = 0, objP = OBJECTS; i <= gameData.objs.nLastObject; i++, objP++)
	if ((objP->nType == OBJ_ROBOT) && ROBOTINFO (objP->id).thief) {
		if (gameStates.app.bNostalgia)
			objP->flags |= OF_EXPLODING | OF_SHOULD_BE_DEAD;
		else {
			ApplyDamageToRobot (objP, objP->shields + 1, -1);
			objP->flags |= OF_ARMAGEDDON;
			}
		if (bVerbose)
			HUDInitMessage (TXT_THIEF_TOASTED);
		}
}

//------------------------------------------------------------------------------

#ifdef _DEBUG

void KillAllSnipers (int bVerbose)
{
	int     i, nKilled=0;

//	Kill all snipers.
for (i = 0; i <= gameData.objs.nLastObject; i++)
	if (OBJECTS [i].nType == OBJ_ROBOT)
		if (OBJECTS [i].cType.aiInfo.behavior == AIB_SNIPE) {
			nKilled++;
			OBJECTS [i].flags |= OF_EXPLODING | OF_SHOULD_BE_DEAD;
		}
if (bVerbose)
	HUDInitMessage (TXT_BOTS_TOASTED, nKilled);
}

#endif

//------------------------------------------------------------------------------

void KillBuddy (int bVerbose)
{
	int     i;
	tObject	*objP = OBJECTS;

	//	Kill buddy.
for (i = 0; i <= gameData.objs.nLastObject; i++, objP++)
	if ((objP->nType == OBJ_ROBOT) && ROBOTINFO (objP->id).companion) {
		OBJECTS [i].flags |= OF_EXPLODING | OF_SHOULD_BE_DEAD;
		if (bVerbose)
			HUDInitMessage (TXT_BUDDY_TOASTED);
		}
}

//------------------------------------------------------------------------------

void AccessoryCheat (int bVerbose)
{
if (!gameStates.app.bD1Mission) {
	LOCALPLAYER.flags |= 
		PLAYER_FLAGS_HEADLIGHT | 
		PLAYER_FLAGS_AFTERBURNER | 
		PLAYER_FLAGS_AMMO_RACK | 
		PLAYER_FLAGS_CONVERTER |
		PLAYER_FLAGS_SLOWMOTION |
		PLAYER_FLAGS_BULLETTIME;
	gameData.physics.xAfterburnerCharge = f1_0;
	}
if (bVerbose)
	HUDInitMessage (TXT_ACCESSORIES);
}

//------------------------------------------------------------------------------

void AcidCheat (int bVerbose)
{
if (gameStates.app.cheats.bAcid) {
	gameStates.app.cheats.bAcid = 0;
	gameStates.render.nInterpolationMethod = nInterpolationMethodSave;
	gameStates.render.glFOV = DEFAULT_FOV;
	if (bVerbose)
		HUDInitMessage (TXT_COMING_DOWN);
	}
else {
	gameStates.app.cheats.bAcid = 1;
	nInterpolationMethodSave=gameStates.render.nInterpolationMethod;
	gameStates.render.nInterpolationMethod = 1;
	gameStates.render.glFOV = FISHEYE_FOV;
	if (bVerbose)
		HUDInitMessage (TXT_GOING_UP);
	}
}

//------------------------------------------------------------------------------

void AfterburnerCheat (int bVerbose)
{
if (!gameStates.app.bD1Mission)
	gameStates.gameplay.bAfterburnerCheat = !gameStates.gameplay.bAfterburnerCheat;
if (gameStates.gameplay.bAfterburnerCheat) {
	LOCALPLAYER.flags |= PLAYER_FLAGS_AFTERBURNER;
	HUDInitMessage (TXT_AB_CHEAT);
	}
}

//------------------------------------------------------------------------------

void AhimsaCheat (int bVerbose)
{
gameStates.app.cheats.bRobotsFiring = !gameStates.app.cheats.bRobotsFiring;
if (gameStates.app.cheats.bRobotsFiring) {
	if (bVerbose)
		HUDInitMessage (TXT_BOTFIRE_ON);
	}
else {
	DoCheatPenalty ();
	if (bVerbose)
		HUDInitMessage (TXT_BOTFIRE_OFF);
	}
}

//------------------------------------------------------------------------------

void AllKeysCheat (int bVerbose)
{
if (bVerbose)
	HUDInitMessage (TXT_ALL_KEYS);
LOCALPLAYER.flags |= PLAYER_FLAGS_ALL_KEYS;
}

//------------------------------------------------------------------------------

void BlueOrbCheat (int bVerbose)
{
if (BoostVal (&LOCALPLAYER.shields, MAX_SHIELDS)) {
	MultiSendShields ();
	PowerupBasic (0, 0, 15, SHIELD_SCORE, "%s %s %d", TXT_SHIELD, TXT_BOOSTED_TO, 
						f2ir (LOCALPLAYER.shields));
	}
else if (bVerbose)
	HUDInitMessage (TXT_MAXED_OUT, TXT_SHIELD);
}

//------------------------------------------------------------------------------

void BuddyDudeCheat (int bVerbose)
{
gameStates.app.cheats.bMadBuddy = !gameStates.app.cheats.bMadBuddy;
if (gameStates.app.cheats.bMadBuddy) {
	strcpy (gameData.escort.szName, "Wingnut");
	if (bVerbose)
		HUDInitMessage (TXT_GB_ANGRY, gameData.escort.szName);
	}
else {
	strcpy (gameData.escort.szName, gameData.escort.szRealName);
	if (bVerbose)
		HUDInitMessage (TXT_GB_CALM, gameData.escort.szName);
	}
}

//------------------------------------------------------------------------------

void BuddyLifeCheat (int bVerbose)
{
if (bVerbose)
	HUDInitMessage (TXT_GB_CLONE);
CreateBuddyBot ();
}

//------------------------------------------------------------------------------

void BouncyCheat (int bVerbose)
{
if (bVerbose)
	HUDInitMessage (TXT_WPN_BOUNCE);
gameStates.app.cheats.bBouncingWeapons = 1;
}

//------------------------------------------------------------------------------

void CloakCheat (int bVerbose)
{
	int	bCloaked;

if (!(LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED))
	LOCALPLAYER.flags |= PLAYER_FLAGS_CLOAKED;
else if (LOCALPLAYER.cloakTime == 0x7fffffff)
	LOCALPLAYER.flags &= ~PLAYER_FLAGS_CLOAKED;
bCloaked = (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) != 0;
if (bVerbose)
	HUDInitMessage ("%s %s!", TXT_CLOAKED, bCloaked ? TXT_ON : TXT_OFF);
LOCALPLAYER.cloakTime = bCloaked ? 0x7fffffff : 0; //gameData.time.xGame + i2f (1000);
}

//------------------------------------------------------------------------------

void CubeWarpCheat (int bVerbose)
{
	int nNewSegSide [2] = {0, 0};

MenuGetValues (TXT_ENTER_SEGNUM, nNewSegSide, 2);
if ((nNewSegSide [0] >= 0) && (nNewSegSide [0] <= gameData.segs.nLastSegment)) {
	DoCheatPenalty ();
	if (nNewSegSide [1] < 0)
		nNewSegSide [1] = 0;
	else if (nNewSegSide [1] > 5)
		nNewSegSide [1] = 5;
	TriggerSetObjOrient (LOCALPLAYER.nObject, (short) nNewSegSide [0], (short) nNewSegSide [1], 1, 0);
	TriggerSetObjPos (LOCALPLAYER.nObject, (short) nNewSegSide [0]);
	}
}

//------------------------------------------------------------------------------

void ElectroCheat (int bVerbose)
{
if (BoostVal (&LOCALPLAYER.energy, MAX_ENERGY))
	 PowerupBasic (15, 15, 7, ENERGY_SCORE, "%s %s %d", TXT_ENERGY, TXT_BOOSTED_TO, 
						 f2ir (LOCALPLAYER.energy));
else if (bVerbose)
	HUDInitMessage (TXT_MAXED_OUT, TXT_SHIELD);
}

//------------------------------------------------------------------------------

void ExitPathCheat (int bVerbose)
{
MarkPathToExit ();
}

//------------------------------------------------------------------------------

void ExtraLifeCheat (int bVerbose)
{
LOCALPLAYER.lives++;
PowerupBasic (20, 20, 20, 0, TXT_EXTRA_LIFE);
}

//------------------------------------------------------------------------------

void FinishLevelCheat (int bVerbose)
{
KillEverything (bVerbose);
}

//------------------------------------------------------------------------------

void FramerateCheat (int bVerbose)
{
gameStates.render.bShowFrameRate = !gameStates.render.bShowFrameRate;
}

//------------------------------------------------------------------------------

void FullMapCheat (int bVerbose)
{
if (gameStates.render.bAllVisited)
	gameStates.render.bViewDist++;
else if (LOCALPLAYER.flags & PLAYER_FLAGS_FULLMAP)
	gameStates.render.bAllVisited = 1;
else
	LOCALPLAYER.flags |= PLAYER_FLAGS_FULLMAP;
if (bVerbose)
	HUDInitMessage (TXT_FULL_MAP);
}

//------------------------------------------------------------------------------

void GasolineCheat (int bVerbose)
{
LOCALPLAYER.shields = MAX_SHIELDS;
MultiSendShields ();
LOCALPLAYER.energy = MAX_ENERGY;
if (bVerbose)
	HUDInitMessage (TXT_SLURP);
}

//------------------------------------------------------------------------------

void HomingCheat (int bVerbose)
{
	int	i;

if ((gameStates.app.cheats.bHomingWeapons = !gameStates.app.cheats.bHomingWeapons)) {
	for (i = 0; i < 20; i++) {
		bOldHomingStates [i] = WI_homingFlag (i);
		WI_set_homingFlag (i, 1);
		}
	if (bVerbose)
		HUDInitMessage (TXT_WPN_HOMING);
	}
else {
	for (i = 0; i < 20; i++) 
		WI_set_homingFlag (i, bOldHomingStates [i]);
	}
}

//------------------------------------------------------------------------------

void InvulCheat (int bVerbose)
{
	int	bInvul;

if (!(LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE))
	LOCALPLAYER.flags |= PLAYER_FLAGS_INVULNERABLE;
else if (LOCALPLAYER.invulnerableTime == 0x7fffffff)
	LOCALPLAYER.flags &= ~PLAYER_FLAGS_INVULNERABLE;
bInvul = (LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE) != 0;
if (bVerbose)
	HUDInitMessage ("%s %s!", TXT_INVULNERABILITY, bInvul ? TXT_ON : TXT_OFF);
LOCALPLAYER.invulnerableTime = bInvul ? 0x7fffffff : 0; //gameData.time.xGame + i2f (1000);
SetSpherePulse (gameData.multiplayer.spherePulse + gameData.multiplayer.nLocalPlayer, 0.02f, 0.5f);
}

//------------------------------------------------------------------------------

void FillBackground ();
void LoadBackgroundBitmap ();

void JohnHeadCheat (int bVerbose)
{
gameStates.app.cheats.bJohnHeadOn = !gameStates.app.cheats.bJohnHeadOn;
LoadBackgroundBitmap ();
FillBackground ();
if (bVerbose)
	HUDInitMessage (gameStates.app.cheats.bJohnHeadOn? TXT_HI_JOHN : TXT_BYE_JOHN);
}

//------------------------------------------------------------------------------

void KillBossCheat (int bVerbose)
{
if (bVerbose)
	HUDInitMessage (TXT_BAMBI_WINS);
KillAllBossRobots (bVerbose);
}

//------------------------------------------------------------------------------

void KillBuddyCheat (int bVerbose)
{
if (bVerbose)
	HUDInitMessage (TXT_TRUE_DREAMS);
KillAllBuddyBots (bVerbose);
}

//------------------------------------------------------------------------------

void KillThiefCheat (int bVerbose)
{
if (bVerbose)
	HUDInitMessage (TXT_RIGHTEOUS);
KillThief (bVerbose);
}

//------------------------------------------------------------------------------

void KillRobotsCheat (int bVerbose)
{
if (bVerbose)
	HUDInitMessage (TXT_ARMAGEDDON);
KillAllRobots (bVerbose);
ShakerRockStuff ();
}

//------------------------------------------------------------------------------

void LevelWarpCheat (int bVerbose)
{
int nNewLevel;

MenuGetValues (TXT_WARP_TO_LEVEL, &nNewLevel, 1);
if ((nNewLevel > 0) && (nNewLevel <= gameData.missions.nLastLevel)) {
	DoCheatPenalty ();
	StartNewLevel (nNewLevel, 0);
	}
}

//------------------------------------------------------------------------------

void MonsterCheat (int bVerbose)
{
gameStates.app.cheats.bMonsterMode = !gameStates.app.cheats.bMonsterMode;
if (bVerbose)
	HUDInitMessage (gameStates.app.cheats.bMonsterMode ? TXT_MONSTER_ON : TXT_MONSTER_OFF);
}

//------------------------------------------------------------------------------

void PhysicsCheat (int bVerbose)
{
gameStates.app.cheats.bPhysics = 0xbada55;
if (bVerbose)
	HUDInitMessage (TXT_LOUP_GAROU);
}

//------------------------------------------------------------------------------

void RapidFireCheat (int bVerbose)
{
if (gameStates.app.cheats.bLaserRapidFire) {
	gameStates.app.cheats.bLaserRapidFire = 0;
	if (bVerbose)
		HUDInitMessage ("%s", TXT_RAPIDFIRE_OFF);
	}
else {
	gameStates.app.cheats.bLaserRapidFire = 0xbada55;
	DoCheatPenalty ();
	if (bVerbose)
		HUDInitMessage ("%s", TXT_RAPIDFIRE_ON);
	}
}

//------------------------------------------------------------------------------

void RobotsKillRobotsCheat (int bVerbose)
{
gameStates.app.cheats.bRobotsKillRobots = !gameStates.app.cheats.bRobotsKillRobots;
if (!gameStates.app.cheats.bRobotsKillRobots) {
	if (bVerbose)
		HUDInitMessage (TXT_KILL_PLAYER);
	}
else {
	DoCheatPenalty ();
	if (bVerbose)
		HUDInitMessage (TXT_RABID_BOTS);
	}
}

//------------------------------------------------------------------------------

void SpeedCheat (int bVerbose)
{
if ((gameStates.app.cheats.bSpeed = !gameStates.app.cheats.bSpeed)) {
	BulletTimeOn ();
	DoCheatPenalty ();
	}
else
	SlowMotionOff ();
}

//------------------------------------------------------------------------------

void TriFusionCheat (int bVerbose)
{
	tPlayer	*playerP = &LOCALPLAYER;

if (gameData.multiplayer.weaponStates [gameData.multiplayer.nLocalPlayer].bTripleFusion)
	return;
playerP->primaryWeaponFlags |= 1 << FUSION_INDEX;
gameData.weapons.bTripleFusion = 1;
SelectWeapon (4, 0, 1, 1);
DoCheatPenalty ();
}

//------------------------------------------------------------------------------

void TurboCheat (int bVerbose)
{
gameStates.app.cheats.bTurboMode = !gameStates.app.cheats.bTurboMode;
if (!gameStates.app.cheats.bTurboMode) {
	if (bVerbose)
		HUDInitMessage (TXT_DILATED);
	}
else {
	if (bVerbose)
		HUDInitMessage (TXT_SWOOSH);
	DoCheatPenalty ();
	}
}

//------------------------------------------------------------------------------

void UnlockAllCheat (int bVerbose)
{
#if 1//def _DEBUG
UnlockAllWalls (!gameStates.app.cheats.nUnlockLevel);
if (bVerbose)
	HUDInitMessage (!gameStates.app.cheats.nUnlockLevel ? TXT_ROBBING_BANK : TXT_LET_ME_OVER);
#else
UnlockAllWalls (gameStates.app.bD1Mission || !gameStates.app.cheats.nUnlockLevel);
if (bVerbose)
	HUDInitMessage ((gameStates.app.bD1Mission || !gameStates.app.cheats.nUnlockLevel) ? TXT_ROBBING_BANK : TXT_LET_ME_OVER);
#endif
gameStates.app.cheats.nUnlockLevel = 1;
}

//------------------------------------------------------------------------------

void DoWowieCheat (int bVerbose, int bInitialize)
{
	int	h, i;

if (gameStates.app.bD1Mission) {
	LOCALPLAYER.primaryWeaponFlags = (1 << LASER_INDEX | (1 << VULCAN_INDEX) | (1 << SPREADFIRE_INDEX) | (1 << PLASMA_INDEX)) | (1 << FUSION_INDEX);
	LOCALPLAYER.secondaryWeaponFlags = (1 << CONCUSSION_INDEX) | (1 << HOMING_INDEX) | (1 << PROXMINE_INDEX) | (1 << SMART_INDEX) | (1 << MEGA_INDEX);
	for (i = 0; i < MAX_D1_PRIMARY_WEAPONS; i++)
		LOCALPLAYER.primaryAmmo [i] = nMaxPrimaryAmmo [i];
	for (i = 0; i < MAX_D1_SECONDARY_WEAPONS; i++)
		LOCALPLAYER.secondaryAmmo [i] = nMaxSecondaryAmmo [i];
	}
else {
	if (gameData.pig.tex.nHamFileVersion < 3) {// SHAREWARE
		LOCALPLAYER.primaryWeaponFlags = ~ ((1<<PHOENIX_INDEX) | (1<<OMEGA_INDEX) | (1<<FUSION_INDEX) | HAS_FLAG (SUPER_LASER_INDEX));
		LOCALPLAYER.secondaryWeaponFlags = ~ ((1<<MERCURY_INDEX) | (1<<MEGA_INDEX) | (1<<EARTHSHAKER_INDEX));
		}
	else {
		LOCALPLAYER.primaryWeaponFlags = 0xffff ^ HAS_FLAG (SUPER_LASER_INDEX);		//no super laser
		LOCALPLAYER.secondaryWeaponFlags = 0xffff;
		}
	h = (LOCALPLAYER.flags & PLAYER_FLAGS_AMMO_RACK) ? 2 : 1;
	for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
		LOCALPLAYER.primaryAmmo [i] = nMaxPrimaryAmmo [i] * h;
	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
		LOCALPLAYER.secondaryAmmo [i] = nMaxSecondaryAmmo [i] * h;
	if (!COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0))
		LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX] = 4;
	if (gameData.pig.tex.nHamFileVersion < 3) {// SHAREWARE
		LOCALPLAYER.secondaryAmmo [MERCURY_INDEX] = 0;
		LOCALPLAYER.secondaryAmmo [EARTHSHAKER_INDEX] = 0;
		LOCALPLAYER.secondaryAmmo [MEGA_INDEX] = 0;
		}

	if (gameData.app.nGameMode & GM_HOARD)
		LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX] = 12;
	else if (gameData.app.nGameMode & GM_ENTROPY) {
		LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX] = 5 * h;
		LOCALPLAYER.secondaryAmmo [SMARTMINE_INDEX] = 5 * h;
		}
	}
bLastSecondaryWasSuper [PROXMINE_INDEX] = 1;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordLaserLevel (LOCALPLAYER.laserLevel, MAX_LASER_LEVEL);

LOCALPLAYER.energy = MAX_ENERGY;
if (gameStates.app.bD1Mission)
	LOCALPLAYER.laserLevel = MAX_LASER_LEVEL;
else
	LOCALPLAYER.laserLevel = MAX_SUPER_LASER_LEVEL;
LOCALPLAYER.flags |= PLAYER_FLAGS_QUAD_LASERS;
gameData.physics.xAfterburnerCharge = f1_0;
SetMaxOmegaCharge ();
UpdateLaserWeaponInfo ();
if (bInitialize)
	SetLastSuperWeaponStates ();
}

//------------------------------------------------------------------------------

void WowieCheat (int bVerbose)
{
if (bVerbose)
	HUDInitMessage (TXT_WOWIE_ZOWIE);
DoWowieCheat (bVerbose, 1);
}

//------------------------------------------------------------------------------

void SuperWowieCheat (int bVerbose)
{
if (gameStates.gameplay.bMineMineCheat) {
	gameStates.gameplay.bMineMineCheat = 0;
	LOCALPLAYER.flags &= ~(PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE);
	LOCALPLAYER.invulnerableTime =
	LOCALPLAYER.cloakTime = 0;
	}
else {
	AccessoryCheat (bVerbose);
	WowieCheat (bVerbose);
	LOCALPLAYER.flags |= PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE;
	LOCALPLAYER.invulnerableTime =
	LOCALPLAYER.cloakTime = 0x7fffffff;
	gameStates.app.cheats.bSpeed = 1;
	LOCALPLAYER.primaryWeaponFlags |= 1 << FUSION_INDEX;
	gameData.weapons.bTripleFusion = 1;
	gameStates.gameplay.bMineMineCheat = 1;
	SetSpherePulse (gameData.multiplayer.spherePulse + gameData.multiplayer.nLocalPlayer, 0.02f, 0.5f);
	}
}

//------------------------------------------------------------------------------

void EnableD1Cheats (int bVerbose)
{
gameStates.app.cheats.bD1CheatsEnabled = !gameStates.app.cheats.bD1CheatsEnabled;
if (bVerbose)
	HUDInitMessage (gameStates.app.cheats.bD1CheatsEnabled ? TXT_WANNA_CHEAT : LOCALPLAYER.score ? TXT_GOODGUY : TXT_TOOLATE);
}

//------------------------------------------------------------------------------

#define CHEATSPOT 14
#define CHEATEND 15

typedef void tCheatFunc (int bVerbose);
typedef tCheatFunc *pCheatFunc;

typedef struct tCheat {
	const char		*pszCheat;
	pCheatFunc	cheatFunc;
	char			bPunish;		//0: never punish, 1: always punish, -1: cheat function decides whether to punish
	char			bEncrypted;
	char			bD1Cheat;
} tCheat;

static char *pszCheat;

//------------------------------------------------------------------------------

inline int Cheat (tCheat *pCheat)
{
if (strcmp (pCheat->bEncrypted ? pszCheat : szCheatBuf + CHEATEND - strlen (pCheat->pszCheat), 
				pCheat->pszCheat))
	return 0;	// not this cheatcode
#ifndef _DEBUG
if (pCheat->bPunish && IsMultiGame &&
	 !(gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bEnableCheats)) {	//trying forbidden cheatcode in multiplayer
	MultiDoCheatPenalty ();
	return 1;
	}
if ((pCheat->bD1Cheat != -1) && (pCheat->bD1Cheat != gameStates.app.bD1Mission)) {	//trying cheat code from other game version
	MultiDoCheatPenalty ();
	return 1;
	}
if ((pCheat->bD1Cheat > 0) && !gameStates.app.cheats.bD1CheatsEnabled)	//D1 cheats not enabled
	return 1;
#endif
if (pCheat->bPunish > 0) {
	DoCheatPenalty ();
	if (IsMultiGame)
		MultiSendCheating ();
	}
if (pCheat->cheatFunc) {
	pCheat->cheatFunc (1);
	}
return 1;
}

//------------------------------------------------------------------------------

extern char *jcrypt (char *);

#define N_LAMER_CHEATS (sizeof (LamerCheats) / sizeof (*LamerCheats))

char szAccessoryCheat [9]			= "dWdz[kCK";		// al-ifalafel
char szAcidCheat [9]					= "qPmwxz\"S";		// bit-tersweet
char szAfterburnerCheat [9]		= "emontree";		// L-emontree ("The World's Fastest Indian" anybody? ;)
char szAhimsaCheat [9]				= "!Uscq_yc";		// New for 1.1 / im-agespace 
char szAllKeysCheat [9]				= "%v%MrgbU";		//only Matt knows / or-algroove
char szBlueOrbCheat [8]				= "blueorb";
char szBouncyCheat [9]				= "bGbiChQJ";		//only Matt knows / duddaboo
char szBuddyDudeCheat [9]			= "u#uzIr%e";		//only Matt knows / g-owingnut
char szBuddyLifeCheat [9]			= "%A-BECuY";		//only Matt knows / he-lpvishnu
char szCloakCheat [9]				= "itsarock";
char szCubeWarpCheat [9]			= "subspace";		// subspace
char szElectroCheat [9]				= "electro";
char szExitPathCheat [9]			= "glebells";		//jin-glebless
char szFinishLevelCheat [9]		= "%bG_bZ<D";		//only Matt knows / d-elshiftb
char szFramerateCheat [9]			= "rQ60#ZBN";		// f-rametime
char szFullMapCheat [9]				= "PI<XQHRI";		//only Matt knows / rockrgrl
char szGasolineCheat [9]			= "?:w8t]M'";		// pumpmeup / New for D2X-XL
char szHomingCheat [9]				= "t\\LIhSB[";		//only Matt knows / l-pnlizard
char szInvulCheat [9]				= "Wv_\\JJ\\Z";	//only Matt knows / almighty
char szJohnHeadCheat [9]			= "ou]];H:%";		// p-igfarmer
char szKillBossCheat [9]			= "odgethis";		//d-odgethis
char szKillBuddyCheat [9]			= "pitapita";		//
char szKillThiefCheat [9]			= "dgedredd";		//ju-dgedredd
char szKillRobotsCheat [9]			= "&wxbs:5O";		//only Matt knows / spaniard
char szLevelWarpCheat [9]			= "ZQHtqbb\"";		//only Matt knows / f-reespace
char szMonsterCheat [9]				= "nfpEfRQp";		//only Matt knows / godzilla
char szRapidFireCheat [9]			= "*jLgHi'J";		//only Matt knows / wildfire
char szRobotsKillRobotsCheat [9] = "rT6xD__S";		// New for 1.1 / silkwing
char szUnlockAllCheat [9]			= "yptonite";		// cr-yptonite / New for D2X-XL
char szWowieCheat [9]				= "F_JMO3CV";		//only Matt knows / h-onestbob
char szSuperWowieCheat [9]			= "minemine";
char szSpeedCheat [9]				= "estheone";		//h-estheone
char szTriFusionCheat [9]			= "cottmeup";		//s-cottmeup (Scott me up, beamy!)

tCheat cheats [] = {
	// Descent 2
	{szAccessoryCheat, AccessoryCheat, 1, 1, 0}, 
	{szAcidCheat, AcidCheat, 0, 1, 0}, 
	{szAfterburnerCheat, AfterburnerCheat, 1, 0, 0}, 
	{szAhimsaCheat, AhimsaCheat, -1, 1, 0}, 
	{szAllKeysCheat, AllKeysCheat, 1, 1, 0}, 
	{szBlueOrbCheat, BlueOrbCheat, 1, 0, 0}, 
	{szBouncyCheat, BouncyCheat, 1, 1, 0}, 
	{szBuddyDudeCheat, BuddyDudeCheat, 1, 1, 0}, 
	{szBuddyLifeCheat, BuddyLifeCheat, 1, 1, 0}, 
	{szCloakCheat, CloakCheat, 1, 0, 0}, 
	{szCubeWarpCheat, CubeWarpCheat, -1, 0, 0}, 
	{szElectroCheat, ElectroCheat, 1, 0, 0}, 
	{szExitPathCheat, ExitPathCheat, 1, 0, 0}, 
	{szFinishLevelCheat, FinishLevelCheat, 1, 1, 0}, 
	{szFramerateCheat, FramerateCheat, 0, 1, -1}, 
	{szGasolineCheat, GasolineCheat, 1, 1, 0}, 
	{szFullMapCheat, FullMapCheat, 1, 1, 0}, 
	{szHomingCheat, HomingCheat, 1, 1, 0}, 
	{szInvulCheat, InvulCheat, 1, 1, 0}, 
	{szJohnHeadCheat, JohnHeadCheat, 0, 1, 0}, 
	{szKillBossCheat, KillBossCheat, 1, 0, 0}, 
	{szKillBuddyCheat, KillBuddyCheat, 0, 0, 0}, 
	{szKillThiefCheat, KillThiefCheat, 1, 0, 0}, 
	{szKillRobotsCheat, KillRobotsCheat, 1, 1, 0}, 
	{szLevelWarpCheat, LevelWarpCheat, -1, 1, 0}, 
	{szMonsterCheat, MonsterCheat, 1, 1, 0}, 
	{szRapidFireCheat, RapidFireCheat, -1, 1, 0}, 
	{szRobotsKillRobotsCheat, RobotsKillRobotsCheat, -1, 1, 0}, 
	{szSpeedCheat, SpeedCheat, -1, 0, 0}, 
	{szTriFusionCheat, TriFusionCheat, -1, 0, 0}, 
	{szUnlockAllCheat, UnlockAllCheat, 1, 0, 0}, 
	{szSuperWowieCheat, SuperWowieCheat, 1, 0, 0}, 
	{szWowieCheat, WowieCheat, 1, 1, 0}, 
	// Descent 1
	{"ahimsa", AhimsaCheat, 1, 0, 1}, 
	{"armerjoe", LevelWarpCheat, 1, 0, 1}, 
	{"astral", PhysicsCheat, 1, 0, 1}, 
	{"bigred", WowieCheat, 1, 0, 1}, 
	{"bruin", ExtraLifeCheat, 1, 0, 1}, 
	{"buggin", TurboCheat, -1, 0, 1}, 
	{"flash", ExitPathCheat, 1, 0, 1}, 
	{"gabbahey", EnableD1Cheats, 0, 0, -1}, 
	{"guile", CloakCheat, 0, 0, -1}, 
	{"lunacy", AhimsaCheat, 1, 0, 1}, 
	{"mitzi", AllKeysCheat, 1, 0, 1}, 
	{"opsytoys", FinishLevelCheat, 1, 0, 1}, 
	{"pletch", NULL, 1, 0, 1}, 
	{"poboys", FinishLevelCheat, 1, 0, 1}, 
	{"porgys", WowieCheat, 1, 0, 1}, 
	{"racerx", InvulCheat, 1, 0, 1}, 
	{"scourge", WowieCheat, 1, 0, 1}, 
	// obsolete (demo?)
	{"ei5cQ-ZQ", NULL, 1, 1, 0}, // mo-therlode
	{"q^EpZxs8", NULL, 1, 1, 0}, // c-urrygoat
	{"mxk (DyyP", NULL, 1, 1, 0}, // zi-ngermans
	{"cBo#@y@P", NULL, 1, 1, 0}, // ea-tangelos
	{"CLygLBGQ", NULL, 1, 1, 0}, // e-ricaanne
	{"xAnHQxZX", NULL, 1, 1, 0}, // jos-huaakira
	{"cKc[KUWo", NULL, 1, 1, 0}, // wh-ammazoom
	{NULL, NULL, 0, 0, 0}
	};


//	Main Cheat function

//------------------------------------------------------------------------------

void FinalCheats (int key)
{
	int		i;
	tCheat	*pCheat;

key = KeyToASCII (key);
for (i = 0; i < 15; i++)
	szCheatBuf [i] = szCheatBuf [i+1];
szCheatBuf [CHEATSPOT] = key;
pszCheat = jcrypt (szCheatBuf + 7);
for (pCheat = cheats; pCheat->pszCheat && !Cheat (pCheat); pCheat++)
	;
}

//------------------------------------------------------------------------------
// Internal Cheat Menu
#ifdef _DEBUG
void DoCheatMenu ()
{
	int mmn;
	tMenuItem mm[16];
	char score_text[21];

	sprintf ( score_text, "%d", LOCALPLAYER.score );

	memset (mm, 0, sizeof (mm));
	mm[0].nType=NM_TYPE_CHECK; 
	mm[0].value=LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE; 
	mm[0].text="Invulnerability";
	mm[1].nType=NM_TYPE_CHECK; 
	mm[1].value=LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED; 
	mm[1].text="Cloaked";
	mm[2].nType=NM_TYPE_CHECK; 
	mm[2].value=0; 
	mm[2].text="All keys";
	mm[3].nType=NM_TYPE_NUMBER; 
	mm[3].value=f2i (LOCALPLAYER.energy); 
	mm[3].text="% Energy"; mm[3].minValue=0; 
	mm[3].maxValue=200;
	mm[4].nType=NM_TYPE_NUMBER; 
	mm[4].value=f2i (LOCALPLAYER.shields); 
	mm[4].text="% Shields"; mm[4].minValue=0; 
	mm[4].maxValue=200;
	mm[5].nType=NM_TYPE_TEXT; 
	mm[5].text = "Score:";
	mm[6].nType=NM_TYPE_INPUT; 
	mm[6].text_len = 10; 
	mm[6].text = score_text;
	//mm[7].nType=NM_TYPE_RADIO; mm[7].value= (LOCALPLAYER.laserLevel==0); mm[7].group=0; mm[7].text="Laser level 1";
	//mm[8].nType=NM_TYPE_RADIO; mm[8].value= (LOCALPLAYER.laserLevel==1); mm[8].group=0; mm[8].text="Laser level 2";
	//mm[9].nType=NM_TYPE_RADIO; mm[9].value= (LOCALPLAYER.laserLevel==2); mm[9].group=0; mm[9].text="Laser level 3";
	//mm[10].nType=NM_TYPE_RADIO; mm[10].value= (LOCALPLAYER.laserLevel==3); mm[10].group=0; mm[10].text="Laser level 4";

	mm[7].nType=NM_TYPE_NUMBER; 
	mm[7].value=LOCALPLAYER.laserLevel+1; 
	mm[7].text="Laser Level"; mm[7].minValue=0; 
	mm[7].maxValue=MAX_SUPER_LASER_LEVEL+1;
	mm[8].nType=NM_TYPE_NUMBER; 
	mm[8].value=LOCALPLAYER.secondaryAmmo [CONCUSSION_INDEX]; 
	mm[8].text="Missiles"; 
	mm[8].minValue=0; 
	mm[8].maxValue=200;

	mmn = ExecMenu ("Wimp Menu", NULL, 9, mm, NULL, NULL );

	if (mmn > -1 )  {
		if ( mm[0].value )  {
			LOCALPLAYER.flags |= PLAYER_FLAGS_INVULNERABLE;
			LOCALPLAYER.invulnerableTime = gameData.time.xGame+i2f (1000);
		} else
			LOCALPLAYER.flags &= ~PLAYER_FLAGS_INVULNERABLE;
		if ( mm[1].value ) {
			LOCALPLAYER.flags |= PLAYER_FLAGS_CLOAKED;
			if (gameData.app.nGameMode & GM_MULTI)
				MultiSendCloak ();
			AIDoCloakStuff ();
			LOCALPLAYER.cloakTime = gameData.time.xGame;
		}
		else
			LOCALPLAYER.flags &= ~PLAYER_FLAGS_CLOAKED;

		if (mm[2].value) LOCALPLAYER.flags |= PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY;
		LOCALPLAYER.energy=i2f (mm[3].value);
		LOCALPLAYER.shields=i2f (mm[4].value);
		LOCALPLAYER.score = atoi (mm[6].text);
		//if (mm[7].value) LOCALPLAYER.laserLevel=0;
		//if (mm[8].value) LOCALPLAYER.laserLevel=1;
		//if (mm[9].value) LOCALPLAYER.laserLevel=2;
		//if (mm[10].value) LOCALPLAYER.laserLevel=3;
		LOCALPLAYER.laserLevel = mm[7].value-1;
		LOCALPLAYER.secondaryAmmo [CONCUSSION_INDEX] = mm[8].value;
		InitGauges ();
	}
}
#endif


//------------------------------------------------------------------------------
//eof
