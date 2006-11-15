/* $Id: fuelcen.h,v 1.5 2003/10/04 03:14:47 btb Exp $ */
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
 * Definitions for fueling centers.
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:56:31  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:28:43  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.26  1995/01/26  12:19:16  rob
 * Added externs of things needed for multiplayer.
 *
 * Revision 1.25  1994/10/30  14:11:10  mike
 * rip out repair center stuff.
 *
 * Revision 1.24  1994/10/03  23:36:36  mike
 * Add nSegment and fuelcen_num (renaming dest_seg and *path) in tMatCenInfo struct.
 *
 * Revision 1.23  1994/09/30  00:37:44  mike
 * Change tFuelCenInfo struct.
 *
 * Revision 1.22  1994/09/27  15:42:49  mike
 * Kill some obsolete matcen constants, Prototype Special_names.
 *
 * Revision 1.21  1994/09/27  00:04:30  mike
 * Moved tFuelCenInfo struct here from fuelcen.c
 *
 * Revision 1.20  1994/09/25  15:55:37  mike
 * Prototype function DisableMatCens.
 *
 * Revision 1.19  1994/09/24  17:41:34  mike
 * Prototype TriggerMatCen.
 *
 * Revision 1.18  1994/08/03  17:52:19  matt
 * Tidied up repair centers a bit
 *
 * Revision 1.17  1994/08/02  12:16:01  mike
 * *** empty log message ***
 *
 * Revision 1.16  1994/08/01  11:04:03  yuan
 * New materialization centers.
 *
 * Revision 1.15  1994/07/21  19:02:15  mike
 * break repair centers.
 *
 * Revision 1.14  1994/07/14  11:25:22  john
 * Made control centers destroy better; made automap use Tab key.
 *
 * Revision 1.13  1994/07/13  10:45:33  john
 * Made control center tObject switch when dead.
 *
 * Revision 1.12  1994/07/09  17:36:44  mike
 * Add extern for find_connected_repair_seg.
 *
 * Revision 1.11  1994/06/15  19:00:32  john
 * Show timer in on top of 3d with mine destroyed...
 *
 * Revision 1.10  1994/05/31  16:49:46  john
 * Begin to add robot materialization centers.
 *
 * Revision 1.9  1994/05/30  20:22:03  yuan
 * New triggers.
 *
 * Revision 1.8  1994/05/05  16:41:14  matt
 * Cleaned up repair center code, and moved some from tObject.c to fuelcen.c
 *
 * Revision 1.7  1994/04/21  20:41:21  yuan
 * Added extern.
 *
 * Revision 1.6  1994/04/21  20:28:32  john
 * Added flag for Yuan to tell when a fuel center is destroyed.
 *
 * Revision 1.5  1994/04/14  17:00:59  john
 * Made repair cen's work properly; added reset_all_fuelcens.
 *
 * Revision 1.4  1994/04/12  20:28:04  john
 * Added control center.
 *
 * Revision 1.3  1994/04/08  15:37:10  john
 * Added repair centers.
 *
 * Revision 1.2  1994/04/06  19:10:38  john
 * NEw version.
 *
 *
 * Revision 1.1  1994/04/06  12:39:02  john
 * Initial revision
 *
 *
 */

#ifndef _FUELCEN_H
#define _FUELCEN_H

#include "segment.h"
#include "object.h"

//------------------------------------------------------------
// A refueling center is one tSegment... to identify it in the
// tSegment structure, the "special" field is set to
// SEGMENT_IS_FUELCEN.  The "value" field is then used for how
// much fuel the center has left, with a maximum value of 100.

//-------------------------------------------------------------
// To hook into Inferno:
// * When all segents are deleted or before a new mine is created
//   or loaded, call FuelCenReset().
// * Add call to FuelCenCreate(tSegment * segp) to make a tSegment
//   which isn't a fuel center be a fuel center.
// * When a mine is loaded call fuelcen_activate(segp) with each
//   new tSegment as it loads. Always do this.
// * When a tSegment is deleted, always call FuelCenDelete(segp).
// * Call FuelCenReplenishAll() to fill 'em all up, like when
//   a new game is started.
// * When an tObject that needs to be refueled is in a tSegment, call
//   FuelCenGiveFuel(segp) to get fuel. (Call once for any refueling
//   tObject once per frame with the tObject's current tSegment.) This
//   will return a value between 0 and 100 that tells how much fuel
//   he got.


