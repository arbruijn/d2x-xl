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
	int     nType;
	int     nSegment;
	sbyte   bFlag;
	sbyte   bEnabled;
	sbyte   nLives;          // Number of times this can be enabled.
	sbyte   pad1;
	fix     xCapacity;
	fix     xMaxCapacity;
	fix     xTimer;          // used in matcen for when next robot comes out
	fix     xDisableTime;   // Time until center disabled.
	//tObject  *last_created_obj;
	//int     last_created_sig;
	vmsVector vCenter;
} __pack__ tFuelCenInfo;

// The max number of robot centers per mine.

typedef struct  {
	int     objFlags;    		// Up to 32 different robots
	fix     xHitPoints;     	// How hard it is to destroy this particular matcen
	fix     xInterval;       	// Interval between materialogrifizations
	short   nSegment;         	// Segment this is attached to.
	short   nFuelCen;    		// Index in fuelcen array.
} __pack__ old_tMatCenInfo;

typedef struct tMatCenInfo {
	int     objFlags [3]; 		// Up to 92 different robots
	fix     xHitPoints;     	// How hard it is to destroy this particular matcen
	fix     xInterval;       	// Interval between materializations
	short   nSegment;         	// Segment this is attached to.
	short   nFuelCen;    		// Index in fuelcen array.
} __pack__ tMatCenInfo;

extern tMatCenInfo RobotCenters [MAX_ROBOT_CENTERS];

//--repair-- extern tObject *RepairObj;  // which tObject getting repaired, or NULL

// Called when a materialization center gets triggered by the tPlayer
// flying through some tTrigger!
int MatCenTrigger (short nSegment);
void DisableMatCens (void);
void InitAllMatCens (void);
void BotGenCreate (tSegment *segP, int oldType);
void FuelCenCheckForHoardGoal(tSegment *segp);
void SpawnBotTrigger (tObject *objP, short nSegment);
int GetMatCenObjType (tFuelCenInfo *matCenP, int *objFlags);
void SetEquipGenStates (void);

#if 0
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
#define MATCEN_IDX(_matcenP)		((short) ((_matcenP) - gameData.matCens.matCens))

#endif
