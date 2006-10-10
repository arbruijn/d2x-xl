/* $Id: weapon.c,v 1.9 2003/10/11 09:28:38 btb Exp $ */
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
 * Functions for weapons...
 *
 * Old Log:
 * Revision 1.2  1995/10/31  10:17:39  allender
 * new shareware stuff
 *
 * Revision 1.1  1995/05/16  15:32:16  allender
 * Initial revision
 *
 * Revision 2.1  1995/03/21  14:38:43  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.0  1995/02/27  11:27:25  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.54  1995/02/15  15:21:48  mike
 * make smart missile select if mega missiles used up.
 *
 *
 * Revision 1.53  1995/02/12  02:12:30  john
 * Fixed bug with state restore making weapon beeps.
 *
 * Revision 1.52  1995/02/09  20:42:15  mike
 * change weapon autoselect, always autoselect smart, mega.
 *
 * Revision 1.51  1995/02/07  20:44:26  mike
 * autoselect mega, smart when you pick them up.
 *
 * Revision 1.50  1995/02/07  13:32:25  rob
 * Added include of multi.h
 *
 * Revision 1.49  1995/02/07  13:21:33  yuan
 * Fixed 2nd typo
 *
 * Revision 1.48  1995/02/07  13:16:39  yuan
 * Fixed typo.
 *
 * Revision 1.47  1995/02/07  12:53:12  rob
 * Added network sound prop. to weapon switch.
 *
 * Revision 1.46  1995/02/06  15:53:17  mike
 * don't autoselect smart or mega missile when you pick it up.
 *
 * Revision 1.45  1995/02/02  21:43:34  mike
 * make autoselection better.
 *
 * Revision 1.44  1995/02/02  16:27:21  mike
 * make concussion missiles trade up.
 *
 * Revision 1.43  1995/02/01  23:34:57  adam
 * messed with weapon change sounds
 *
 * Revision 1.42  1995/02/01  17:12:47  mike
 * Make smart missile, mega missile not auto-select.
 *
 * Revision 1.41  1995/02/01  15:50:54  mike
 * fix bogus weapon selection sound code.
 *
 * Revision 1.40  1995/01/31  16:16:31  mike
 * Separate smart blobs for robot and player.
 *
 * Revision 1.39  1995/01/30  21:12:11  mike
 * Use new weapon selection sounds, different for primary and secondary.
 *
 * Revision 1.38  1995/01/29  13:46:52  mike
 * Don't auto-select fusion cannon when you run out of energy.
 *
 * Revision 1.37  1995/01/20  11:11:13  allender
 * record weapon changes again.  (John somehow lost my 1.35 changes).
 *
 * Revision 1.36  1995/01/19  17:00:46  john
 * Made save game work between levels.
 *
 * Revision 1.34  1995/01/09  17:03:48  mike
 * fix autoselection of weapons.
 *
 * Revision 1.33  1995/01/05  15:46:31  john
 * Made weapons not rearm when starting a saved game.
 *
 * Revision 1.32  1995/01/03  12:34:23  mike
 * autoselect next lower weapon if run out of smart or mega missile.
 *
 * Revision 1.31  1994/12/12  21:39:37  matt
 * Changed vulcan ammo: 10K max, 5K w/weapon, 1250 per powerup
 *
 * Revision 1.30  1994/12/09  19:55:04  matt
 * Added weapon name in "not available in shareware" message
 *
 * Revision 1.29  1994/12/06  13:50:24  adam
 * added shareware msg. when choosing 4 top weapons
 *
 * Revision 1.28  1994/12/02  22:07:13  mike
 * if you gots 19 concussion missiles and you runs over 4, say you picks up 1, not 4, we do the math, see?
 *
 * Revision 1.27  1994/12/02  20:06:24  matt
 * Made vulcan ammo print at approx 25 times actual
 *
 * Revision 1.26  1994/12/02  15:05:03  matt
 * Fixed bogus weapon constants and arrays
 *
 * Revision 1.25  1994/12/02  10:50:34  yuan
 * Localization
 *
 * Revision 1.24  1994/11/29  15:48:28  matt
 * selecting weapon now makes sound
 *
 * Revision 1.23  1994/11/28  11:26:58  matt
 * Cleaned up hud message printing for picking up weapons
 *
 * Revision 1.22  1994/11/27  23:13:39  matt
 * Made changes for new con_printf calling convention
 *
 * Revision 1.21  1994/11/12  16:38:34  mike
 * clean up default ammo stuff.
 *
 * Revision 1.20  1994/11/07  17:41:18  mike
 * messages for when you try to fire a weapon you don't have or don't have ammo for.
 *
 * Revision 1.19  1994/10/21  20:40:05  mike
 * fix double vulcan ammo.
 *
 * Revision 1.18  1994/10/20  09:49:05  mike
 * kill messages no one liked...*sniff* *sniff*
 *
 * Revision 1.17  1994/10/19  11:17:07  mike
 * Limit amount of player ammo.
 *
 * Revision 1.16  1994/10/12  08:04:18  mike
 * Fix proximity/homing confusion.
 *
 * Revision 1.15  1994/10/11  18:27:58  matt
 * Changed auto selection of secondary weapons
 *
 * Revision 1.14  1994/10/08  23:37:54  matt
 * Don't pick up weapons you already have; also fixed bAutoSelect bug
 * for seconary weapons
 *
 * Revision 1.13  1994/10/08  14:55:47  matt
 * Fixed bug that selected vulcan cannon when picked up ammo, even though
 * you didn't have the weapon.
 *
 * Revision 1.12  1994/10/08  12:50:32  matt
 * Fixed bug that let you select weapons you don't have
 *
 * Revision 1.11  1994/10/07  23:37:56  matt
 * Made weapons select when pick up better one
 *
 * Revision 1.10  1994/10/07  16:02:08  matt
 * Fixed problem with weapon auto-select
 *
 * Revision 1.9  1994/10/05  17:00:20  matt
 * Made PlayerHasWeapon() public and moved constants to header file
 *
 * Revision 1.8  1994/09/26  11:27:13  mike
 * Fix auto selection of weapon when you run out of ammo.
 *
 * Revision 1.7  1994/09/13  16:40:45  mike
 * Add rearm delay and missile firing delay.
 *
 * Revision 1.6  1994/09/13  14:43:12  matt
 * Added cockpit weapon displays
 *
 * Revision 1.5  1994/09/03  15:23:06  mike
 * Auto select next weaker weapon when one runs out, clean up code.
 *
 * Revision 1.4  1994/09/02  16:38:19  mike
 * Eliminate a pile of arrays, associate weapon data with gameData.weapons.info.
 *
 * Revision 1.3  1994/09/02  11:57:10  mike
 * Add a bunch of stuff, I forget what.
 *
 * Revision 1.2  1994/06/03  16:26:32  john
 * Initial version.
 *
 * Revision 1.1  1994/06/03  14:40:43  john
 * Initial revision
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: weapon.c,v 1.9 2003/10/11 09:28:38 btb Exp $";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "inferno.h"
#include "game.h"
#include "laser.h"
#include "weapon.h"
#include "mono.h"
#include "player.h"
#include "gauges.h"
#include "error.h"
#include "sounds.h"
#include "text.h"
#include "powerup.h"
#include "fireball.h"
#include "newdemo.h"
#include "multi.h"
#include "newmenu.h"
#include "ai.h"
#include "args.h"
#include "escort.h"
#include "network.h"
#include "u_mem.h"

#if defined (TACTILE)
#include "tactile.h"
#endif

int POrderList (int num);
int SOrderList (int num);
//	Note, only Vulcan cannon requires ammo.
// NOTE: Now Vulcan and Gauss require ammo. -5/3/95 Yuan
//ubyte	Default_primary_ammo_level[MAX_PRIMARY_WEAPONS] = {255, 0, 255, 255, 255};
//ubyte	Default_secondary_ammo_level[MAX_SECONDARY_WEAPONS] = {3, 0, 0, 0, 0};

