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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "inferno.h"
#include "gauges.h"
#include "error.h"
#include "text.h"
#include "newdemo.h"
#include "newmenu.h"
#include "network.h"
#include "u_mem.h"
#include "input.h"
#include "headlight.h"

#if defined (TACTILE)
#include "tactile.h"
#endif

//	Note, only Vulcan cannon requires ammo.
// NOTE: Now Vulcan and Gauss require ammo. -5/3/95 Yuan
//ubyte	DefaultPrimaryAmmoLevel [MAX_PRIMARY_WEAPONS] = {255, 0, 255, 255, 255};
//ubyte	DefaultSecondaryAmmoLevel [MAX_SECONDARY_WEAPONS] = {3, 0, 0, 0, 0};

//	Convert primary weapons to indices in gameData.weapons.info array.
ubyte primaryWeaponToWeaponInfo [MAX_PRIMARY_WEAPONS] = {
	LASER_ID, VULCAN_ID, SPREADFIRE_ID, PLASMA_ID, FUSION_ID, 
	SUPERLASER_ID, GAUSS_ID, HELIX_ID, PHOENIX_ID, OMEGA_ID};

ubyte secondaryWeaponToWeaponInfo [MAX_SECONDARY_WEAPONS] = {
	CONCUSSION_ID, HOMINGMSL_ID, PROXMINE_ID, SMARTMSL_ID, MEGAMSL_ID, 
	FLASHMSL_ID, GUIDEDMSL_ID, SMARTMINE_ID, MERCURYMSL_ID, EARTHSHAKER_ID};

//for each Secondary weapon, which gun it fires out of
ubyte secondaryWeaponToGunNum [MAX_SECONDARY_WEAPONS] = {4,4,7,7,7,4,4,7,4,7};

int nMaxPrimaryAmmo [MAX_PRIMARY_WEAPONS] = {
	0, VULCAN_AMMO_MAX, 0, 0, 0, 
	0, VULCAN_AMMO_MAX, 0, 0, 0};

ubyte nMaxSecondaryAmmo [MAX_SECONDARY_WEAPONS] = {20, 10, 10, 5, 5, 20, 20, 15, 10, 10};

//for each primary weapon, what kind of powerup gives weapon
ubyte primaryWeaponToPowerup [MAX_PRIMARY_WEAPONS] = {
	POW_LASER, POW_VULCAN, POW_SPREADFIRE, POW_PLASMA, POW_FUSION, 
	POW_LASER, POW_GAUSS, POW_HELIX, POW_PHOENIX, POW_OMEGA};

//for each Secondary weapon, what kind of powerup gives weapon
ubyte secondaryWeaponToPowerup [MAX_SECONDARY_WEAPONS] = {
	POW_CONCUSSION_1, POW_HOMINGMSL_1, POW_PROXMINE, POW_SMARTMSL, POW_MEGAMSL,
	POW_FLASHMSL_1, POW_GUIDEDMSL_1, POW_SMARTMINE, POW_MERCURYMSL_1, POW_EARTHSHAKER};

D2D1_weapon_info weaponInfoD2D1 [D1_MAX_WEAPON_TYPES] = {
	{0,1,0,1,0,0,0,32768,16384,{655360,655360,655360,655360,655360},{7864320,7864320,7864320,7864320,7864320},32768,0,0,49152,655360,0},
	{0,1,0,1,0,0,0,32768,16384,{720896,720896,720896,720896,720896},{8192000,8192000,8192000,8192000,8192000},32768,0,0,65536,655360,0},
	{0,1,0,1,0,0,0,32768,13107,{786432,786432,786432,786432,786432},{8519680,8519680,8519680,8519680,8519680},32768,0,0,81920,655360,0},
	{0,1,0,1,0,0,0,32768,13107,{851968,851968,851968,851968,851968},{8847360,8847360,8847360,8847360,8847360},32768,0,0,114688,655360,0},
	{0,1,0,1,0,0,0,32768,16384,{131072,196608,262144,327680,393216},{983040,1966080,2949120,3932160,5570560},65536,0,0,65536,655360,0},
	{0,1,0,1,0,0,0,32768,16384,{196608,262144,393216,458752,524288},{983040,1966080,2949120,3932160,5570560},65536,0,0,65536,655360,0},
	{0,1,0,1,0,0,0,32768,16384,{262144,393216,589824,720896,851968},{983040,1966080,2949120,3932160,5570560},65536,0,0,65536,655360,0},
	{0,0,0,0,0,0,0,0,0,{0,0,0,0,0},{0,0,0,0,0},0,0,0,0,0,0},
	{0,1,1,0,0,0,0,0,32768,{1966080,1966080,1966080,1966080,1966080},{5898240,5898240,5898240,5898240,5898240},65536,0,0,262144,655360,1966080},
	{0,1,0,1,0,0,0,65536,75366,{65536,65536,65536,65536,65536},{7864320,7864320,7864320,7864320,7864320},6553,0,0,196608,327680,0},
	{0,1,0,1,0,0,0,32768,16384,{131072,196608,196608,262144,393216},{1966080,2621440,3276800,3932160,5242880},6553,0,0,65536,655360,0},
	{0,1,1,0,0,0,0,0,3276,{262144,262144,262144,262144,262144},{32768000,32768000,32768000,32768000,32768000},655,0,0,0,196608,0},
	{0,1,0,1,0,0,0,32768,13107,{655360,655360,655360,655360,655360},{7864320,7864320,7864320,7864320,7864320},13107,0,0,0,655360,0},
	{0,1,0,1,0,0,0,32768,9830,{720896,720896,720896,720896,720896},{9830400,9830400,9830400,9830400,9830400},655,0,0,131072,655360,0},
	{1,1,0,1,0,0,0,0,65536,{3932160,3932160,3932160,3932160,3932160},{9830400,9830400,9830400,9830400,9830400},32768,0,0,327680,655360,0},
	{0,1,1,0,0,0,1,0,32768,{2621440,2621440,2621440,2621440,2621440},{5898240,5898240,5898240,5898240,5898240},65536,0,0,262144,1310720,1310720},
	{0,1,1,1,1,1,0,0,13107,{1966080,1966080,1966080,1966080,1966080},{0,0,0,0,0},65536,2162,0,0,2293760,2621440},
	{0,1,1,0,0,0,0,0,32768,{1638400,1638400,1638400,1638400,1638400},{5570560,5570560,5570560,5570560,5570560},65536,0,0,524288,196608,1310720},
	{0,1,1,0,0,0,1,0,32768,{13041664,13041664,13041664,13041664,13041664},{5570560,5570560,5570560,5570560,5570560},65536,0,0,524288,655360,5242880},
	{0,1,1,0,0,0,1,0,98304,{2293760,2293760,2293760,2293760,2293760},{5898240,5898240,5898240,5898240,5898240},65536,0,0,65536,786432,0},
	{0,1,0,1,0,0,0,65536,13107,{655360,655360,655360,655360,655360},{13107200,13107200,13107200,13107200,13107200},13107,0,0,0,655360,0},
	{0,1,1,0,0,0,1,0,32768,{983040,983040,1310720,1966080,3211264},{3932160,3932160,3932160,3932160,5242880},65536,0,0,262144,1310720,2621440},
	{0,1,1,0,0,0,0,0,32768,{655360,983040,1310720,1966080,3276800},{2621440,3276800,3932160,4587520,6225920},65536,0,0,262144,655360,2621440},
	{0,1,0,1,0,0,0,65536,13107,{786432,786432,786432,786432,786432},{13107200,13107200,13107200,13107200,13107200},13107,0,0,0,655360,0},
	{0,1,0,1,0,0,0,32768,16384,{131072,196608,196608,262144,393216},{1966080,2621440,3276800,3932160,5242880},6553,0,0,65536,655360,0},
	{0,1,0,1,0,0,0,32768,16384,{131072,196608,196608,262144,393216},{1966080,2621440,3276800,3932160,5242880},6553,0,0,65536,655360,0},
	{0,1,0,1,0,0,0,39321,9830,{327680,393216,458752,524288,655360},{5242880,5898240,6553600,7864320,9830400},655,0,0,0,655360,0},
	{0,1,0,1,0,0,0,32768,16384,{327680,393216,524288,786432,1179648},{983040,1966080,2949120,3932160,5242880},131072,0,0,65536,655360,0},
	{0,1,1,0,0,0,1,0,32768,{5242880,5242880,5242880,7864320,9830400},{5570560,5570560,5570560,5570560,5570560},65536,0,0,524288,655360,5242880},
	{0,1,1,0,0,0,1,0,98304,{720896,786432,851968,917504,983040},{5898240,5898240,5898240,5898240,5898240},65536,0,0,65536,524288,0}
	};