// Destroys all fuel centers, clears tSegment backpointer array.
void FuelCenReset();
// Create materialization center
void MatCenCreate ( tSegment * segp, int oldType );
// Makes a tSegment a fuel center.
void FuelCenCreate( tSegment * segp, int oldType );
// Makes a fuel center active... needs to be called when
// a tSegment is loaded from disk.
void FuelCenActivate( tSegment * segp, int stationType );
// Deletes a tSegment as a fuel center.
void FuelCenDelete( tSegment * segp );

// Charges all fuel centers to max capacity.
void FuelCenReplenishAll();

// Create a matcen robot
tObject *CreateMorphRobot (tSegment *segp, vmsVector *object_pos, ubyte object_id);

// Returns the amount of fuel this tSegment can give up.
// Can be from 0 to 100.
fix FuelCenGiveFuel(tSegment *segp, fix MaxAmountCanTake );
fix RepairCenGiveShields(tSegment *segp, fix MaxAmountCanTake );
fix HostileRoomDamageShields (tSegment *segp, fix MaxAmountCanGive);

// Call once per frame.
void FuelcenUpdateAll();

// Called when hit by laser.
void FuelCenDamage(tSegment *segp, fix AmountOfDamage );

// Called to repair an tObject
//--repair-- int refuel_do_repair_effect( tObject * obj, int firstTime, int repair_seg );

extern char Special_names[MAX_CENTER_TYPES][11];

//--repair-- //do the repair center for this frame
//--repair-- void do_repair_sequence(tObject *obj);
//--repair--
//--repair-- //see if we should start the repair center
//--repair-- void check_start_repair_center(tObject *obj);
//--repair--
//--repair-- //if repairing, cut it short
//--repair-- abort_repair_center();

// An array of pointers to segments with fuel centers.
typedef struct tFuelCenInfo {
	int     Type;
	int     nSegment;
	sbyte   Flag;
	sbyte   Enabled;
	sbyte   Lives;          // Number of times this can be enabled.
	sbyte   dum1;
	fix     Capacity;
	fix     MaxCapacity;
	fix     Timer;          // used in matcen for when next robot comes out
	fix     DisableTime;   // Time until center disabled.
	//tObject  *last_created_obj;
	//int     last_created_sig;
	vmsVector Center;
} __pack__ tFuelCenInfo;

// The max number of robot centers per mine.

typedef struct  {
	int     robotFlags;    // Up to 32 different robots
	fix     hit_points;     // How hard it is to destroy this particular matcen
	fix     interval;       // Interval between materialogrifizations
	short   nSegment;         // Segment this is attached to.
	short   fuelcen_num;    // Index in fuelcen array.
} __pack__ old_tMatCenInfo;

typedef struct tMatCenInfo {
	int     robotFlags[2]; // Up to 64 different robots
	fix     hit_points;     // How hard it is to destroy this particular matcen
	fix     interval;       // Interval between materialogrifizations
	short   nSegment;         // Segment this is attached to.
	short   fuelcen_num;    // Index in fuelcen array.
} __pack__ tMatCenInfo;

extern tMatCenInfo RobotCenters[MAX_ROBOT_CENTERS];

//--repair-- extern tObject *RepairObj;  // which tObject getting repaired, or NULL

// Called when a materialization center gets triggered by the player
// flying through some tTrigger!
void MatCenTrigger (short nSegment);

void DisableMatCens (void);

void InitAllMatCens (void);

void FuelCenCheckForHoardGoal(tSegment *segp);

#ifdef FAST_FILE_IO
#define OldMatCenInfoRead(mi, fp) CFRead(mi, sizeof(old_tMatCenInfo), 1, fp)
#define MatCenInfoRead(mi, fp) CFRead(mi, sizeof(tMatCenInfo), 1, fp)
#else
/*
 * reads an old_tMatCenInfo structure from a CFILE
 */
void OldMatCenInfoRead(old_tMatCenInfo *mi, CFILE *fp);

/*
 * reads a tMatCenInfo structure from a CFILE
 */
void MatCenInfoRead (tMatCenInfo *ps, CFILE *fp);
#endif

#define FUELCEN_IDX(_fuelcenP)	((short) ((_fuelcenP) - gameData.matCens.fuelCenters))
#define MATCEN_IDX(_matcenP)		((short) ((_matcenP) - gameData.matCens.robotCenters))

#endif