//	Convert primary weapons to indices in gameData.weapons.info array.
ubyte primaryWeaponToWeaponInfo[MAX_PRIMARY_WEAPONS] = {LASER_ID, VULCAN_ID, SPREADFIRE_ID, PLASMA_ID, FUSION_ID, SUPER_LASER_ID, GAUSS_ID, HELIX_ID, PHOENIX_ID, OMEGA_ID};
ubyte secondaryWeaponToWeaponInfo[MAX_SECONDARY_WEAPONS] = {CONCUSSION_ID, HOMING_ID, PROXIMITY_ID, SMART_ID, MEGA_ID, FLASH_ID, GUIDEDMISS_ID, SUPERPROX_ID, MERCURY_ID, EARTHSHAKER_ID};

//for each Secondary weapon, which gun it fires out of
ubyte secondaryWeaponToGunNum[MAX_SECONDARY_WEAPONS] = {4,4,7,7,7,4,4,7,4,7};

int nMaxPrimaryAmmo [MAX_PRIMARY_WEAPONS] = {0, VULCAN_AMMO_MAX, 0, 0, 0, 0, VULCAN_AMMO_MAX, 0, 0, 0};
ubyte nMaxSecondaryAmmo [MAX_SECONDARY_WEAPONS] = {20, 10, 10, 5, 5, 20, 20, 15, 10, 10};

//for each primary weapon, what kind of powerup gives weapon
ubyte primaryWeaponToPowerup[MAX_PRIMARY_WEAPONS] = {POW_LASER,POW_VULCAN_WEAPON,POW_SPREADFIRE_WEAPON,POW_PLASMA_WEAPON,POW_FUSION_WEAPON,POW_LASER,POW_GAUSS_WEAPON,POW_HELIX_WEAPON,POW_PHOENIX_WEAPON,POW_OMEGA_WEAPON};

//for each Secondary weapon, what kind of powerup gives weapon
ubyte secondaryWeaponToPowerup[MAX_SECONDARY_WEAPONS] = {POW_MISSILE_1,POW_HOMING_AMMO_1,POW_PROXIMITY_WEAPON,POW_SMARTBOMB_WEAPON,POW_MEGA_WEAPON,POW_SMISSILE1_1,POW_GUIDED_MISSILE_1,POW_SMART_MINE,POW_MERCURY_MISSILE_1,POW_EARTHSHAKER_MISSILE};

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

//allow player to reorder menus?

//char	*Primary_weapon_names[MAX_PRIMARY_WEAPONS] = {
//	"Laser Cannon",
//	"Vulcan Cannon",
//	"Spreadfire Cannon",
//	"Plasma Cannon",
//	"Fusion Cannon"
//};

//char	*pszSecondaryWeaponNames[MAX_SECONDARY_WEAPONS] = {
//	"Concussion Missile",
//	"Homing Missile",
//	"Proximity Bomb",
//	"Smart Missile",
//	"Mega Missile"
//};

//char	*pszShortPrimaryWeaponNames[MAX_PRIMARY_WEAPONS] = {
//	"Laser",
//	"Vulcan",
//	"Spread",
//	"Plasma",
//	"Fusion"
//};

//char	*pszShortSecondaryWeaponNames[MAX_SECONDARY_WEAPONS] = {
//	"Concsn\nMissile",
//	"Homing\nMissile",
//	"Proxim.\nBomb",
//	"Smart\nMissile",
//	"Mega\nMissile"
//};