// autoselect ordering

ubyte nWeaponOrder [2][11]= {{9,8,7,6,5,4,3,2,1,0,255},{9,8,4,3,1,5,0,255,7,6,2}};
ubyte nDefaultWeaponOrder [2][11]= {{9,8,7,6,5,4,3,2,1,0,255},{9,8,4,3,1,5,0,255,7,6,2}};

// bCycling weapon key pressed?

ubyte bCycling = 0;

//allow tPlayer to reorder menus?

//char	*Primary_weapon_names [MAX_PRIMARY_WEAPONS] = {
//	"Laser Cannon",
//	"Vulcan Cannon",
//	"Spreadfire Cannon",
//	"Plasma Cannon",
//	"Fusion Cannon"
//};

//char	*pszSecondaryWeaponNames [MAX_SECONDARY_WEAPONS] = {
//	"Concussion Missile",
//	"Homing Missile",
//	"Proximity Bomb",
//	"Smart Missile",
//	"Mega Missile"
//};

//char	*pszShortPrimaryWeaponNames [MAX_PRIMARY_WEAPONS] = {
//	"Laser",
//	"Vulcan",
//	"Spread",
//	"Plasma",
//	"Fusion"
//};

//char	*pszShortSecondaryWeaponNames [MAX_SECONDARY_WEAPONS] = {
//	"Concsn\nMissile",
//	"Homing\nMissile",
//	"Proxim.\nBomb",
//	"Smart\nMissile",
//	"Mega\nMissile"
//};

sbyte   bIsEnergyWeapon [MAX_WEAPON_TYPES] = {
	1, 1, 1, 1, 1,
	1, 1, 1, 0, 1,
	1, 0, 1, 1, 1,
	0, 1, 0, 0, 1,
	1, 0, 0, 1, 1,
	1, 1, 1, 0, 1,
	1, 1, 0, 1, 1,
	1
};

// ; (0) Laser Level 1
// ; (1) Laser Level 2
// ; (2) Laser Level 3
// ; (3) Laser Level 4
// ; (4) Unknown Use
// ; (5) Josh Blobs
// ; (6) Unknown Use
// ; (7) Unknown Use
// ; (8) ---------- Concussion Missile ----------
// ; (9) ---------- Flare ----------
// ; (10) ---------- Blue laser that blue guy shoots -----------
// ; (11) ---------- Vulcan Cannon ----------
// ; (12) ---------- Spreadfire Cannon ----------
// ; (13) ---------- Plasma Cannon ----------
// ; (14) ---------- Fusion Cannon ----------
// ; (15) ---------- Homing Missile ----------
// ; (16) ---------- Proximity Bomb ----------
// ; (17) ---------- Smart Missile ----------
// ; (18) ---------- Mega Missile ----------
// ; (19) ---------- Children of the PLAYER'S Smart Missile ----------
// ; (20) ---------- Bad Guy Spreadfire Laser ----------
// ; (21) ---------- SuperMech Homing Missile ----------
// ; (22) ---------- Regular Mech's missile -----------
// ; (23) ---------- Silent Spreadfire Laser ----------
// ; (24) ---------- Red laser that baby spiders shoot -----------
// ; (25) ---------- Green laser that rifleman shoots -----------
// ; (26) ---------- Plasma gun that 'plasguy' fires ------------
// ; (27) ---------- Blobs fired by Red Spiders -----------
// ; (28) ---------- Final Boss's Mega Missile ----------
// ; (29) ---------- Children of the ROBOT'S Smart Missile ----------
// ; (30) Laser Level 5
// ; (31) Laser Level 6
// ; (32) ---------- Super Vulcan Cannon ----------
// ; (33) ---------- Super Spreadfire Cannon ----------
// ; (34) ---------- Super Plasma Cannon ----------
// ; (35) ---------- Super Fusion Cannon ----------

