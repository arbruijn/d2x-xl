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
CObject *CreateMorphRobot (CSegment *segp, CFixVector *object_pos, uint8_t object_id);

int32_t GatherFlagGoals (void);

// An array of pointers to segments with fuel centers.
typedef struct tProducerInfo {
	int32_t     nType;
	int32_t     nSegment;
	int8_t		bFlag;
	int8_t		bEnabled;
	int8_t		nLives;          // Number of times this can be enabled.
	int8_t		pad1;
	fix			xCapacity;
	fix			xMaxCapacity;
	fix			xTimer;          // used in object producer for when next robot comes out
	fix			xDisableTime;   // Time until center disabled.
	//CObject  *last_created_obj;
	//int32_t     last_created_sig;
	CFixVector	vCenter;
	bool			bAssigned;
} __pack__ tProducerInfo;

// The max number of robot centers per mine.

typedef struct  {
	int32_t		objFlags;    		// Up to 32 different robots
	fix			xHitPoints;     	// How hard it is to destroy this particular object producer
	fix			xInterval;       	// Interval between materialogrifizations
	int16_t		nSegment;         // Segment this is attached to.
	int16_t		nProducer;    		// Index in producer array.
} __pack__ old_tObjProducerInfo;

typedef struct tObjectProducerInfo {
	int32_t		objFlags [3]; 		// Up to 92 different robots
	fix			xHitPoints;     	// How hard it is to destroy this particular object producer
	fix			xInterval;       	// Interval between materializations
	int16_t		nSegment;         // Segment this is attached to.
	int16_t		nProducer;    		// Index in producer array.
	bool			bAssigned;
} __pack__ tObjectProducerInfo;

//--repair-- extern CObject *RepairObj;  // which CObject getting repaired, or NULL

// Called when a materialization center gets triggered by the player
// flying through some CTrigger!
int32_t StartObjectProducer (int16_t nSegment);
void DisableObjectProducers (void);
void InitAllObjectProducers (void);
void OperateRobotMaker (CObject *pObj, int16_t nSegment);
void SetEquipmentMakerStates (void);

void OldReadObjectProducerInfo(old_tObjProducerInfo *mi, CFile& cf);
void ReadObjectProducerInfo (tObjectProducerInfo *ps, CFile& cf, bool bOldFormat);

void ResetGenerators (void);
void UpdateAllProducers (void);

#define PRODUCER_IDX(_pProducer)	((int16_t) ((_pProducer) - gameData.producers.producers.Buffer ()))
#define OBJECT_PRODUCER_IDX(_pProducer)	((int16_t) ((_pObjProducer) - gameData.producers.producers.Buffer ()))

#endif