sbyte   bIsEnergyWeapon[MAX_WEAPON_TYPES] = {
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

//	------------------------------------------------------------------------------------
//	Return:
// Bits set:
//		HAS_WEAPON_FLAG
//		HAS_ENERGY_FLAG
//		HAS_AMMO_FLAG	
// See weapon.h for bit values
int PlayerHasWeapon (int nWeapon, int bSecondary, int nPlayer)
{
	int		return_value = 0;
	int		nWeaponIndex;
	player	*playerP = gameData.multi.players + ((nPlayer < 0) ? gameData.multi.nLocalPlayer : nPlayer);

//	Hack! If energy goes negative, you can't fire a weapon that doesn't require energy.
//	But energy should not go negative (but it does), so find out why it does!
if (gameStates.app.bD1Mission && (nWeapon >= SUPER_WEAPON))
	return 0;
if (playerP->energy < 0)
	playerP->energy = 0;

if (!bSecondary) {
	nWeaponIndex = primaryWeaponToWeaponInfo[nWeapon];

	if (nWeapon == SUPER_LASER_INDEX) {
		if ((playerP->primary_weapon_flags & (1 << LASER_INDEX)) &&
				(playerP->laser_level > MAX_LASER_LEVEL))
			return_value |= HAS_WEAPON_FLAG;
		}
	else if (nWeapon == LASER_INDEX) {
		if ((playerP->primary_weapon_flags & (1 << LASER_INDEX)) &&
			 (playerP->laser_level <= MAX_LASER_LEVEL))
			return_value |= HAS_WEAPON_FLAG;
		}
	else if (nWeapon == SPREADFIRE_INDEX) {
		if ((playerP->primary_weapon_flags & (1 << nWeapon)) &&
			!(extraGameInfo [0].bSmartWeaponSwitch && 
				(playerP->primary_weapon_flags & (1 << HELIX_INDEX))))
			return_value |= HAS_WEAPON_FLAG;
		}
	else if (nWeapon == VULCAN_INDEX) {
		if ((playerP->primary_weapon_flags & (1 << nWeapon)) &&
			!(extraGameInfo [0].bSmartWeaponSwitch && 
				(playerP->primary_weapon_flags & (1 << GAUSS_INDEX))))
			return_value |= HAS_WEAPON_FLAG;
		}
	else {
		if (playerP->primary_weapon_flags & (1 << nWeapon))
			return_value |= HAS_WEAPON_FLAG;
		}

	// Special case: Gauss cannon uses vulcan ammo.		
	if (nWeapon == GAUSS_INDEX) {
		if (WI_ammo_usage (nWeaponIndex) <= playerP->primary_ammo [VULCAN_INDEX])
			return_value |= HAS_AMMO_FLAG;
		}
	else
		if (WI_ammo_usage (nWeaponIndex) <= playerP->primary_ammo [nWeapon])
			return_value |= HAS_AMMO_FLAG;
	if (nWeapon == OMEGA_INDEX) {	// Hack: Make sure player has energy to omega
		if (playerP->energy || xOmegaCharge)
			return_value |= HAS_ENERGY_FLAG;
		}
	else {
/*
		if (nWeapon == SUPER_LASER_INDEX) {	
			if (playerP->energy || xOmegaCharge)
				return_value |= HAS_ENERGY_FLAG;
		}
*/
		if (WI_energy_usage (nWeaponIndex) <= playerP->energy)
			return_value |= HAS_ENERGY_FLAG;
		}
	}
else {
	nWeaponIndex = secondaryWeaponToWeaponInfo[nWeapon];

	if (playerP->secondary_weapon_flags & (1 << nWeapon))
		return_value |= HAS_WEAPON_FLAG;

	if (WI_ammo_usage (nWeaponIndex) <= playerP->secondary_ammo [nWeapon])
		return_value |= HAS_AMMO_FLAG;

	if (WI_energy_usage(nWeaponIndex) <= playerP->energy)
		return_value |= HAS_ENERGY_FLAG;
}

return return_value;
}

//	------------------------------------------------------------------------------------

void InitWeaponOrdering ()
 {
  // short routine to setup default weapon priorities for new pilots

  int i;

  for (i=0;i<MAX_PRIMARY_WEAPONS+1;i++)
	primaryOrder[i]=defaultPrimaryOrder[i];
  for (i=0;i<MAX_SECONDARY_WEAPONS+1;i++)
	secondaryOrder[i]=defaultSecondaryOrder[i];
 }	

//	------------------------------------------------------------------------------------

void CyclePrimary ()
{
bCycling = 1;
AutoSelectWeapon (0, 0);
bCycling = 0;
}

//	------------------------------------------------------------------------------------

void CycleSecondary ()
{
bCycling = 1;
AutoSelectWeapon (1, 0);
bCycling = 0;
}


//	------------------------------------------------------------------------------------
//if message flag set, print message saying selected
void SelectWeapon(int weapon_num, int secondary_flag, int print_message, int bWaitForRearm)
{
	char	*weapon_name;

if (gameData.demo.nState==ND_STATE_RECORDING)
	NDRecordPlayerWeapon(secondary_flag, weapon_num);
if (!secondary_flag) {
	if (gameData.weapons.nPrimary != weapon_num) {
		if (bWaitForRearm) 
			DigiPlaySampleOnce( SOUND_GOOD_SELECTION_PRIMARY, F1_0);
#ifdef NETWORK
		if (gameData.app.nGameMode & GM_MULTI)	{
			if (bWaitForRearm) 
				MultiSendPlaySound(SOUND_GOOD_SELECTION_PRIMARY, F1_0);
			}
#endif
		xNextLaserFireTime = bWaitForRearm ? gameData.time.xGame + REARM_TIME : 0;
		gameData.app.nGlobalLaserFiringCount = 0;
		} 
	else {
		// Select super version if available.
		if (bWaitForRearm) {
			if (!bCycling)
				; // -- MK, only plays when can't fire weapon anyway, fixes bug -- DigiPlaySampleOnce( SOUND_ALREADY_SELECTED, F1_0);
			else
				DigiPlaySampleOnce( SOUND_BAD_SELECTION, F1_0);
			}
		}
	gameData.weapons.nOverridden = weapon_num;
	if (!secondary_flag && extraGameInfo [IsMultiGame].bSmartWeaponSwitch && !gameStates.app.bD1Mission) {
		switch (weapon_num) {
			case 1:
				if (gameData.multi.players [gameData.multi.nLocalPlayer].primary_weapon_flags & (1 << 6))
					weapon_num = 6;
				break;
			case 2:
				if (gameData.multi.players [gameData.multi.nLocalPlayer].primary_weapon_flags & (1 << 7))
					weapon_num = 7;
				break;
			}
		}
	gameData.weapons.nPrimary = (!secondary_flag && (weapon_num == SUPER_LASER_INDEX)) ? LASER_INDEX : weapon_num;
	weapon_name = PRIMARY_WEAPON_NAMES (weapon_num);
   #if defined (TACTILE)
 	tactile_set_button_jolt();
	#endif
	//save flag for whether was super version
	bLastPrimaryWasSuper[weapon_num % SUPER_WEAPON] = (weapon_num >= SUPER_WEAPON);
	}
else {
	if (gameData.weapons.nSecondary != weapon_num) {
		if (bWaitForRearm) 
			DigiPlaySampleOnce( SOUND_GOOD_SELECTION_SECONDARY, F1_0);
#ifdef NETWORK
		if (gameData.app.nGameMode & GM_MULTI) {
			if (bWaitForRearm) 
				MultiSendPlaySound(SOUND_GOOD_SELECTION_PRIMARY, F1_0);
			}
#endif
		xNextMissileFireTime = bWaitForRearm ? gameData.time.xGame + REARM_TIME : 0;
		gameData.app.nGlobalMissileFiringCount = 0;
		}
	else {
		if (bWaitForRearm) {
		 if (!bCycling)
			DigiPlaySampleOnce( SOUND_ALREADY_SELECTED, F1_0);
		 else
			DigiPlaySampleOnce( SOUND_BAD_SELECTION, F1_0);
		}
	}
	gameData.weapons.nSecondary = weapon_num;
	weapon_name = SECONDARY_WEAPON_NAMES(weapon_num);
	//save flag for whether was super version
	bLastSecondaryWasSuper[weapon_num % SUPER_WEAPON] = (weapon_num >= SUPER_WEAPON);
	}

if (print_message) {
	if (weapon_num == LASER_INDEX && !secondary_flag)
		HUDInitMessage(TXT_WPN_LEVEL, weapon_name, gameData.multi.players [gameData.multi.nLocalPlayer].laser_level+1, TXT_SELECTED);
	else
		HUDInitMessage("%s %s", weapon_name, TXT_SELECTED);
	}
}

//flags whether the last time we use this weapon, it was the 'super' version
//	------------------------------------------------------------------------------------
//	Select a weapon, primary or secondary.
void DoSelectWeapon(int nWeapon, int bSecondary)
{
	int	weapon_num_save=nWeapon;
	int	weapon_status,current,has_flag;
	ubyte	last_was_super;

if (!bSecondary) {
	current = gameData.weapons.nPrimary;
	if ((current == LASER_INDEX) && (gameData.multi.players [gameData.multi.nLocalPlayer].laser_level > MAX_LASER_LEVEL))
		current = SUPER_LASER_INDEX;
	last_was_super = bLastPrimaryWasSuper [nWeapon];
	if ((nWeapon == LASER_INDEX) && (gameData.multi.players [gameData.multi.nLocalPlayer].laser_level > MAX_LASER_LEVEL))
		nWeapon = SUPER_LASER_INDEX;
	has_flag = HAS_WEAPON_FLAG;
	}
else {
	current = gameData.weapons.nSecondary;
	last_was_super = bLastSecondaryWasSuper [nWeapon];
	has_flag = HAS_WEAPON_FLAG+HAS_AMMO_FLAG;
	}

if ((current == nWeapon) || (current == nWeapon + SUPER_WEAPON)) {
	//already have this selected, so toggle to other of normal/super version
	if (!bSecondary && (current == SUPER_LASER_INDEX))
		return;
	nWeapon %= SUPER_WEAPON;
	if (!last_was_super)
		nWeapon += SUPER_WEAPON;
	weapon_status = PlayerHasWeapon (nWeapon, bSecondary, -1);
	}
else {
	//go to last-select version of requested missile
	if (last_was_super && (nWeapon < SUPER_WEAPON))
		nWeapon += SUPER_WEAPON;
	weapon_status = PlayerHasWeapon (nWeapon, bSecondary, -1);
	//if don't have last-selected, try other version
	if ((weapon_status & has_flag) != has_flag) {
		nWeapon = 2 * weapon_num_save + SUPER_WEAPON - nWeapon;
		weapon_status = PlayerHasWeapon(nWeapon, bSecondary, -1);
		if ((weapon_status & has_flag) != has_flag)
			nWeapon = 2 * weapon_num_save + SUPER_WEAPON - nWeapon;
		}
	}

//if we don't have the weapon we're switching to, give error & bail
if ((weapon_status & has_flag) != has_flag) {
	if (!bSecondary) {
		if (nWeapon==SUPER_LASER_INDEX)
			return; 		//no such thing as super laser, so no error
		HUDInitMessage("%s %s!", TXT_DONT_HAVE, PRIMARY_WEAPON_NAMES(nWeapon));
		}
	else
		HUDInitMessage("%s %s%s",TXT_HAVE_NO, SECONDARY_WEAPON_NAMES(nWeapon), TXT_SX);
	DigiPlaySample( SOUND_BAD_SELECTION, F1_0);
	return;
	}
//now actually select the weapon
SelectWeapon(nWeapon, bSecondary, 1, 1);
}

//	----------------------------------------------------------------------------------------

inline int WeaponId (int w)
{
if (w != LASER_INDEX)
	return w;
else if ((gameData.multi.players [gameData.multi.nLocalPlayer].laser_level <= MAX_LASER_LEVEL) || 
			!bLastPrimaryWasSuper [LASER_INDEX])
	return LASER_INDEX;
else
	return SUPER_LASER_INDEX;
}

//	----------------------------------------------------------------------------------------

void SetLastSuperWeaponStates (void)
{
	player	*playerP = gameData.multi.players + gameData.multi.nLocalPlayer;
	int		i, j;

for (i = 0, j = 1 << 5; i < 5; i++, j <<= 1) {
	bLastPrimaryWasSuper [i] = i ? playerP->primary_weapon_flags & j : playerP->laser_level > MAX_LASER_LEVEL;
	bLastSecondaryWasSuper [i] = playerP->secondary_weapon_flags & j;
	}
}

//	----------------------------------------------------------------------------------------
//	Automatically select next best weapon if unable to fire current weapon.
// Weapon type: 0==primary, 1==secondary

void AutoSelectWeapon(int nWeaponType, int bAutoSelect)
{
	int	r;
	int	nCutPoint;
	int	bLooped = 0;

if (bAutoSelect && !gameOpts->gameplay.nAutoSelectWeapon)
	return;
if (!nWeaponType) {
	r = PlayerHasWeapon (WeaponId (gameData.weapons.nPrimary), 0, -1);
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
// if (primaryOrder[iNewWeapon] == FUSION_INDEX)
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
				if ((nNewWeapon != 255) && (PlayerHasWeapon (nNewWeapon, 0, -1) == HAS_ALL)) {
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
	r = PlayerHasWeapon(gameData.weapons.nSecondary, 1, -1);
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
			else if (PlayerHasWeapon(secondaryOrder [iCurWeapon], 1, -1) == HAS_ALL) {
				SelectWeapon(secondaryOrder [iCurWeapon], 1, 1, 1);
				bTryAgain = 0;
				}
			}
		}
	}
}