//	-----------------------------------------------------------------------------

int AllowedToFireLaser (void)
{
	float	s;

if (gameStates.app.bPlayerIsDead) {
	gameData.missiles.nGlobalFiringCount = 0;
	return 0;
	}
if (gameStates.app.bD2XLevel && (gameData.segs.segment2s [gameData.objs.console->nSegment].special == SEGMENT_IS_NODAMAGE))
	return 0;
//	Make sure enough time has elapsed to fire laser, but if it looks like it will
//	be a long while before laser can be fired, then there must be some mistake!
if (!IsMultiGame && ((s = gameStates.gameplay.slowmo [0].fSpeed) > 1)) {
	fix t = gameData.laser.xLastFiredTime + (fix) ((gameData.laser.xNextFireTime - gameData.laser.xLastFiredTime) * s);
	if ((t > gameData.time.xGame) && (t < gameData.time.xGame + 2 * F1_0 * s))
		return 0;
	}
else {
	if ((gameData.laser.xNextFireTime > gameData.time.xGame) &&  (gameData.laser.xNextFireTime < gameData.time.xGame + 2 * F1_0))
		return 0;
	}
return 1;
}

//------------------------------------------------------------------------------

fix	xNextFlareFireTime = 0;
#define	FLARE_BIG_DELAY	 (F1_0*2)

int AllowedToFireFlare (void)
{
if ((xNextFlareFireTime > gameData.time.xGame) &&
	 (xNextFlareFireTime < gameData.time.xGame + FLARE_BIG_DELAY))	//	In case time is bogus, never wait > 1 second.
		return 0;
if (LOCALPLAYER.energy >= WI_energy_usage (FLARE_ID))
	xNextFlareFireTime = gameData.time.xGame + (fix) (gameStates.gameplay.slowmo [0].fSpeed * F1_0 / 4);
else
	xNextFlareFireTime = gameData.time.xGame + (fix) (gameStates.gameplay.slowmo [0].fSpeed * FLARE_BIG_DELAY);
return 1;
}

//------------------------------------------------------------------------------

int AllowedToFireMissile (int nPlayer, int bCheckSegment)
{
	float	s;
	fix	t;

//	Make sure enough time has elapsed to fire missile, but if it looks like it will
//	be a long while before missile can be fired, then there must be some mistake!
if (gameStates.app.bD2XLevel && bCheckSegment && 
    (gameData.segs.segment2s [gameData.objs.console->nSegment].special == SEGMENT_IS_NODAMAGE))
	return 0;
if (!IsMultiGame && ((s = gameStates.gameplay.slowmo [0].fSpeed) > 1)) {
	t = gameData.missiles.xLastFiredTime + (fix) ((gameData.missiles.xNextFireTime - gameData.missiles.xLastFiredTime) * s);
	if ((t > gameData.time.xGame) && (t < gameData.time.xGame + 5 * F1_0 * s))
		return 0;
	}
else if (nPlayer < 0) {
	if ((gameData.missiles.xNextFireTime > gameData.time.xGame) && 
		 (gameData.missiles.xNextFireTime < gameData.time.xGame + 5 * F1_0))
		return 0;
	}
else {
	t = gameData.multiplayer.weaponStates [nPlayer].xMslFireTime;
	if ((t > gameData.time.xGame) && (t < gameData.time.xGame + 5 * F1_0))
		return 0;
	}
return 1;
}

