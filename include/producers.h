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

#ifndef _PRODUCER_H
#define _PRODUCER_H

#include "segment.h"
#include "object.h"

// Create a robot in an object producer
CObject *CreateMorphRobot (CSegment *segp, CFixVector *object_pos, ubyte object_id);

int GatherFlagGoals (void);
// Called to repair an CObject
//--repair-- int refuel_do_repair_effect( CObject * obj, int firstTime, int repair_seg );

//--repair-- //do the repair center for this frame
//--repair-- void do_repair_sequence(CObject *obj);
//--repair--
//--repair-- //see if we should start the repair center
//--repair-- void check_start_repair_center(CObject *obj);
//--repair--
//--repair-- //if repairing, cut it short
//--repair-- abort_repair_center();

// An array of pointers to segments with fuel centers.
typedef struct tProducerInfo {
	int     nType;
	int     nSegment;
	sbyte   bFlag;
	sbyte   bEnabled;
	sbyte   nLives;          // Number of times this can be enabled.
	sbyte   pad1;
	fix     xCapacity;
	fix     xMaxCapacity;
	fix     xTimer;          // used in object producer for when next robot comes out
	fix     xDisableTime;   // Time until center disabled.
	//CObject  *last_created_obj;
	//int     last_created_sig;
	CFixVector vCenter;
} __pack__ tProducerInfo;

// The max number of robot centers per mine.

typedef struct  {
	int     objFlags;    		// Up to 32 different robots
	fix     xHitPoints;     	// How hard it is to destroy this particular object producer
	fix     xInterval;       	// Interval between materialogrifizations
	short   nSegment;         	// Segment this is attached to.
	short   nProducer;    		// Index in producer array.
} __pack__ old_tObjProducerInfo;

typedef struct tObjectProducerInfo {
	int     objFlags [3]; 		// Up to 92 different robots
	fix     xHitPoints;     	// How hard it is to destroy this particular object producer
	fix     xInterval;       	// Interval between materializations
	short   nSegment;         	// Segment this is attached to.
	short   nProducer;    		// Index in producer array.
} __pack__ tObjectProducerInfo;

//--repair-- extern CObject *RepairObj;  // which CObject getting repaired, or NULL

// Called when a materialization center gets triggered by the player
// flying through some CTrigger!
int StartObjectProducer (short nSegment);
void DisableObjectProducers (void);
void InitAllObjectProducers (void);
void OperateRobotMaker (CObject *objP, short nSegment);
void SetEquipGenStates (void);

#if 0
#define OldReadObjectProducerInfo(mi, fp) CFRead(mi, sizeof(old_tObjProducerInfo), 1, fp)
#define ReadObjectProducerInfo(mi, fp) CFRead(mi, sizeof(tObjectProducerInfo), 1, fp)
#else
/*
 * reads an old_tObjProducerInfo structure from a CFILE
 */
void OldReadObjectProducerInfo(old_tObjProducerInfo *mi, CFile& cf);

/*
 * reads a tObjectProducerInfo structure from a CFILE
 */
void ReadObjectProducerInfo (tObjectProducerInfo *ps, CFile& cf, bool bOldFormat);
#endif

#define PRODUCER_IDX(_producerP)	((short) ((_producerP) - gameData.producers.producers.Buffer ()))
#define OBJECT_PRODUCER_IDX(_producerP)	((short) ((_objProducerP) - gameData.producers.producers.Buffer ()))

#endif