#ifndef RELEASE

//	----------------------------------------------------------------------------------------
//	Show player which weapons he has, how much ammo...
//	Looks like a debug screen now because it writes to mono screen, but that will change...
void ShowWeaponStatus(void)
{
	int	i;
#if TRACE
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++) {
	if (gameData.multi.players [gameData.multi.nLocalPlayer].primary_weapon_flags & (1 << i))
		con_printf (CON_DEBUG, "HAVE");
	else
		con_printf (CON_DEBUG, "    ");
	con_printf (CON_DEBUG, 
		"  Weapon: %20s, charges: %4i\n", 
		PRIMARY_WEAPON_NAMES(i), gameData.multi.players [gameData.multi.nLocalPlayer].primary_ammo [i]);
	}
	con_printf (CON_DEBUG, "\n");
	for (i=0; i<MAX_SECONDARY_WEAPONS; i++) {
		if (gameData.multi.players [gameData.multi.nLocalPlayer].secondary_weapon_flags & (1 << i))
			con_printf (CON_DEBUG, "HAVE");
		else
			con_printf (CON_DEBUG, "    ");
		con_printf (CON_DEBUG, 
			"  Weapon: %20s, charges: %4i\n", 
			SECONDARY_WEAPON_NAMES(i), gameData.multi.players [gameData.multi.nLocalPlayer].secondary_ammo [i]);
	}
con_printf (CON_DEBUG, "\n");
con_printf (CON_DEBUG, "\n");
#endif
}

#endif

//	---------------------------------------------------------------------
//called when one of these weapons is picked up
//when you pick up a secondary, you always get the weapon & ammo for it
//	Returns true if powerup picked up, else returns false.
int PickupSecondary (object *objP, int nWeaponIndex, int count, int nPlayer)
{
	int		max;
	int		nPickedUp;
	int		cutpoint, bEmpty = 0;
	player	*playerP = gameData.multi.players + nPlayer;

max = nMaxSecondaryAmmo [nWeaponIndex];
if (playerP->flags & PLAYER_FLAGS_AMMO_RACK)
	max *= 2;
if (playerP->secondary_ammo [nWeaponIndex] >= max) {
	if (LOCALPLAYER (nPlayer))
		HUDInitMessage("%s %i %ss!", 
			TXT_ALREADY_HAVE, 
			playerP->secondary_ammo [nWeaponIndex],
			SECONDARY_WEAPON_NAMES(nWeaponIndex));
	return 0;
	}
playerP->secondary_weapon_flags |= (1 << nWeaponIndex);
playerP->secondary_ammo [nWeaponIndex] += count;
nPickedUp = count;
if (playerP->secondary_ammo [nWeaponIndex] > max) {
	nPickedUp = count - (playerP->secondary_ammo [nWeaponIndex] - max);
	playerP->secondary_ammo [nWeaponIndex] = max;
	if ((nPickedUp < count) && (nWeaponIndex != PROXIMITY_INDEX) && (nWeaponIndex != SMART_MINE_INDEX)) {
		short nObject = OBJ_IDX (objP);
		CBRK (gameData.multi.leftoverPowerups [nObject].nCount);
		gameData.multi.leftoverPowerups [nObject].nCount = count - nPickedUp;
		gameData.multi.leftoverPowerups [nObject].nType = secondaryWeaponToPowerup [nWeaponIndex];
		gameData.multi.leftoverPowerups [nObject].spitterP = gameData.objs.objects + playerP->objnum;
		}
	}
if (LOCALPLAYER (nPlayer)) {
	cutpoint = SOrderList (255);
	bEmpty = playerP->secondary_ammo [gameData.weapons.nSecondary] == 0;
	if (gameOpts->gameplay.nAutoSelectWeapon) {
		if (gameOpts->gameplay.nAutoSelectWeapon == 1) {
			if (bEmpty)
				SelectWeapon (nWeaponIndex, 1, 0, 1);
			}
		else if ((SOrderList (nWeaponIndex) < cutpoint) && 
					(bEmpty || (SOrderList (nWeaponIndex) < SOrderList (gameData.weapons.nSecondary))))
			SelectWeapon(nWeaponIndex,1, 0, 1);
		else {
			//if we don't auto-select this weapon, but it's a proxbomb or smart mine,
			//we want to do a mini-auto-selection that applies to the drop bomb key
			if ((nWeaponIndex == PROXIMITY_INDEX || nWeaponIndex == SMART_MINE_INDEX) &&
				!(gameData.weapons.nSecondary == PROXIMITY_INDEX || gameData.weapons.nSecondary == SMART_MINE_INDEX)) {
				int cur = bLastSecondaryWasSuper [PROXIMITY_INDEX] ? SMART_MINE_INDEX : PROXIMITY_INDEX;
				if (SOrderList (nWeaponIndex) < SOrderList (cur))
					bLastSecondaryWasSuper[PROXIMITY_INDEX] = (nWeaponIndex == SMART_MINE_INDEX);
				}
			}
		}
	//note: flash for all but concussion was 7,14,21
	if (count>1) {
		PALETTE_FLASH_ADD (15,15,15);
		HUDInitMessage("%d %s%s", nPickedUp, SECONDARY_WEAPON_NAMES(nWeaponIndex), TXT_SX);
		}
	else {
		PALETTE_FLASH_ADD (10,10,10);
		HUDInitMessage("%s!", SECONDARY_WEAPON_NAMES (nWeaponIndex));
		}
	}
return 1;
}

//	-----------------------------------------------------------------------------

void ValidatePrios (ubyte *order, ubyte *defaultOrder, int n)
{
	ubyte		*f;
	int		i, s = (n + 1) * sizeof (ubyte);

//check for validity
if (!(f = (ubyte *) d_malloc (s)))
	return;
memset (f, 0, s);
for (i = 0; i <= n; i++)
	if (order [i] == 255)
		f [10]++;
	else if (order [i] < n)
		f [order [i]]++;
for (i = 0; i <= n; i++)
	if (f [i] != 1) {
		memcpy (order, defaultOrder, s);
		break;
		}
d_free (f);
}

//	-----------------------------------------------------------------------------

void ReorderPrimary ()
{
	newmenu_item	m [MAX_PRIMARY_WEAPONS + 2];
	int				i;

ValidatePrios (primaryOrder, defaultPrimaryOrder, MAX_PRIMARY_WEAPONS);
memset (m, 0, sizeof (m));
for (i = 0; i < MAX_PRIMARY_WEAPONS + 1; i++) {
	m [i].type = NM_TYPE_MENU;
	if (primaryOrder [i] == 255)
		m [i].text="ˆˆˆˆˆˆˆ Never autoselect ˆˆˆˆˆˆˆ";
	else
		m [i].text= (char *) PRIMARY_WEAPON_NAMES (primaryOrder [i]);
	m [i].value = primaryOrder [i];
}
gameStates.menus.bReordering = 1;
i = ExecMenu ("Reorder Primary", "Shift+Up/Down arrow to move item", i, m, NULL, NULL);
gameStates.menus.bReordering = 0;

for (i = 0; i < MAX_PRIMARY_WEAPONS + 1; i++)
	primaryOrder [i] = m [i].value;
}