//	------------------------------------------------------------------------------------
//	Return:
// Bits set:
//		HAS_WEAPON_FLAG
//		HAS_ENERGY_FLAG
//		HAS_AMMO_FLAG
// See weapon[HA] for bit values
int PlayerHasWeapon (int nWeapon, int bSecondary, int nPlayer, int bAll)
{
	int		returnValue = 0;
	int		nWeaponIndex;
	tPlayer	*playerP = gameData.multiplayer.players + ((nPlayer < 0) ? gameData.multiplayer.nLocalPlayer : nPlayer);

//	Hack! If energy goes negative, you can't fire a weapon that doesn't require energy.
//	But energy should not go negative (but it does), so find out why it does!
if (gameStates.app.bD1Mission && (nWeapon >= SUPER_WEAPON))
	return 0;
if (playerP->energy < 0)
	playerP->energy = 0;

if (!bSecondary) {
	nWeaponIndex = primaryWeaponToWeaponInfo [nWeapon];

	if (nWeapon == SUPER_LASER_INDEX) {
		if ((playerP->primaryWeaponFlags & (1 << LASER_INDEX)) &&
				(bAll || (playerP->laserLevel > MAX_LASER_LEVEL)))
			returnValue |= HAS_WEAPON_FLAG;
		}
	else if (nWeapon == LASER_INDEX) {
		if ((playerP->primaryWeaponFlags & (1 << LASER_INDEX)) &&
			 (bAll || (playerP->laserLevel <= MAX_LASER_LEVEL)))
			returnValue |= HAS_WEAPON_FLAG;
		}
	else if (nWeapon == SPREADFIRE_INDEX) {
		if ((playerP->primaryWeaponFlags & (1 << nWeapon)) &&
			 (bAll || !(extraGameInfo [0].bSmartWeaponSwitch && 
							(playerP->primaryWeaponFlags & (1 << HELIX_INDEX)))))
			returnValue |= HAS_WEAPON_FLAG;
		}
	else if (nWeapon == VULCAN_INDEX) {
		if ((playerP->primaryWeaponFlags & (1 << nWeapon)) &&
			(bAll || !(extraGameInfo [0].bSmartWeaponSwitch && 
						  (playerP->primaryWeaponFlags & (1 << GAUSS_INDEX)))))
			returnValue |= HAS_WEAPON_FLAG;
		}
	else {
		if (playerP->primaryWeaponFlags & (1 << nWeapon))
			returnValue |= HAS_WEAPON_FLAG;
		}

	// Special case: Gauss cannon uses vulcan ammo.	
	if (nWeapon == GAUSS_INDEX) {
		if (WI_ammo_usage (nWeaponIndex) <= playerP->primaryAmmo [VULCAN_INDEX])
			returnValue |= HAS_AMMO_FLAG;
		}
	else
		if (WI_ammo_usage (nWeaponIndex) <= playerP->primaryAmmo [nWeapon])
			returnValue |= HAS_AMMO_FLAG;
	if (nWeapon == OMEGA_INDEX) {	// Hack: Make sure tPlayer has energy to omega
		if (playerP->energy || gameData.omega.xCharge)
			returnValue |= HAS_ENERGY_FLAG;
		}
	else {
/*
		if (nWeapon == SUPER_LASER_INDEX) {
			if (playerP->energy || gameData.omega.xCharge)
				returnValue |= HAS_ENERGY_FLAG;
		}
*/
		if (WI_energy_usage (nWeaponIndex) <= playerP->energy)
			returnValue |= HAS_ENERGY_FLAG;
		}
	}
else {
	nWeaponIndex = secondaryWeaponToWeaponInfo [nWeapon];

	if (playerP->secondaryWeaponFlags & (1 << nWeapon))
		returnValue |= HAS_WEAPON_FLAG;

	if (WI_ammo_usage (nWeaponIndex) <= playerP->secondaryAmmo [nWeapon])
		returnValue |= HAS_AMMO_FLAG;

	if (WI_energy_usage(nWeaponIndex) <= playerP->energy)
		returnValue |= HAS_ENERGY_FLAG;
}

return returnValue;
}

//	------------------------------------------------------------------------------------

void InitWeaponOrdering (void)
 {
  // short routine to setup default weapon priorities for new pilots

  int i;

for (i = 0; i < MAX_PRIMARY_WEAPONS + 1; i++)
	primaryOrder [i] = defaultPrimaryOrder [i];
for (i = 0; i < MAX_SECONDARY_WEAPONS + 1; i++)
	secondaryOrder [i] = defaultSecondaryOrder [i];
 }

//	------------------------------------------------------------------------------------

void CyclePrimary (void)
{
bCycling = 1;
AutoSelectWeapon (0, 0);
bCycling = 0;
}

//	------------------------------------------------------------------------------------

void CycleSecondary (void)
{
bCycling = 1;
AutoSelectWeapon (1, 0);
bCycling = 0;
}


//	------------------------------------------------------------------------------------
//if message flag set, print message saying selected
void SelectWeapon (int nWeaponNum, int bSecondary, int bPrintMessage, int bWaitForRearm)
{
	const char	*szWeaponName;

if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordPlayerWeapon (bSecondary, nWeaponNum);
if (!bSecondary) {
	if (gameData.weapons.nPrimary != nWeaponNum) {
		if (bWaitForRearm) 
			DigiPlaySampleOnce (SOUND_GOOD_SELECTION_PRIMARY, F1_0);
		if (IsMultiGame)	{
			if (bWaitForRearm) 
				MultiSendPlaySound (SOUND_GOOD_SELECTION_PRIMARY, F1_0);
			}
		gameData.laser.xNextFireTime = bWaitForRearm ? gameData.time.xGame + (fix) (gameStates.gameplay.slowmo [1].fSpeed * REARM_TIME) : 0;
		gameData.laser.nGlobalFiringCount = 0;
		} 
	else {
		// Select super version if available.
		if (bWaitForRearm) {
			if (!bCycling)
				; // -- MK, only plays when can't fire weapon anyway, fixes bug -- DigiPlaySampleOnce(SOUND_ALREADY_SELECTED, F1_0);
			else
				DigiPlaySampleOnce(SOUND_BAD_SELECTION, F1_0);
			}
		}
	gameData.weapons.nOverridden = nWeaponNum;
	if (!bSecondary && extraGameInfo [IsMultiGame].bSmartWeaponSwitch && !gameStates.app.bD1Mission) {
		switch (nWeaponNum) {
			case 1:
				if (LOCALPLAYER.primaryWeaponFlags & (1 << 6))
					nWeaponNum = 6;
				break;
			case 2:
				if (LOCALPLAYER.primaryWeaponFlags & (1 << 7))
					nWeaponNum = 7;
				break;
			}
		}
	gameData.weapons.nPrimary = (!bSecondary && (nWeaponNum == SUPER_LASER_INDEX)) ? LASER_INDEX : nWeaponNum;
	StopPrimaryFire ();
	szWeaponName = PRIMARY_WEAPON_NAMES (nWeaponNum);
   #if defined (TACTILE)
 	TactileSetButtonJolt();
	#endif
	//save flag for whether was super version
	bLastPrimaryWasSuper [nWeaponNum % SUPER_WEAPON] = (nWeaponNum >= SUPER_WEAPON);
	}
else {
	if (gameData.weapons.nSecondary != nWeaponNum) {
		if (bWaitForRearm) 
			DigiPlaySampleOnce (SOUND_GOOD_SELECTION_SECONDARY, F1_0);
		if (IsMultiGame) {
			if (bWaitForRearm) 
				MultiSendPlaySound (SOUND_GOOD_SELECTION_PRIMARY, F1_0);
			}
		gameData.missiles.xNextFireTime = bWaitForRearm ? gameData.time.xGame + REARM_TIME : 0;
		gameData.missiles.nGlobalFiringCount = 0;
		}
	else {
		if (bWaitForRearm) {
		 if (!bCycling)
			DigiPlaySampleOnce (SOUND_ALREADY_SELECTED, F1_0);
		 else
			DigiPlaySampleOnce (SOUND_BAD_SELECTION, F1_0);
		}
	}
	//if (nWeaponNum % SUPER_WEAPON != PROXMINE_INDEX)
		gameData.weapons.nSecondary = nWeaponNum;
	szWeaponName = SECONDARY_WEAPON_NAMES (nWeaponNum);
	//save flag for whether was super version
	bLastSecondaryWasSuper [nWeaponNum % SUPER_WEAPON] = (nWeaponNum >= SUPER_WEAPON);
	}

if (bPrintMessage) {
	if (nWeaponNum == LASER_INDEX && !bSecondary)
		HUDInitMessage (TXT_WPN_LEVEL, szWeaponName, LOCALPLAYER.laserLevel + 1, TXT_SELECTED);
	else
		HUDInitMessage ("%s %s", szWeaponName, TXT_SELECTED);
	}
}

