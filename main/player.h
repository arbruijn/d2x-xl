/* $Id: player.h,v 1.4 2003/10/04 03:14:47 btb Exp $ */
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
 * Structure information for the tPlayer
 *
 * Old Log:
 * Revision 1.1  1995/05/16  16:01:11  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:30:25  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.41  1994/12/20  17:56:43  yuan
 * Multiplayer tObject capability.
 *
 * Revision 1.40  1994/12/02  15:04:42  matt
 * Fixed bogus weapon constants and arrays
 *
 * Revision 1.39  1994/11/25  22:47:08  matt
 * Made saved game descriptions longer
 *
 * Revision 1.38  1994/11/21  17:29:38  matt
 * Cleaned up sequencing & game saving for secret levels
 *
 * Revision 1.37  1994/11/17  12:57:13  rob
 * Changed netKillsLevel to netKilledTotal.
 *
 * Revision 1.36  1994/11/14  17:20:33  rob
 * Bumped tPlayer file version.
 *
 * Revision 1.35  1994/11/04  19:55:06  rob
 * Changed a previously unused pad character to represent whether or not
 * the tPlayer is connected to a net game (used to be nObject=-1 but we
 * want to keep the nObject info in case of re-joins)
 *
 * Revision 1.34  1994/10/22  14:13:54  mike
 * Add homingObjectDist field to tPlayer struct.
 *
 * Revision 1.33  1994/10/22  00:08:45  matt
 * Fixed up problems with bonus & game sequencing
 * Player doesn't get credit for hostages unless he gets them out alive
 *
 * Revision 1.32  1994/10/21  20:43:03  mike
 * Add hostages_on_board to tPlayer struct.
 *
 * Revision 1.31  1994/10/19  20:00:00  john
 * Added bonus points at the end of level based on skill level.
 *
 * Revision 1.30  1994/10/19  15:14:24  john
 * Took % hits out of tPlayer structure, made %kills work properly.
 *
 * Revision 1.29  1994/10/19  12:12:27  john
 * Added hour variable.
 *
 * Revision 1.28  1994/10/17  17:24:48  john
 * Added startingLevel to tPlayer struct.
 *
 * Revision 1.27  1994/10/13  15:42:02  mike
 * Remove afterburner.
 *
 * Revision 1.26  1994/10/10  17:00:23  mike
 * Lower number of players from 10 to 8.
 *
 * Revision 1.25  1994/10/09  14:53:26  matt
 * Made tPlayer cockpit state & window size save/restore with saved games & automap
 *
 * Revision 1.24  1994/10/08  20:24:10  matt
 * Added difficulty level to tPlayer structure for game load/save
 *
 * Revision 1.23  1994/10/05  17:39:53  rob
 * Changed nKillerObj to a short (was char)
 *
 * Revision 1.22  1994/10/03  22:59:07  matt
 * Limit callsign to 8 chars long, so we can use it as filename
 *
 * Revision 1.21  1994/09/23  10:14:30  mike
 * Rename PLAYER_FLAGS_INVINCIBLE to PLAYER_FLAGS_INVULNERABLE.
 * Add INVULNERABLE_TIME_MAX = 30 seconds.
 *
 * Revision 1.20  1994/09/21  20:44:22  matt
 * Player explosion fireball now specified in bitmaps.tbl
 *
 * Revision 1.19  1994/09/21  12:27:37  mike
 * Move CLOAK_TIME_MAX here from game.c
 *
 * Revision 1.18  1994/09/16  13:10:16  mike
 * Add afterburner and cloak stuff.
 *
 * Revision 1.17  1994/09/11  20:30:26  matt
 * Cleaned up thrust vars, changing a few names
 *
 * Revision 1.16  1994/09/09  14:22:45  matt
 * Added extra gun for tPlayer
 *
 * Revision 1.15  1994/09/07  13:30:11  john
 * Added code to tell how many packets were lost.
 *
 * Revision 1.14  1994/09/02  11:56:33  mike
 * Alignment on the tPlayer struct.
 *
 * Revision 1.13  1994/08/25  18:12:05  matt
 * Made tPlayer's weapons and flares fire from the positions on the 3d model.
 * Also added support for quad lasers.
 *
 * Revision 1.12  1994/08/22  15:49:40  mike
 * change spelling on num_missles -> num_missiles.
 *
 * Revision 1.11  1994/08/18  10:47:32  john
 * Cleaned up game sequencing and tPlayer death stuff
 * in preparation for making the tPlayer explode into
 * pieces when dead.
 *
 * Revision 1.10  1994/08/17  16:50:05  john
 * Added damaging fireballs, missiles.
 *
 * Revision 1.9  1994/08/15  00:24:10  john
 * First version of netgame with players killing
 * each other. still buggy...
 *
 * Revision 1.8  1994/08/12  22:41:26  john
 * Took away Player_stats; add Players array.
 *
 * Revision 1.7  1994/08/09  17:53:25  adam
 * *** empty log message ***
 *
 * Revision 1.6  1994/07/13  00:15:05  matt
 * Moved all (or nearly all) of the values that affect tPlayer movement to
 * bitmaps.tbl
 *
 * Revision 1.5  1994/07/08  21:44:17  matt
 * Made laser powerups saturate correctly
 *
 * Revision 1.4  1994/07/07  14:59:02  john
 * Made radar powerups.
 *
 *
 * Revision 1.3  1994/07/02  13:49:39  matt
 * Cleaned up includes
 *
 * Revision 1.2  1994/07/02  13:10:03  matt
 * Moved tPlayer stats struct from loadgame.h to player.h
 *
 * Revision 1.1  1994/07/02  11:00:43  matt
 * Initial revision
 *
 *
 */