//	-----------------------------------------------------------------------------

void ReorderSecondary ()
{
	newmenu_item m[MAX_SECONDARY_WEAPONS + 2];
	int i;

ValidatePrios (secondaryOrder, defaultSecondaryOrder, MAX_SECONDARY_WEAPONS);
memset (m, 0, sizeof (m));
for (i = 0; i < MAX_SECONDARY_WEAPONS + 1; i++)
{
	m[i].type = NM_TYPE_MENU;
	if (secondaryOrder [i] == 255)
		m[i].text = "ˆˆˆˆˆˆˆ Never autoselect ˆˆˆˆˆˆˆ";
	else
		m[i].text = (char *) SECONDARY_WEAPON_NAMES (secondaryOrder[i]);
	m[i].value=secondaryOrder[i];
}
gameStates.menus.bReordering = 1;
i = ExecMenu ("Reorder Secondary", "Shift+Up/Down arrow to move item", i, m, NULL, NULL);
gameStates.menus.bReordering = 0;
for (i = 0; i < MAX_SECONDARY_WEAPONS + 1; i++)
	secondaryOrder [i] = m [i].value;
}

//	-----------------------------------------------------------------------------

int POrderList (int num)
{
	int i;

for (i = 0; i < MAX_PRIMARY_WEAPONS + 1; i++)
	if (primaryOrder [i] == num)
		return (i);
Error ("Primary Weapon is not in order list!!!");
return 0;
}

//	-----------------------------------------------------------------------------

int SOrderList (int num)
{
	int i;

for (i = 0; i < MAX_SECONDARY_WEAPONS + 1; i++)
	if (secondaryOrder[i]==num)
		return i;
Error ("Secondary Weapon is not in order list!!!");
return 0;
}


//	-----------------------------------------------------------------------------

//called when a primary weapon is picked up
//returns true if actually picked up
int PickupPrimary (int nWeaponIndex, int nPlayer)
{
	player	*playerP = gameData.multi.players + nPlayer;
	//ushort old_flags = gameData.multi.players [gameData.multi.nLocalPlayer].primary_weapon_flags;
	ushort flag = 1 << nWeaponIndex;
	int cutpoint;
	int supposed_weapon = gameData.weapons.nPrimary;

if ((nWeaponIndex != LASER_INDEX) && (playerP->primary_weapon_flags & flag)) {		//already have
	if (LOCALPLAYER (nPlayer))
		HUDInitMessage("%s %s!", TXT_ALREADY_HAVE_THE, PRIMARY_WEAPON_NAMES (nWeaponIndex));
	return 0;
	}
playerP->primary_weapon_flags |= flag;
if (LOCALPLAYER (nPlayer)) {
	cutpoint=POrderList (255);
	if ((gameData.weapons.nPrimary == LASER_INDEX) && 
		(playerP->laser_level >= 4))
		supposed_weapon = SUPER_LASER_INDEX;  // allotment for stupid way of doing super laser
	if ((gameOpts->gameplay.nAutoSelectWeapon == 2) && 
		 (POrderList (nWeaponIndex) < cutpoint) && 
		 (POrderList (nWeaponIndex) < POrderList (supposed_weapon)))
		SelectWeapon (nWeaponIndex, 0, 0, 1);
	PALETTE_FLASH_ADD (7,14,21);
	if (nWeaponIndex != LASER_INDEX)
  		HUDInitMessage("%s!", PRIMARY_WEAPON_NAMES (nWeaponIndex));
	}
return 1;
}

//	-----------------------------------------------------------------------------

int CheckToUsePrimary(int nWeaponIndex)
{
	ushort old_flags = gameData.multi.players [gameData.multi.nLocalPlayer].primary_weapon_flags;
	ushort flag = 1 << nWeaponIndex;
	int cutpoint;

cutpoint=POrderList (255);
if (!(old_flags & flag) && 
	 (gameOpts->gameplay.nAutoSelectWeapon == 2) &&
	 POrderList(nWeaponIndex)<cutpoint && 
	 POrderList(nWeaponIndex)<POrderList(gameData.weapons.nPrimary)) {
	if (nWeaponIndex==SUPER_LASER_INDEX)
		SelectWeapon(LASER_INDEX,0,0,1);
	else
		SelectWeapon(nWeaponIndex,0,0,1);
	}
PALETTE_FLASH_ADD(7,14,21);
return 1;
}

//	-----------------------------------------------------------------------------

//called when ammo (for the vulcan cannon) is picked up
//	Returns the amount picked up
int PickupAmmo(int class_flag,int nWeaponIndex,int ammo_count, int nPlayer)
{
	int		max,cutpoint,supposed_weapon=gameData.weapons.nPrimary;
	int		old_ammo=class_flag;		//kill warning
	player	*playerP = gameData.multi.players + nPlayer;

Assert(class_flag==CLASS_PRIMARY && nWeaponIndex==VULCAN_INDEX);

max = nMaxPrimaryAmmo [nWeaponIndex];
if (playerP->flags & PLAYER_FLAGS_AMMO_RACK)
	max *= 2;
if (playerP->primary_ammo [nWeaponIndex] == max)
	return 0;
old_ammo = playerP->primary_ammo [nWeaponIndex];
playerP->primary_ammo [nWeaponIndex] += ammo_count;
if (playerP->primary_ammo [nWeaponIndex] > max) {
	ammo_count += (max - playerP->primary_ammo [nWeaponIndex]);
	playerP->primary_ammo [nWeaponIndex] = max;
	}
if (nPlayer = gameData.multi.nLocalPlayer) {
	cutpoint = POrderList (255);
	if ((gameData.weapons.nPrimary == LASER_INDEX) && (playerP->laser_level >= 4))
		supposed_weapon = SUPER_LASER_INDEX;  // allotment for stupid way of doing super laser
	if ((playerP->primary_weapon_flags & (1<<nWeaponIndex)) && 
		(nWeaponIndex > gameData.weapons.nPrimary) && 
		(old_ammo == 0) &&
		(POrderList (nWeaponIndex) < cutpoint) && 
		(POrderList (nWeaponIndex) < POrderList (supposed_weapon)))
		SelectWeapon(nWeaponIndex,0,0,1);
	}
return ammo_count;	//return amount used
}

//	-----------------------------------------------------------------------------

#define	ESHAKER_SHAKE_TIME		(F1_0*2)
#define	MAX_ESHAKER_DETONATES	4

fix	eshakerDetonateTimes [MAX_ESHAKER_DETONATES];

//	Call this to initialize for a new level.
//	Sets all super mega missile detonation times to 0 which means there aren't any.
void InitShakerDetonates(void)
{
	int	i;

for (i = 0; i < MAX_ESHAKER_DETONATES; i++)
	eshakerDetonateTimes [i] = 0;	
}

//	-----------------------------------------------------------------------------

//	If a smega missile been detonated, rock the mine!
//	This should be called every frame.
//	Maybe this should affect all robots, being called when they get their physics done.
void RockTheMineFrame(void)
{
	int	i;

for (i = 0; i < MAX_ESHAKER_DETONATES; i++) {
	if (eshakerDetonateTimes[i] != 0) {
		fix	delta_time = gameData.time.xGame - eshakerDetonateTimes[i];
		if (!gameStates.gameplay.seismic.bSound) {
			DigiPlaySampleLooping((short) gameStates.gameplay.seismic.nSound, F1_0, -1, -1);
			gameStates.gameplay.seismic.bSound = 1;
			gameStates.gameplay.seismic.nNextSoundTime = gameData.time.xGame + d_rand()/2;
			}
		if (delta_time < ESHAKER_SHAKE_TIME) {
			//	Control center destroyed, rock the player's ship.
			int	fc, rx, rz;
			fix	h;
			// -- fc = abs(delta_time - ESHAKER_SHAKE_TIME/2);
			//	Changed 10/23/95 to make decreasing for super mega missile.
			fc = (ESHAKER_SHAKE_TIME - delta_time) / (ESHAKER_SHAKE_TIME / 16);
			if (fc > 16)
				fc = 16;
			else if (fc == 0)
				fc = 1;
			gameStates.gameplay.seismic.nVolume += fc;
			h = 3 * F1_0 / 16 + (F1_0 * (16 - fc)) / 32;
			rx = FixMul(d_rand() - 16384, h);
			rz = FixMul(d_rand() - 16384, h);
			gameData.objs.console->mtype.phys_info.rotvel.x += rx;
			gameData.objs.console->mtype.phys_info.rotvel.z += rz;
			//	Shake the buddy!
			if (gameData.escort.nObjNum != -1) {
				gameData.objs.objects[gameData.escort.nObjNum].mtype.phys_info.rotvel.x += rx*4;
				gameData.objs.objects[gameData.escort.nObjNum].mtype.phys_info.rotvel.z += rz*4;
				}
			//	Shake a guided missile!
			gameStates.gameplay.seismic.nMagnitude += rx;
			} 
		else
			eshakerDetonateTimes[i] = 0;
		}
	}
//	Hook in the rumble sound effect here.
}