//------------------------------------------------------------------------------

void ToggleBomb (void)
{
int bomb = bLastSecondaryWasSuper [PROXMINE_INDEX] ? PROXMINE_INDEX : SMARTMINE_INDEX;
if ((gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)) ||
	 !(LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX] || LOCALPLAYER.secondaryAmmo [SMARTMINE_INDEX])) {
	DigiPlaySampleOnce (SOUND_BAD_SELECTION, F1_0);
	HUDInitMessage (TXT_NOBOMBS);
	}
else if (!LOCALPLAYER.secondaryAmmo [bomb]) {
	DigiPlaySampleOnce (SOUND_BAD_SELECTION, F1_0);
	HUDInitMessage (TXT_NOBOMB_ANY, (bomb == SMARTMINE_INDEX)? TXT_SMART_MINES : 
						 !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0) ? TXT_SMOKE_GRENADES : TXT_PROX_BOMBS);
	}
else {
	bLastSecondaryWasSuper [PROXMINE_INDEX] = !bLastSecondaryWasSuper [PROXMINE_INDEX];
	DigiPlaySampleOnce (SOUND_GOOD_SELECTION_SECONDARY, F1_0);
	}
}

//flags whether the last time we use this weapon, it was the 'super' version
//	------------------------------------------------------------------------------------
//	Select a weapon, primary or secondary.
void DoSelectWeapon (int nWeapon, int bSecondary)
{
	int	nWeaponSave = nWeapon;
	int	nWeaponStatus, nCurrent, hasFlag;
	ubyte	bLastWasSuper;

if (!bSecondary) {
	nCurrent = gameData.weapons.nPrimary;
	if ((nCurrent == LASER_INDEX) && (LOCALPLAYER.laserLevel > MAX_LASER_LEVEL))
		nCurrent = SUPER_LASER_INDEX;
	bLastWasSuper = bLastPrimaryWasSuper [nWeapon];
	if ((nWeapon == LASER_INDEX) && (LOCALPLAYER.laserLevel > MAX_LASER_LEVEL))
		nWeapon = SUPER_LASER_INDEX;
	hasFlag = HAS_WEAPON_FLAG;
	LOCALPLAYER.energy += gameData.fusion.xCharge;
	gameData.fusion.xCharge = 0;
	}
else if (nWeapon == 2) {
	ToggleBomb ();
	return;
	}
else {
	nCurrent = gameData.weapons.nSecondary;
	bLastWasSuper = bLastSecondaryWasSuper [nWeapon % SUPER_WEAPON];
	hasFlag = HAS_WEAPON_FLAG + HAS_AMMO_FLAG;
	}

if ((nCurrent == nWeapon) || (nCurrent == nWeapon + SUPER_WEAPON)) {
	//already have this selected, so toggle to other of Normal/super version
	if (!bSecondary && (nCurrent == SUPER_LASER_INDEX))
		return;
	nWeapon %= SUPER_WEAPON;
	if (!bLastWasSuper)
		nWeapon += SUPER_WEAPON;
	nWeaponStatus = PlayerHasWeapon (nWeapon, bSecondary, -1, 0);
	}
else {
	//go to last-select version of requested missile
	if (bLastWasSuper && (nWeapon < SUPER_WEAPON))
		nWeapon += SUPER_WEAPON;
	nWeaponStatus = PlayerHasWeapon (nWeapon, bSecondary, -1, 0);
	//if don't have last-selected, try other version
	if ((nWeaponStatus & hasFlag) != hasFlag) {
		nWeapon = 2 * nWeaponSave + SUPER_WEAPON - nWeapon;
		nWeaponStatus = PlayerHasWeapon (nWeapon, bSecondary, -1, 0);
		if ((nWeaponStatus & hasFlag) != hasFlag)
			nWeapon = 2 * nWeaponSave + SUPER_WEAPON - nWeapon;
		}
	}

//if we don't have the weapon we're switching to, give error & bail
if ((nWeaponStatus & hasFlag) != hasFlag) {
	if (!bSecondary) {
		if (nWeapon == SUPER_LASER_INDEX)
			return; 		//no such thing as super laser, so no error
		HUDInitMessage ("%s %s!", TXT_DONT_HAVE, PRIMARY_WEAPON_NAMES (nWeapon));
		}
	else
		HUDInitMessage ("%s %s%s",TXT_HAVE_NO, SECONDARY_WEAPON_NAMES (nWeapon), TXT_SX);
	DigiPlaySample (SOUND_BAD_SELECTION, F1_0);
	return;
	}
//now actually select the weapon
SelectWeapon (nWeapon, bSecondary, 1, 1);
}

//	----------------------------------------------------------------------------------------