#ifndef _PLAYER_H
#define _PLAYER_H

#include "fix.h"
#include "vecmat.h"
#include "weapon.h"

#define MAX_PLAYERS 8
#define MAX_MULTI_PLAYERS MAX_PLAYERS+3

// Initial tPlayer stat values
#define INITIAL_ENERGY  i2f(100)    // 100% energy to start
#define INITIAL_SHIELDS i2f(100)    // 100% shields to start

#define MAX_ENERGY      i2f(200)    // go up to 200
#define MAX_SHIELDS     i2f(200)

#define INITIAL_LIVES               3   // start off with 3 lives

// Values for special flags
#define PLAYER_FLAGS_INVULNERABLE   1       // Player is invulnerable
#define PLAYER_FLAGS_BLUE_KEY       2       // Player has blue key
#define PLAYER_FLAGS_RED_KEY        4       // Player has red key
#define PLAYER_FLAGS_GOLD_KEY       8       // Player has gold key
#define PLAYER_FLAGS_FLAG           16      // Player has his team's flag
#define PLAYER_FLAGS_UNUSED         32      //
#define PLAYER_FLAGS_MAP_ALL        64      // Player can see unvisited areas on map
#define PLAYER_FLAGS_AMMO_RACK      128     // Player has ammo rack
#define PLAYER_FLAGS_CONVERTER      256     // Player has energy->shield converter
#define PLAYER_FLAGS_MAP_ALL_CHEAT  512     // Player can see unvisited areas on map normally
#define PLAYER_FLAGS_QUAD_LASERS    1024    // Player shoots 4 at once
#define PLAYER_FLAGS_CLOAKED        2048    // Player is cloaked for awhile
#define PLAYER_FLAGS_AFTERBURNER    4096    // Player's afterburner is engaged
#define PLAYER_FLAGS_HEADLIGHT      8192    // Player has headlight boost
#define PLAYER_FLAGS_HEADLIGHT_ON   16384   // is headlight on or off?

#define PLAYER_FLAGS_ALL_KEYS			(PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY)

#define AFTERBURNER_MAX_TIME    (F1_0*5)    // Max time afterburner can be on.
#define CALLSIGN_LEN                8       // so can use as filename (was: 12)

// Amount of time tPlayer is cloaked.
#define CLOAK_TIME_MAX          (F1_0*30)
#define INVULNERABLE_TIME_MAX   (F1_0*30)

#define PLAYER_STRUCT_VERSION   17  // increment this every time tPlayer struct changes

// defines for teams
#define TEAM_BLUE   0
#define TEAM_RED    1