//	-----------------------------------------------------------------------------

#ifdef NETWORK
extern void multi_send_seismic (fix,fix);
#endif

#define	SEISMIC_DISTURBANCE_DURATION	(F1_0*5)

int SeismicLevel (void)
{
return gameStates.gameplay.seismic.nLevel;
}

//	-----------------------------------------------------------------------------

void InitSeismicDisturbances(void)
{
gameStates.gameplay.seismic.nStartTime = 0;
gameStates.gameplay.seismic.nEndTime = 0;
}

//	-----------------------------------------------------------------------------
//	Return true if time to start a seismic disturbance.
int StartSeismicDisturbance(void)
{
	int	rval;

if (gameStates.gameplay.seismic.nShakeDuration < 1)
	return 0;
rval =  (2 * FixMul(d_rand(), gameStates.gameplay.seismic.nShakeFrequency)) < gameData.time.xFrame;
if (rval) {
	gameStates.gameplay.seismic.nStartTime = gameData.time.xGame;
	gameStates.gameplay.seismic.nEndTime = gameData.time.xGame + gameStates.gameplay.seismic.nShakeDuration;
	if (!gameStates.gameplay.seismic.bSound) {
		DigiPlaySampleLooping((short) gameStates.gameplay.seismic.nSound, F1_0, -1, -1);
		gameStates.gameplay.seismic.bSound = 1;
		gameStates.gameplay.seismic.nNextSoundTime = gameData.time.xGame + d_rand()/2;
		}
#ifdef NETWORK
	if (gameData.app.nGameMode & GM_MULTI)
		multi_send_seismic (gameStates.gameplay.seismic.nStartTime,gameStates.gameplay.seismic.nEndTime);
#endif
	}
return rval;
}

//	-----------------------------------------------------------------------------

void SeismicDisturbanceFrame(void)
{
if (gameStates.gameplay.seismic.nShakeFrequency) {
	if (((gameStates.gameplay.seismic.nStartTime < gameData.time.xGame) && 
		  (gameStates.gameplay.seismic.nEndTime > gameData.time.xGame)) || StartSeismicDisturbance()) {
		fix	delta_time = gameData.time.xGame - gameStates.gameplay.seismic.nStartTime;
		int	fc, rx, rz;
		fix	h;

		fc = abs(delta_time - (gameStates.gameplay.seismic.nEndTime - gameStates.gameplay.seismic.nStartTime) / 2);
		fc /= F1_0 / 16;
		if (fc > 16)
			fc = 16;
		else if (fc == 0)
			fc = 1;
		gameStates.gameplay.seismic.nVolume += fc;
		h = 3 * F1_0 / 16 + (F1_0 * (16 - fc)) / 32;
		rx = FixMul(d_rand() - 16384, h);
		rz = FixMul(d_rand() - 16384, h);
		gameData.objs.console->mtype.phys_info.rotvel.x += rx;
		gameData.objs.console->mtype.phys_info.rotvel.z += rz;
		//	Shake the buddy!
		if (gameData.escort.nObjNum != -1) {
			gameData.objs.objects[gameData.escort.nObjNum].mtype.phys_info.rotvel.x += rx*4;
			gameData.objs.objects[gameData.escort.nObjNum].mtype.phys_info.rotvel.z += rz*4;
			}
		//	Shake a guided missile!
		gameStates.gameplay.seismic.nMagnitude += rx;
		}
	}
}


//	-----------------------------------------------------------------------------
//	Call this when a smega detonates to start the process of rocking the mine.
void ShakerRockStuff(void)
{
	int	i;

for (i = 0; i < MAX_ESHAKER_DETONATES; i++)
	if (eshakerDetonateTimes[i] + ESHAKER_SHAKE_TIME < gameData.time.xGame)
		eshakerDetonateTimes[i] = 0;
for (i = 0; i < MAX_ESHAKER_DETONATES; i++)
	if (eshakerDetonateTimes[i] == 0) {
		eshakerDetonateTimes[i] = gameData.time.xGame;
		break;
		}
}

//	-----------------------------------------------------------------------------

//	Call this once/frame to process all super mines in the level.
void ProcessSmartMinesFrame(void)
{
	int	i, j;
	int	start, add;

	//	If we don't know of there being any super mines in the level, just
	//	check every 8th object each frame.
if (gameStates.gameplay.bHaveSmartMines == 0) {
	start = gameData.app.nFrameCount & 7;
	add = 8;
	} 
else {
	start = 0;
	add = 1;
	}
gameStates.gameplay.bHaveSmartMines = 0;

for (i = start; i <= gameData.objs.nLastObject; i += add) {
	if ((gameData.objs.objects[i].type == OBJ_WEAPON) && (gameData.objs.objects[i].id == SUPERPROX_ID)) {
		int	parent_num = gameData.objs.objects[i].ctype.laser_info.parent_num;

		gameStates.gameplay.bHaveSmartMines = 1;
		if (gameData.objs.objects[i].lifeleft + F1_0*2 < gameData.weapons.info[SUPERPROX_ID].lifetime) {
			vms_vector	*vBombPos = &gameData.objs.objects[i].pos;

			for (j = 0; j <= gameData.objs.nLastObject; j++) {
				if ((gameData.objs.objects[j].type == OBJ_PLAYER) || 
					 (gameData.objs.objects[j].type == OBJ_ROBOT)) {
					fix	dist = VmVecDistQuick(vBombPos, &gameData.objs.objects[j].pos);
	
					if (j != parent_num)
						if (dist - gameData.objs.objects[j].size < F1_0*20) {
							if (gameData.objs.objects[i].segnum == gameData.objs.objects[j].segnum)
								gameData.objs.objects[i].lifeleft = 1;
							else {
								//	Object which is close enough to detonate smart mine is not in same segment as smart mine.
								//	Need to do a more expensive check to make sure there isn't an obstruction.
								if (((gameData.app.nFrameCount ^ (i+j)) % 4) == 0) {
									fvi_query	fq;
									fvi_info		hit_data;
									int			fate;

									fq.startSeg = gameData.objs.objects[i].segnum;
									fq.p0	= &gameData.objs.objects[i].pos;
									fq.p1 = &gameData.objs.objects[j].pos;
									fq.rad = 0;
									fq.thisObjNum = i;
									fq.ignoreObjList = NULL;
									fq.flags	= 0;

									fate = FindVectorIntersection(&fq, &hit_data);
									if (fate != HIT_WALL)
										gameData.objs.objects[i].lifeleft = 1;
									}
								}
							}
					}
				}
			}
		}
	}
}

//	-----------------------------------------------------------------------------

#define SPIT_SPEED 20