inline int WeaponId (int w)
{
if (w != LASER_INDEX)
	return w;
if ((LOCALPLAYER.laserLevel <= MAX_LASER_LEVEL) || !bLastPrimaryWasSuper [LASER_INDEX])
	return LASER_INDEX;
return SUPER_LASER_INDEX;
}

//	----------------------------------------------------------------------------------------

void SetLastSuperWeaponStates (void)
{
	tPlayer	*playerP = gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer;
	int		i, j;

for (i = 0, j = 1 << 5; i < 5; i++, j <<= 1) {
	bLastPrimaryWasSuper [i] = i ? ((playerP->primaryWeaponFlags & j) != 0) : (playerP->laserLevel > MAX_LASER_LEVEL);
	bLastSecondaryWasSuper [i] = (playerP->secondaryWeaponFlags & j) != 0;
	}
}

//	----------------------------------------------------------------------------------------
//	Automatically select next best weapon if unable to fire current weapon.
// Weapon nType: 0==primary, 1==secondary

void AutoSelectWeapon (int nWeaponType, int bAutoSelect)
{
	int	r;
	int	nCutPoint;
	int	bLooped = 0;

if (bAutoSelect && !gameOpts->gameplay.nAutoSelectWeapon)
	return;
if (!nWeaponType) {
	r = PlayerHasWeapon (WeaponId (gameData.weapons.nPrimary), 0, -1, 0);
	if ((r != HAS_ALL) || bCycling) {
		int	bTryAgain = 1;
		int	iCurWeapon = POrderList (WeaponId (gameData.weapons.nOverridden));
		int	iNewWeapon = iCurWeapon;
		int	nCurWeapon, nNewWeapon;

		nCurWeapon = primaryOrder [iCurWeapon];
		nCutPoint = POrderList (255);
		while (bTryAgain) {
			if (++iNewWeapon >= nCutPoint) {
				if (bLooped) {
					if (bCycling) 
						SelectWeapon (gameData.weapons.nPrimary, 0, 0, 1);
					else {
						HUDInitMessage (TXT_NO_PRIMARY);
#ifdef TACTILE
						if (TactileStick)
						ButtonReflexClear(0);
#endif
						SelectWeapon (0, 0, 0, 1);
						}
					return;
					}
				iNewWeapon = 0;
				bLooped = 1;
				}

			if (iNewWeapon == MAX_PRIMARY_WEAPONS)
				iNewWeapon = 0;

//	Hack alert!  Because the fusion uses 0 energy at the end (it's got the weird chargeup)
//	it looks like it takes 0 to fire, but it doesn't, so never auto-select.
// if (primaryOrder [iNewWeapon] == FUSION_INDEX)
//	continue;

			nNewWeapon = primaryOrder [iNewWeapon];
			if (nNewWeapon == gameData.weapons.nPrimary) {
				if ((nCurWeapon == SUPER_LASER_INDEX) && (nNewWeapon == LASER_INDEX))
					continue;
				else if (bCycling)
					SelectWeapon (gameData.weapons.nPrimary, 0, 0, 1);
				else {
					HUDInitMessage (TXT_NO_PRIMARY);
#ifdef TACTILE
					if (TactileStick)
					ButtonReflexClear (0);
#endif
					//	if (POrderList(0)<POrderList(255))
					SelectWeapon(0, 0, 0, 1);
					}
				return;			// Tried all weapons!
				} 
			else {
				if ((nNewWeapon != 255) && (PlayerHasWeapon (nNewWeapon, 0, -1, 0) == HAS_ALL)) {
					SelectWeapon ((nNewWeapon == SUPER_LASER_INDEX) ? LASER_INDEX : nNewWeapon, 0, 1, 1);
					bLastPrimaryWasSuper [nNewWeapon % SUPER_WEAPON] = (nNewWeapon >= SUPER_WEAPON);
					return;
					}
				}
			}
		}
	} 
else {
	Assert(nWeaponType==1);
	r = PlayerHasWeapon (gameData.weapons.nSecondary, 1, -1, 0);
	if (r != HAS_ALL || bCycling) {
		int	bTryAgain = 1;
		int	iCurWeapon = SOrderList (gameData.weapons.nSecondary);

		nCutPoint = SOrderList (255);
		while (bTryAgain) {
			if (++iCurWeapon >= nCutPoint) {
				if (bLooped) {
					if (bCycling)
						SelectWeapon (gameData.weapons.nSecondary, 1, 0, 1);
					else
						HUDInitMessage (TXT_NO_SECSELECT);
					return;
					}
				iCurWeapon = 0;
				bLooped = 1;
				}
			if (iCurWeapon == MAX_SECONDARY_WEAPONS)
				iCurWeapon = 0;
			if (secondaryOrder [iCurWeapon] == gameData.weapons.nSecondary) {
				if (bCycling)
					SelectWeapon (gameData.weapons.nSecondary, 1, 0, 1);
				else
					HUDInitMessage (TXT_NO_SECAVAIL);
				return;				// Tried all weapons!
				}
			else if (PlayerHasWeapon (secondaryOrder [iCurWeapon], 1, -1, 0) == HAS_ALL) {
				SelectWeapon (secondaryOrder [iCurWeapon], 1, 1, 1);
				bTryAgain = 0;
				}
			}
		}
	}
}

#ifdef _DEBUG

//	----------------------------------------------------------------------------------------
//	Show tPlayer which weapons he has, how much ammo...
//	Looks like a debug screen now because it writes to mono screen, but that will change...
void ShowWeaponStatus (void)
{
	int	i;
#if TRACE
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++) {
	if (LOCALPLAYER.primaryWeaponFlags & (1 << i))
		con_printf (CONDBG, "HAVE");
	else
		con_printf (CONDBG, "    ");
	con_printf (CONDBG, 
		"  Weapon: %20s, charges: %4i\n", 
		PRIMARY_WEAPON_NAMES(i), LOCALPLAYER.primaryAmmo [i]);
	}
	con_printf (CONDBG, "\n");
	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++) {
		if (LOCALPLAYER.secondaryWeaponFlags & (1 << i))
			con_printf (CONDBG, "HAVE");
		else
			con_printf (CONDBG, "    ");
		con_printf (CONDBG, 
			"  Weapon: %20s, charges: %4i\n", 
			SECONDARY_WEAPON_NAMES(i), LOCALPLAYER.secondaryAmmo [i]);
	}