// When this structure changes, increment the constant
// SAVE_FILE_VERSION in playsave.c
typedef struct tPlayer {
	// Who am I data
	char    callsign[CALLSIGN_LEN+1];   // The callsign of this tPlayer, for net purposes.
	ubyte   netAddress[6];         // The network address of the tPlayer.
	sbyte   connected;              // Is the tPlayer connected or not?
	int     nObject;                 // What tObject number this tPlayer is. (made an int by mk because it's very often referenced)
	int     nPacketsGot;          // How many packets we got from them
	int     nPacketsSent;         // How many packets we sent to them

	//  -- make sure you're 4 byte aligned now!

	// Game data
	uint    flags;                  // Powerup flags, see below...
	fix     energy;                 // Amount of energy remaining.
	fix     shields;                // shields remaining (protection)
	ubyte   lives;                  // Lives remaining, 0 = game over.
	sbyte   level;                  // Current level tPlayer is playing. (must be signed for secret levels)
	ubyte   laserLevel;            // Current level of the laser.
	sbyte   startingLevel;         // What level the tPlayer started on.
	short   nKillerObj;          // Who killed me.... (-1 if no one)
	ushort  primaryWeaponFlags;   // bit set indicates the tPlayer has this weapon.
	ushort  secondaryWeaponFlags; // bit set indicates the tPlayer has this weapon.
	ushort  primaryAmmo[MAX_PRIMARY_WEAPONS]; // How much ammo of each nType.
	ushort  secondaryAmmo[MAX_SECONDARY_WEAPONS]; // How much ammo of each nType.
#if 1 //for inventory system
	ubyte	  nInvuls;
	ubyte   nCloaks;
#else
	ushort  pad; // Pad because increased weaponFlags from byte to short -YW 3/22/95
#endif
	//  -- make sure you're 4 byte aligned now

	// Statistics...
	int     last_score;             // Score at beginning of current level.
	int     score;                  // Current score.
	fix     timeLevel;             // Level time played
	fix     timeTotal;             // Game time played (high word = seconds)

	fix     cloakTime;             // Time cloaked
	fix     invulnerableTime;      // Time invulnerable

	short   nKillGoalCount;          // Num of players killed this level
	short   netKilledTotal;       // Number of times killed total
	short   netKillsTotal;        // Number of net kills total
	short   numKillsLevel;        // Number of kills this level
	short   numKillsTotal;        // Number of kills total
	short   numRobotsLevel;       // Number of initial robots this level
	short   numRobotsTotal;       // Number of robots total
	ushort  hostages_rescuedTotal; // Total number of hostages rescued.
	ushort  hostagesTotal;         // Total number of hostages.
	ubyte   hostages_on_board;      // Number of hostages on ship.
	ubyte   hostagesLevel;         // Number of hostages on this level.
	fix     homingObjectDist;     // Distance of nearest homing tObject.
	sbyte   hoursLevel;            // Hours played (since timeTotal can only go up to 9 hours)
	sbyte   hoursTotal;            // Hours played (since timeTotal can only go up to 9 hours)
} __pack__ tPlayer;


//version 16 structure

#define MAX_PRIMARY_WEAPONS16   5
#define MAX_SECONDARY_WEAPONS16 5

typedef struct player16 {
	// Who am I data
	char    callsign[CALLSIGN_LEN+1]; // The callsign of this tPlayer, for net purposes.
	ubyte   netAddress[6];         // The network address of the tPlayer.
	sbyte   connected;              // Is the tPlayer connected or not?
	int     nObject;                 // What tObject number this tPlayer is. (made an int by mk because it's very often referenced)
	int     nPacketsGot;          // How many packets we got from them
	int     nPacketsSent;         // How many packets we sent to them

	//  -- make sure you're 4 byte aligned now!

	// Game data
	uint    flags;                  // Powerup flags, see below...
	fix     energy;                 // Amount of energy remaining.
	fix     shields;                // shields remaining (protection)
	ubyte   lives;                  // Lives remaining, 0 = game over.
	sbyte   level;                  // Current level tPlayer is playing. (must be signed for secret levels)
	ubyte   laserLevel;            // Current level of the laser.
	sbyte   startingLevel;         // What level the tPlayer started on.
	short   nKillerObj;          // Who killed me.... (-1 if no one)
	ubyte   primaryWeaponFlags;   // bit set indicates the tPlayer has this weapon.
	ubyte   secondaryWeaponFlags; // bit set indicates the tPlayer has this weapon.
	ushort  primaryAmmo[MAX_PRIMARY_WEAPONS16];    // How much ammo of each nType.
	ushort  secondaryAmmo[MAX_SECONDARY_WEAPONS16];// How much ammo of each nType.

	//  -- make sure you're 4 byte aligned now

	// Statistics...
	int     last_score;             // Score at beginning of current level.
	int     score;                  // Current score.
	fix     timeLevel;             // Level time played
	fix     timeTotal;             // Game time played (high word = seconds)

	fix     cloakTime;             // Time cloaked
	fix     invulnerableTime;      // Time invulnerable

	short   netKilledTotal;       // Number of times killed total
	short   netKillsTotal;        // Number of net kills total
	short   numKillsLevel;        // Number of kills this level
	short   numKillsTotal;        // Number of kills total
	short   numRobotsLevel;       // Number of initial robots this level
	short   numRobotsTotal;       // Number of robots total
	ushort  hostages_rescuedTotal; // Total number of hostages rescued.
	ushort  hostagesTotal;         // Total number of hostages.
	ubyte   hostages_on_board;      // Number of hostages on ship.
	ubyte   hostagesLevel;         // Number of hostages on this level.
	fix     homingObjectDist;     // Distance of nearest homing tObject.
	sbyte   hoursLevel;            // Hours played (since timeTotal can only go up to 9 hours)
	sbyte   hoursTotal;            // Hours played (since timeTotal can only go up to 9 hours)
} __pack__ player16;

/*
 * reads a tPlayerShip structure from a CFILE
 */
void PlayerShipRead(tPlayerShip *ps, CFILE *fp);

#endif