//this function is for when the player intentionally drops a powerup
//this function is based on DropPowerup()
int SpitPowerup (object *spitter, ubyte id, int seed)
{
	short		objnum;
	object	*objP;
	vms_vector	new_velocity, new_pos;

#if 0
if ((gameData.app.nGameMode & GM_NETWORK) &&
	 (gameData.multi.powerupsInMine [(int)id] + PowerupsOnShips (id) >= 
	  gameData.multi.maxPowerupsAllowed [id]))
	return -1;
#endif
d_srand(seed);
VmVecScaleAdd (&new_velocity,
					&spitter->mtype.phys_info.velocity,
					&spitter->orient.fvec,
					i2f (SPIT_SPEED));
new_velocity.x += (d_rand() - 16384) * SPIT_SPEED * 2;
new_velocity.y += (d_rand() - 16384) * SPIT_SPEED * 2;
new_velocity.z += (d_rand() - 16384) * SPIT_SPEED * 2;
// Give keys zero velocity so they can be tracked better in multi
if ((gameData.app.nGameMode & GM_MULTI) && (id >= POW_KEY_BLUE) && (id <= POW_KEY_GOLD))
	VmVecZero(&new_velocity);
//there's a piece of code which lets the player pick up a powerup if
//the distance between him and the powerup is less than 2 time their
//combined radii.  So we need to create powerups pretty far out from
//the player.
VmVecScaleAdd(&new_pos,&spitter->pos,&spitter->orient.fvec,spitter->size);
#ifdef NETWORK
if ((gameData.app.nGameMode & GM_MULTI) && (multiData.create.nLoc >= MAX_NET_CREATE_OBJECTS))
	return (-1);
#endif
objnum = CreateObject (OBJ_POWERUP, id, (short) (GetTeam (gameData.multi.nLocalPlayer) + 1), 
							  (short) spitter->segnum, &new_pos, &vmdIdentityMatrix, gameData.objs.pwrUp.info[id].size, 
							  CT_POWERUP, MT_PHYSICS, RT_POWERUP, 1);
if (objnum < 0) {
	Int3();
	return objnum;
	}
objP = gameData.objs.objects + objnum;
objP->mtype.phys_info.velocity = new_velocity;
objP->mtype.phys_info.drag = 512;	//1024;
objP->mtype.phys_info.mass = F1_0;
objP->mtype.phys_info.flags = PF_BOUNCE;
objP->rtype.vclip_info.nClipIndex = gameData.objs.pwrUp.info [objP->id].nClipIndex;
objP->rtype.vclip_info.xFrameTime = gameData.eff.pVClips[objP->rtype.vclip_info.nClipIndex].xFrameTime;
objP->rtype.vclip_info.nCurFrame = 0;
if (spitter == gameData.objs.console)
	objP->ctype.powerup_info.flags |= PF_SPAT_BY_PLAYER;
switch (objP->id) {
	case POW_MISSILE_1:
	case POW_MISSILE_4:
	case POW_SHIELD_BOOST:
	case POW_ENERGY:
		objP->lifeleft = (d_rand() + F1_0*3) * 64;		//	Lives for 3 to 3.5 binary minutes (a binary minute is 64 seconds)
		if (gameData.app.nGameMode & GM_MULTI)
			objP->lifeleft /= 2;
		break;
	default:
		//if (gameData.app.nGameMode & GM_MULTI)
		//	objP->lifeleft = (d_rand() + F1_0*3) * 64;		//	Lives for 5 to 5.5 binary minutes (a binary minute is 64 seconds)
		break;
	}
MultiSendWeapons (1);
return objnum;
}

//	-----------------------------------------------------------------------------

void DropCurrentWeapon ()
{
	int	objnum = -1, 
			ammo = 0, 
			seed;
#if 0
if (gameData.weapons.nPrimary == 0) {
	if (extraGameInfo [IsMultiGame].nWeaponDropMode ||
		((gameData.multi.players [gameData.multi.nLocalPlayer].laser_level <= MAX_LASER_LEVEL) &&
			!(gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_QUAD_LASERS))) {
	HUDInitMessage (TXT_CANT_DROP_PRIM);
	return;
	}
#endif
seed = d_rand ();
if (gameData.weapons.nPrimary == 0) {	//special laser drop handling
	if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_QUAD_LASERS) {
		gameData.multi.players [gameData.multi.nLocalPlayer].flags &= ~PLAYER_FLAGS_QUAD_LASERS;
		objnum = SpitPowerup (gameData.objs.console, POW_QUAD_FIRE, seed);
		if (objnum == -1) {
			gameData.multi.players [gameData.multi.nLocalPlayer].flags |= PLAYER_FLAGS_QUAD_LASERS;
			return;
			}
		HUDInitMessage(TXT_DROP_QLASER);
		}
	else if (gameData.multi.players [gameData.multi.nLocalPlayer].laser_level > MAX_LASER_LEVEL) {
		gameData.multi.players [gameData.multi.nLocalPlayer].laser_level--;
		objnum = SpitPowerup (gameData.objs.console, POW_SUPER_LASER, seed);
		if (objnum == -1) {
			gameData.multi.players [gameData.multi.nLocalPlayer].laser_level++;
			return;
			}
		HUDInitMessage (TXT_DROP_SLASER);
		}
#if 0	
	// cannot drop standard lasers because picking up super lasers will automatically
	// give you laser level 4, allowing an exploit by dropping all lasers, picking up
	// the super lasers first, thus receiving laser level 4, etc.
	else if (gameData.multi.players [gameData.multi.nLocalPlayer].primary_weapon_flags & 1) {
		if (0 > (objnum = SpitPowerup (gameData.objs.console, POW_LASER, seed)))
			return;
		if (gameData.multi.players [gameData.multi.nLocalPlayer].laser_level)
			gameData.multi.players [gameData.multi.nLocalPlayer].laser_level--;
		else
			gameData.multi.players [gameData.multi.nLocalPlayer].primary_weapon_flags &= ~1;
		HUDInitMessage(TXT_DROP_LASER);
		}
#endif
	}
else {
	if (gameData.weapons.nPrimary) 	//if selected weapon was not the laser
		gameData.multi.players [gameData.multi.nLocalPlayer].primary_weapon_flags &= (~(1 << gameData.weapons.nPrimary));
	objnum = SpitPowerup (gameData.objs.console, primaryWeaponToPowerup [gameData.weapons.nPrimary], seed);
	if (objnum == -1) {
		if (gameData.weapons.nPrimary) 	//if selected weapon was not the laser
			gameData.multi.players [gameData.multi.nLocalPlayer].primary_weapon_flags |= (1 << gameData.weapons.nPrimary);
		return;
		}
	HUDInitMessage (TXT_DROP_WEAPON, PRIMARY_WEAPON_NAMES (gameData.weapons.nPrimary));
	}
DigiPlaySample (SOUND_DROP_WEAPON,F1_0);
if ((gameData.weapons.nPrimary == VULCAN_INDEX) || (gameData.weapons.nPrimary == GAUSS_INDEX)) {
	//if it's one of these, drop some ammo with the weapon
	ammo = gameData.multi.players [gameData.multi.nLocalPlayer].primary_ammo [VULCAN_INDEX];
	if ((gameData.multi.players [gameData.multi.nLocalPlayer].primary_weapon_flags & HAS_FLAG(VULCAN_INDEX)) && 
		 (gameData.multi.players [gameData.multi.nLocalPlayer].primary_weapon_flags & HAS_FLAG(GAUSS_INDEX)))
		ammo /= 2;		//if both vulcan & gauss, drop half
	gameData.multi.players [gameData.multi.nLocalPlayer].primary_ammo [VULCAN_INDEX] -= ammo;
	if (objnum != -1)
		gameData.objs.objects [objnum].ctype.powerup_info.count = ammo;
	}
if (gameData.weapons.nPrimary == OMEGA_INDEX) {
	//dropped weapon has current energy
	if (objnum != -1)
		gameData.objs.objects[objnum].ctype.powerup_info.count = xOmegaCharge;
	}
#ifdef NETWORK
if (gameData.app.nGameMode & GM_MULTI) {
	MultiSendDropWeapon (objnum, seed);
	MultiSendWeapons (1);
	}
#endif
if (gameData.weapons.nPrimary) //if selected weapon was not the laser
	AutoSelectWeapon (0, 0);
}

//	-----------------------------------------------------------------------------

extern void DropOrb (void);