con_printf (CONDBG, "\n");
con_printf (CONDBG, "\n");
#endif
}

#endif

//	-----------------------------------------------------------------------------

//	Call this once/frame to process all super mines in the level.
void ProcessSmartMinesFrame (void)
{
	int			i, j;
	int			nStart, nStep, nParentObj;
	fix			dist;
	tObject		*objPi, *objPj;
	vmsVector	*vBombPos;

	//	If we don't know of there being any super mines in the level, just
	//	check every 8th tObject each frame.
if (gameStates.gameplay.bHaveSmartMines == 0) {
	nStart = gameData.app.nFrameCount & 7;
	nStep = 8;
	} 
else {
	nStart = 0;
	nStep = 1;
	}
gameStates.gameplay.bHaveSmartMines = 0;

for (i = nStart; i <= gameData.objs.nLastObject [0]; i += nStep) {
	objPi = OBJECTS + i;
	if ((objPi->nType != OBJ_WEAPON) || (objPi->id != SMARTMINE_ID))
		continue;
	nParentObj = objPi->cType.laserInfo.nParentObj;
	gameStates.gameplay.bHaveSmartMines = 1;
	if (objPi->lifeleft + F1_0*2 >= gameData.weapons.info [SMARTMINE_ID].lifetime)
		continue;
	vBombPos = &objPi->position.vPos;
	for (j = 0, objPj = OBJECTS; j <= gameData.objs.nLastObject [0]; j++, objPj++) {
		if (j == nParentObj) 
			continue;
		if ((objPj->nType != OBJ_PLAYER) && (objPj->nType != OBJ_ROBOT))
			continue;
		dist = vmsVector::Dist(*vBombPos, objPj->position.vPos);
		if (dist - objPj->size >= F1_0*20)
			continue;
		if (objPi->nSegment == objPj->nSegment)
			objPi->lifeleft = 1;
		else {
			//	Object which is close enough to detonate smart mine is not in same tSegment as smart mine.
			//	Need to do a more expensive check to make sure there isn't an obstruction.
			if (((gameData.app.nFrameCount ^ (i+j)) % 4) == 0) {
				tFVIQuery	fq;
				tFVIData		hit_data;
				int			fate;

				fq.startSeg = objPi->nSegment;
				fq.p0	= &objPi->position.vPos;
				fq.p1 = &objPj->position.vPos;
				fq.radP0 =
				fq.radP1 = 0;
				fq.thisObjNum = i;
				fq.ignoreObjList = NULL;
				fq.flags	= 0;

				fate = FindVectorIntersection(&fq, &hit_data);
				if (fate != HIT_WALL)
					objPi->lifeleft = 1;
				}
			}
		}
	}
}

//------------------------------------------------------------------------------
//returns which bomb will be dropped next time the bomb key is pressed
int ArmedBomb (void)
{
	int bomb, otherBomb;

	//use the last one selected, unless there aren't any, in which case use
	//the other if there are any
   // If hoard game, only let the tPlayer drop smart mines
if (gameData.app.nGameMode & GM_ENTROPY)
   return PROXMINE_INDEX; //allow for dropping orbs
if (gameData.app.nGameMode & GM_HOARD)
	return SMARTMINE_INDEX;

bomb = bLastSecondaryWasSuper [PROXMINE_INDEX] ? SMARTMINE_INDEX : PROXMINE_INDEX;
otherBomb = SMARTMINE_INDEX + PROXMINE_INDEX - bomb;

if (!LOCALPLAYER.secondaryAmmo [bomb] && LOCALPLAYER.secondaryAmmo [otherBomb]) {
	bomb = otherBomb;
	bLastSecondaryWasSuper [bomb % SUPER_WEAPON] = (bomb == SMARTMINE_INDEX);
	}
return bomb;
}

//------------------------------------------------------------------------------

void DoWeaponStuff (void)
{
  int i;

if (Controls [0].useCloakDownCount)
	ApplyCloak (0, -1);
if (Controls [0].useInvulDownCount)
	ApplyInvul (0, -1);
if (Controls [0].fireFlareDownCount)
	if (AllowedToFireFlare ())
		CreateFlare(gameData.objs.console);
if (AllowedToFireMissile (-1, 1)) {
	i = secondaryWeaponToWeaponInfo [gameData.weapons.nSecondary];
	gameData.missiles.nGlobalFiringCount += WI_fireCount (i) * (Controls [0].fireSecondaryState || Controls [0].fireSecondaryDownCount);
	}
if (gameData.missiles.nGlobalFiringCount) {
	DoMissileFiring (1);			//always enable autoselect for Normal missile firing
	gameData.missiles.nGlobalFiringCount--;
	}
if (Controls [0].cyclePrimaryCount) {
	for (i = 0; i < Controls [0].cyclePrimaryCount; i++)
	CyclePrimary ();
	}
if (Controls [0].cycleSecondaryCount) {
	for (i = 0; i < Controls [0].cycleSecondaryCount; i++)
	CycleSecondary ();
	}
if (Controls [0].headlightCount) {
	for (i = 0; i < Controls [0].headlightCount; i++)
	ToggleHeadlight ();
	}
if (gameData.missiles.nGlobalFiringCount < 0)
	gameData.missiles.nGlobalFiringCount = 0;
//	Drop proximity bombs.
if (Controls [0].dropBombDownCount) {
	if (gameStates.app.bD2XLevel && (gameData.segs.segment2s [gameData.objs.console->nSegment].special == SEGMENT_IS_NODAMAGE))
		Controls [0].dropBombDownCount = 0;
	else {
		int ssw_save = gameData.weapons.nSecondary;
		while (Controls [0].dropBombDownCount--) {
			int ssw_save2 = gameData.weapons.nSecondary = ArmedBomb();
			if (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY))
				DropSecondaryWeapon (-1);
			else
				DoMissileFiring (gameData.weapons.nSecondary == ssw_save);	//only allow autoselect if bomb is actually selected
			if (gameData.weapons.nSecondary != ssw_save2 && ssw_save == ssw_save2)
				ssw_save = gameData.weapons.nSecondary;    //if bomb was selected, and we ran out & autoselect, then stick with new selection
			}
		gameData.weapons.nSecondary = ssw_save;
	}
}
}

