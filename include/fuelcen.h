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
// A refueling center is one CSegment... to identify it in the
// CSegment structure, the "special" field is set to
// SEGMENT_IS_FUELCEN.  The "value" field is then used for how
// much fuel the center has left, with a maximum value of 100.

//-------------------------------------------------------------
// To hook into Inferno:
// * When all segents are deleted or before a new mine is created
//   or loaded, call ResetGenerators().
// * Add call to FuelCenCreate(CSegment * segp) to make a CSegment
//   which isn't a fuel center be a fuel center.
// * When a mine is loaded call fuelcen_activate(segp) with each
//   new CSegment as it loads. Always do this.
// * When a CSegment is deleted, always call FuelCenDelete(segp).
// * Call FuelCenReplenishAll() to fill 'em all up, like when
//   a new game is started.
// * When an CObject that needs to be refueled is in a CSegment, call
//   FuelCenGiveFuel(segp) to get fuel. (Call once for any refueling
//   CObject once per frame with the CObject's current CSegment.) This
//   will return a value between 0 and 100 that tells how much fuel
//   he got.


// Destroys all fuel centers, clears CSegment backpointer array.
void ResetGenerators();
// Create materialization center
void MatCenCreate ( CSegment * segp, int oldType );
// Makes a CSegment a fuel center.
void FuelCenCreate( CSegment * segp, int oldType );
// Makes a fuel center active... needs to be called when
// a CSegment is loaded from disk.
void FuelCenActivate( CSegment * segp, int stationType );
// Deletes a CSegment as a fuel center.
void FuelCenDelete( CSegment * segp );

// Charges all fuel centers to max capacity.
void FuelCenReplenishAll();

// Create a matcen robot
CObject *CreateMorphRobot (CSegment *segp, CFixVector *object_pos, ubyte object_id);

// Returns the amount of fuel this CSegment can give up.
// Can be from 0 to 100.

// Call once per frame.
void FuelcenUpdateAll();

// Called when hit by laser.
void FuelCenDamage(CSegment *segp, fix AmountOfDamage );

int GatherFlagGoals (void);
// Called to repair an CObject
//--repair-- int refuel_do_repair_effect( CObject * obj, int firstTime, int repair_seg );

extern char Special_names[MAX_CENTER_TYPES][11];

//--repair-- //do the repair center for this frame
//--repair-- void do_repair_sequence(CObject *obj);
//--repair--
//--repair-- //see if we should start the repair center
//--repair-- void check_start_repair_center(CObject *obj);
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
	//CObject  *last_created_obj;
	//int     last_created_sig;
	CFixVector vCenter;
} tFuelCenInfo;

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

//--repair-- extern CObject *RepairObj;  // which CObject getting repaired, or NULL

// Called when a materialization center gets triggered by the CPlayerData
// flying through some CTrigger!
int TriggerMatCen (short nSegment);
void DisableMatCens (void);
void InitAllMatCens (void);
void TriggerBotGen (CObject *objP, short nSegment);
int GetMatCenObjType (tFuelCenInfo *matCenP, int *objFlags);
void SetEquipGenStates (void);

#if 0
#define OldMatCenInfoRead(mi, fp) CFRead(mi, sizeof(old_tMatCenInfo), 1, fp)
#define MatCenInfoRead(mi, fp) CFRead(mi, sizeof(tMatCenInfo), 1, fp)
#else
/*
 * reads an old_tMatCenInfo structure from a CFILE
 */
void OldMatCenInfoRead(old_tMatCenInfo *mi, CFile& cf);

/*
 * reads a tMatCenInfo structure from a CFILE
 */
void MatCenInfoRead (tMatCenInfo *ps, CFile& cf);
#endif

#define FUELCEN_IDX(_fuelcenP)	((short) ((_fuelcenP) - gameData.matCens.fuelCenters))
#define MATCEN_IDX(_matcenP)		((short) ((_matcenP) - gameData.matCens.matCens))

#endif