void DropSecondaryWeapon (int nWeapon)
{
	int objnum, seed, nPowerup, bHoardEntropy, bMine;

if (nWeapon < 0)
	nWeapon = gameData.weapons.nSecondary;
if (gameData.multi.players [gameData.multi.nLocalPlayer].secondary_ammo [nWeapon] == 0) {
	HUDInitMessage(TXT_CANT_DROP_SEC);
	return;
	}
nPowerup = secondaryWeaponToPowerup[nWeapon];
bHoardEntropy = (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)) != 0;
bMine = (nPowerup == POW_PROXIMITY_WEAPON) || (nPowerup == POW_SMART_MINE);
if (!bHoardEntropy && bMine &&
	  gameData.multi.players [gameData.multi.nLocalPlayer].secondary_ammo [nWeapon] < 4) {
	HUDInitMessage(TXT_DROP_NEED4);
	return;
	}
if (bHoardEntropy) {
	DropOrb ();
	return;
	}
if (bMine)
	gameData.multi.players [gameData.multi.nLocalPlayer].secondary_ammo [nWeapon] -= 4;
else
	gameData.multi.players [gameData.multi.nLocalPlayer].secondary_ammo [nWeapon]--;
seed = d_rand();
objnum = SpitPowerup (gameData.objs.console, nPowerup, seed);
if (objnum == -1) {
	if (bMine)
		gameData.multi.players [gameData.multi.nLocalPlayer].secondary_ammo [nWeapon] += 4;
	else
		gameData.multi.players [gameData.multi.nLocalPlayer].secondary_ammo [nWeapon]++;
	return;
	}
HUDInitMessage (TXT_DROP_WEAPON, SECONDARY_WEAPON_NAMES (gameData.weapons.nSecondary));
DigiPlaySample (SOUND_DROP_WEAPON,F1_0);
#ifdef NETWORK
if (gameData.app.nGameMode & GM_MULTI) {
	MultiSendDropWeapon (objnum, seed);
	MultiSendWeapons (1);
	}
#endif
if (gameData.multi.players [gameData.multi.nLocalPlayer].secondary_ammo [nWeapon] == 0) {
	gameData.multi.players [gameData.multi.nLocalPlayer].secondary_weapon_flags &= (~(1<<gameData.weapons.nSecondary));
	AutoSelectWeapon (1, 0);
	}
}

//	---------------------------------------------------------------------------------------
//	Do seismic disturbance stuff including the looping sounds with changing volume.

void DoSeismicStuff(void)
{
	int		stv_save;

if (gameStates.limitFPS.bSeismic && !gameStates.app.b40fpsTick)
	return;
stv_save = gameStates.gameplay.seismic.nVolume;
gameStates.gameplay.seismic.nMagnitude = 0;
gameStates.gameplay.seismic.nVolume = 0;
RockTheMineFrame();
SeismicDisturbanceFrame();
if (stv_save != 0) {
	if (gameStates.gameplay.seismic.nVolume == 0) {
		DigiStopLoopingSound();
		gameStates.gameplay.seismic.bSound = 0;
		}

	if ((gameData.time.xGame > gameStates.gameplay.seismic.nNextSoundTime) && gameStates.gameplay.seismic.nVolume) {
		int volume = gameStates.gameplay.seismic.nVolume * 2048;
		if (volume > F1_0)
			volume = F1_0;
		DigiChangeLoopingVolume(volume);
		gameStates.gameplay.seismic.nNextSoundTime = gameData.time.xGame + d_rand()/4 + 8192;
		}
	}
}

//	-----------------------------------------------------------------------------

int tactile_fire_duration[]={120,80,150,250,150,200,100,180,280,100};
int tactile_fire_repeat[]={260,90,160,160,160,210,110,191,291,111};

void tactile_set_button_jolt ()
 {
  #ifdef TACTILE

  FILE *infile;
  int t,i;
  static int stickmag=-1;
  int dur,rep;

  dur=tactile_fire_duration[gameData.weapons.nPrimary];
  rep=tactile_fire_repeat[gameData.weapons.nPrimary];

if (TactileStick) {
	if (stickmag==-1) {
		if (t=FindArg("-stickmag"))
			stickmag=atoi (Args[t+1]);
		else
			stickmag=50;
		infile=(FILE *)fopen ("stick.val","rt");
		if (infile!=NULL) {
			for (i=0;i<10;i++) {
				fscanf (infile,"%d %d\n",&tactile_fire_duration[i],&tactile_fire_repeat[i]);
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
 * reads n weapon_info structs from a CFILE
 */
extern int WeaponInfoReadN(weapon_info *pwi, int n, CFILE *fp, int file_version)
{
	int i, j;

	for (i = 0; i < n; i++, pwi++) {
		gameData.weapons.color [i].red =
		gameData.weapons.color [i].green =
		gameData.weapons.color [i].blue = 1.0;
		pwi->render_type = CFReadByte(fp);
		pwi->persistent = CFReadByte(fp);
		pwi->model_num = CFReadShort(fp);
		pwi->model_num_inner = CFReadShort(fp);

		pwi->flash_vclip = CFReadByte(fp);
		pwi->robot_hit_vclip = CFReadByte(fp);
		pwi->flash_sound = CFReadShort(fp);

		pwi->wall_hit_vclip = CFReadByte(fp);
		pwi->fire_count = CFReadByte(fp);
		pwi->robot_hit_sound = CFReadShort(fp);

		pwi->ammo_usage = CFReadByte(fp);
		pwi->weapon_vclip = CFReadByte(fp);
		pwi->wall_hit_sound = CFReadShort(fp);

		pwi->destroyable = CFReadByte(fp);
		pwi->matter = CFReadByte(fp);
		pwi->bounce = CFReadByte(fp);
		pwi->homing_flag = CFReadByte(fp);

		pwi->speedvar = CFReadByte(fp);
		pwi->flags = CFReadByte(fp);
		pwi->flash = CFReadByte(fp);
		pwi->afterburner_size = CFReadByte(fp);

		if (file_version >= 3)
			pwi->children = CFReadByte(fp);
		else
			/* Set the type of children correctly when using old
			 * datafiles.  In earlier descent versions this was simply
			 * hard-coded in CreateSmartChildren().
			 */
			switch (i)
			{
			case SMART_ID:
				pwi->children = PLAYER_SMART_HOMING_ID;
				break;
			case SUPERPROX_ID:
				pwi->children = SMART_MINE_HOMING_ID;
				break;
#if 0 /* not present in shareware */
			case ROBOT_SUPERPROX_ID:
				pwi->children = ROBOT_SMART_MINE_HOMING_ID;
				break;
			case EARTHSHAKER_ID:
				pwi->children = EARTHSHAKER_MEGA_ID;
				break;
#endif
			default:
				pwi->children = -1;
				break;
			}

		pwi->energy_usage = CFReadFix(fp);
		pwi->fire_wait = CFReadFix(fp);

		if (file_version >= 3)
			pwi->multi_damage_scale = CFReadFix(fp);
		else /* FIXME: hack this to set the real values */
			pwi->multi_damage_scale = F1_0;

		BitmapIndexRead(&pwi->bitmap, fp);

		pwi->blob_size = CFReadFix(fp);
		pwi->flash_size = CFReadFix(fp);
		pwi->impact_size = CFReadFix(fp);
		for (j = 0; j < NDL; j++)
			pwi->strength[j] = CFReadFix(fp);
		for (j = 0; j < NDL; j++)
			pwi->speed[j] = CFReadFix(fp);
		pwi->mass = CFReadFix(fp);
		pwi->drag = CFReadFix(fp);
		pwi->thrust = CFReadFix(fp);
		pwi->po_len_to_width_ratio = CFReadFix(fp);
		pwi->light = CFReadFix(fp);
		if (i == SPREADFIRE_ID)
			pwi->light = F1_0;
		else if (i == HELIX_ID)
			pwi->light = 3 * F1_0 / 2;
		pwi->lifetime = CFReadFix(fp);
		pwi->damage_radius = CFReadFix(fp);
		BitmapIndexRead(&pwi->picture, fp);
		if (file_version >= 3)
			BitmapIndexRead(&pwi->hires_picture, fp);
		else
			pwi->hires_picture.index = pwi->picture.index;
	}
	return i;
}
//	-----------------------------------------------------------------------------
//eof