//	-----------------------------------------------------------------------------

int tactile_fire_duration []={120,80,150,250,150,200,100,180,280,100};
int tactile_fire_repeat []={260,90,160,160,160,210,110,191,291,111};

void TactileSetButtonJolt ()
 {
  #ifdef TACTILE

  FILE *infile;
  int t,i;
  static int stickmag=-1;
  int dur,rep;

  dur=tactile_fire_duration [gameData.weapons.nPrimary];
  rep=tactile_fire_repeat [gameData.weapons.nPrimary];

if (TactileStick) {
	if (stickmag==-1) {
		if (t=FindArg("-stickmag"))
			stickmag=atoi (pszArgList [t+1]);
		else
			stickmag=50;
		infile=(FILE *)fopen ("stick.val","rt");
		if (infile!=NULL) {
			for (i=0;i<10;i++) {
				fscanf (infile,"%d %d\n",&tactile_fire_duration [i],&tactile_fire_repeat [i]);
				}
			fclose (infile);
			}
		}
	ButtonReflexJolt (0,stickmag,0,dur,rep);
	}
#endif
}

//	-----------------------------------------------------------------------------

/*
 * reads n tWeaponInfo structs from a CFILE
 */
int WeaponInfoReadN (tWeaponInfo *pwi, int n, CFILE *fp, int fileVersion)
{
	int i, j;

for (i = 0; i < n; i++, pwi++) {
	gameData.weapons.color [i].red =
	gameData.weapons.color [i].green =
	gameData.weapons.color [i].blue = 1.0;
	pwi->renderType = CFReadByte (fp);
	pwi->persistent = CFReadByte (fp);
	pwi->nModel = CFReadShort (fp);
	pwi->nInnerModel = CFReadShort (fp);
	pwi->nFlashVClip = CFReadByte (fp);
	pwi->robot_hit_vclip = CFReadByte (fp);
	pwi->flashSound = CFReadShort (fp);
	pwi->wall_hit_vclip = CFReadByte (fp);
	pwi->fireCount = CFReadByte (fp);
	pwi->robot_hitSound = CFReadShort (fp);
	pwi->ammo_usage = CFReadByte (fp);
	pwi->nVClipIndex = CFReadByte (fp);
	pwi->wall_hitSound = CFReadShort (fp);
	pwi->destroyable = CFReadByte (fp);
	pwi->matter = CFReadByte (fp);
	pwi->bounce = CFReadByte (fp);
	pwi->homingFlag = CFReadByte (fp);
	pwi->speedvar = CFReadByte (fp);
	pwi->flags = CFReadByte (fp);
	pwi->flash = CFReadByte (fp);
	pwi->afterburner_size = CFReadByte (fp);

	if (fileVersion >= 3)
		pwi->children = CFReadByte (fp);
	else
		// Set the nType of children correctly when using old datafiles.  
		// In earlier descent versions this was simply hard-coded in CreateSmartChildren ().
		switch (i) {
			case SMARTMSL_ID:
				pwi->children = SMARTMSL_BLOB_ID;
				break;
			case SMARTMINE_ID:
				pwi->children = SMARTMINE_BLOB_ID;
				break;
#if 1 /* not present in shareware */
			case ROBOT_SMARTMINE_ID:
				pwi->children = ROBOT_SMARTMINE_BLOB_ID;
				break;
			case EARTHSHAKER_ID:
				pwi->children = EARTHSHAKER_MEGA_ID;
				break;
#endif
			default:
				pwi->children = -1;
				break;
			}
	pwi->energy_usage = CFReadFix (fp);
	pwi->fire_wait = CFReadFix (fp);
	if (fileVersion >= 3)
		pwi->multi_damage_scale = CFReadFix (fp);
	else /* FIXME: hack this to set the real values */
		pwi->multi_damage_scale = F1_0;
	BitmapIndexRead (&pwi->bitmap, fp);
	pwi->blob_size = CFReadFix (fp);
	pwi->xFlashSize = CFReadFix (fp);
	pwi->impact_size = CFReadFix (fp);
	for (j = 0; j < NDL; j++)
		pwi->strength [j] = CFReadFix (fp);
	for (j = 0; j < NDL; j++)
		pwi->speed [j] = CFReadFix (fp);
	pwi->mass = CFReadFix (fp);
	pwi->drag = CFReadFix (fp);
	pwi->thrust = CFReadFix (fp);
	pwi->po_len_to_width_ratio = CFReadFix (fp);
	if (gameData.objs.bIsMissile [i])
		pwi->po_len_to_width_ratio = F1_0 * 10;
	pwi->light = CFReadFix (fp);
	if (i == SPREADFIRE_ID)
		pwi->light = F1_0;
	else if (i == HELIX_ID)
		pwi->light = 3 * F1_0 / 2;
	pwi->lifetime = CFReadFix (fp);
	pwi->damage_radius = CFReadFix (fp);
	BitmapIndexRead (&pwi->picture, fp);
	if (fileVersion >= 3)
		BitmapIndexRead (&pwi->hires_picture, fp);
	else
		pwi->hires_picture.index = pwi->picture.index;
	}
return i;
}

//	-----------------------------------------------------------------------------
//eof
