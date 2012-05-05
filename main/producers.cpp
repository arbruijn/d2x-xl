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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "descent.h"
#include "segmath.h"
#include "error.h"
#include "cockpit.h"
#include "fireball.h"
#include "loadobjects.h"
#include "collide.h"
#include "network.h"
#include "multibot.h"
#include "escort.h"
#include "dropobject.h"

// The max number of fuel stations per mine.

// Every time a robot is created in the morphing code, it decreases capacity of the morpher
// by this amount... when capacity gets to 0, no more morphers...

#define	ROBOT_GEN_TIME (I2X (5))
#define	EQUIP_GEN_TIME (I2X (3) * (gameStates.app.nDifficultyLevel + 1))

#define OBJ_PRODUCER_HP_DEFAULT			I2X (500) // Hitpoints
#define OBJ_PRODUCER_INTERVAL_DEFAULT	I2X (5)	//  5 seconds

//------------------------------------------------------------
// Resets all fuel center info
void ResetGenerators (void)
{
	int i;

for (i = 0; i < LEVEL_SEGMENTS; i++)
	SEGMENTS [i].m_function = SEGMENT_FUNC_NONE;
gameData.producers.nProducers = 0;
gameData.producers.nBotCenters = 0;
gameData.producers.nEquipCenters = 0;
gameData.producers.nRepairCenters = 0;
}

//------------------------------------------------------------
// Turns a CSegment into a fully charged up fuel center...
bool CSegment::CreateProducer (int nOldFunction)
{
if ((m_function != SEGMENT_FUNC_PRODUCERTER) &&
	 (m_function != SEGMENT_FUNC_REPAIRCENTER) &&
	 (m_function != SEGMENT_FUNC_REACTOR) &&
	 (m_function != SEGMENT_FUNC_ROBOTMAKER) &&
	 (m_function != SEGMENT_FUNC_EQUIPMAKER)) {
	PrintLog (0, "Segment %d has invalid function %d in producers.cpp\n", Index (), m_function);
	return false;
	}

if (!CreateObjectProducer (nOldFunction, MAX_FUEL_CENTERS))
	return false;

if (nOldFunction == SEGMENT_FUNC_EQUIPMAKER) {
	gameData.producers.origStationTypes [m_value] = SEGMENT_FUNC_NONE;
	if (m_nObjProducer < --gameData.producers.nEquipCenters) {
		gameData.producers.equipmentMakers [m_nObjProducer] = gameData.producers.equipmentMakers [gameData.producers.nEquipCenters];
		SEGMENTS [gameData.producers.equipmentMakers [gameData.producers.nEquipCenters].nSegment].m_nObjProducer = m_nObjProducer;
		m_nObjProducer = -1;
		}
	}
else if (nOldFunction == SEGMENT_FUNC_ROBOTMAKER) {
	gameData.producers.origStationTypes [m_value] = SEGMENT_FUNC_NONE;
	if (m_nObjProducer < --gameData.producers.nBotCenters) {
		gameData.producers.robotMakers [m_nObjProducer] = gameData.producers.robotMakers [gameData.producers.nBotCenters];
		SEGMENTS [gameData.producers.robotMakers [gameData.producers.nBotCenters].nSegment].m_nObjProducer = m_nObjProducer;
		m_nObjProducer = -1;
		}
	}
return true;
}

//------------------------------------------------------------

bool CSegment::CreateObjectProducer (int nOldFunction, int nMaxCount)
{
if ((nMaxCount > 0) && (m_nObjProducer >= nMaxCount)) {
	m_function = SEGMENT_FUNC_NONE;
	m_nObjProducer = -1;
	return false;
	}

if ((nOldFunction != SEGMENT_FUNC_PRODUCERTER) && 
	 (nOldFunction != SEGMENT_FUNC_REPAIRCENTER) && 
	 (nOldFunction != SEGMENT_FUNC_ROBOTMAKER) && 
	 (nOldFunction != SEGMENT_FUNC_EQUIPMAKER)) {
	if (gameData.producers.nProducers >= MAX_FUEL_CENTERS)
		return false;
	m_value = gameData.producers.nProducers++; // hasn't already been a producer, so allocate a new one
	}

gameData.producers.origStationTypes [m_value] = (nOldFunction == m_function) ? SEGMENT_FUNC_NONE : nOldFunction;
tProducerInfo& producer = gameData.producers.producers [m_value];
producer.nType = m_function;
producer.xMaxCapacity = 
producer.xCapacity = I2X (gameStates.app.nDifficultyLevel + 3);
producer.nSegment = Index ();
producer.xTimer = -1;
producer.bFlag = (m_function == SEGMENT_FUNC_ROBOTMAKER) ? 1 : (m_function == SEGMENT_FUNC_EQUIPMAKER) ? 2 : 0;
producer.vCenter = m_vCenter;
return true;
}

//------------------------------------------------------------
// Adds a producer that already is a special type into the gameData.producers.producers array.
// This function is separate from other producers because we don't want values reset.
bool CSegment::CreateRobotMaker (int nOldFunction)
{
m_nObjProducer = gameData.producers.nBotCenters;
if (!CreateObjectProducer (nOldFunction, gameFileInfo.botGen.count))
	return false;
++gameData.producers.nBotCenters;
gameData.producers.robotMakers [m_nObjProducer].xHitPoints = OBJ_PRODUCER_HP_DEFAULT;
gameData.producers.robotMakers [m_nObjProducer].xInterval = OBJ_PRODUCER_INTERVAL_DEFAULT;
gameData.producers.robotMakers [m_nObjProducer].nSegment = Index ();
gameData.producers.robotMakers [m_nObjProducer].nProducer = m_value;
return true;
}


//------------------------------------------------------------
// Adds a producer that already is a special type into the gameData.producers.producers array.
// This function is separate from other producers because we don't want values reset.
bool CSegment::CreateEquipmentMaker (int nOldFunction)
{
m_nObjProducer = gameData.producers.nEquipCenters;
if (!CreateObjectProducer (nOldFunction, gameFileInfo.equipGen.count))
	return false;
++gameData.producers.nEquipCenters;
gameData.producers.equipmentMakers [m_nObjProducer].xHitPoints = OBJ_PRODUCER_HP_DEFAULT;
gameData.producers.equipmentMakers [m_nObjProducer].xInterval = OBJ_PRODUCER_INTERVAL_DEFAULT;
gameData.producers.equipmentMakers [m_nObjProducer].nSegment = Index ();
gameData.producers.equipmentMakers [m_nObjProducer].nProducer = m_value;
return true;
}

//------------------------------------------------------------

void SetEquipGenStates (void)
{
for (int i = 0; i < gameData.producers.nEquipCenters; i++)
	gameData.producers.producers [gameData.producers.equipmentMakers [i].nProducer].bEnabled =
		FindTriggerTarget (gameData.producers.producers [i].nSegment, -1) == 0;
}

//------------------------------------------------------------
// Adds a CSegment that already is a special nType into the gameData.producers.producers array.
bool CSegment::CreateGenerator (int nType)
{
m_function = nType;
if (m_function == SEGMENT_FUNC_ROBOTMAKER)
	return CreateRobotMaker (SEGMENT_FUNC_NONE);
else if (m_function == SEGMENT_FUNC_EQUIPMAKER)
	return CreateEquipmentMaker (SEGMENT_FUNC_NONE);
else if ((m_function == SEGMENT_FUNC_PRODUCERTER) || (m_function == SEGMENT_FUNC_REPAIRCENTER)) {
	if (!CreateProducer (SEGMENT_FUNC_NONE))
		return false;
	if (m_function == SEGMENT_FUNC_REPAIRCENTER)
		m_nObjProducer = gameData.producers.nRepairCenters++;
	return true;
	}
return false;
}

//	The lower this number is, the more quickly the center can be re-triggered.
//	If it's too low, it can mean all the robots won't be put out, but for about 5
//	robots, that's not real likely.
#define	OBJECT_PRODUCER_LIFE (I2X (30 - 2 * gameStates.app.nDifficultyLevel))

//------------------------------------------------------------
//	Trigger (enable) the materialization center in CSegment nSegment
int StartObjectProducer (short nSegment)
{
	// -- CSegment		*segP = &SEGMENTS [nSegment];
	CSegment*		segP = &SEGMENTS [nSegment];
	CFixVector		pos, delta;
	tProducerInfo	*objProducerP;
	int				nObject;

if (segP->m_nObjProducer < 0)
	return 0;
if (segP->m_function == SEGMENT_FUNC_EQUIPMAKER) {	// toggle it on or off
	if (segP->m_nObjProducer >= gameData.producers.nEquipCenters)
		return 0;
	objProducerP = gameData.producers.producers + gameData.producers.equipmentMakers [segP->m_nObjProducer].nProducer;
	return (objProducerP->bEnabled = !objProducerP->bEnabled) ? 1 : 2;
	}
if (segP->m_nObjProducer >= gameData.producers.nBotCenters)
	return 0;
objProducerP = gameData.producers.producers + gameData.producers.robotMakers [segP->m_nObjProducer].nProducer;
if (objProducerP->bEnabled)
	return 0;
//	MK: 11/18/95, At insane, object producers work forever!
if (gameStates.app.bD1Mission || (gameStates.app.nDifficultyLevel + 1 < NDL)) {
	if (!objProducerP->nLives)
		return 0;
	--objProducerP->nLives;
	}

objProducerP->xTimer = I2X (1000);	//	Make sure the first robot gets emitted right away.
objProducerP->bEnabled = 1;			//	Say this center is enabled, it can create robots.
objProducerP->xCapacity = I2X (gameStates.app.nDifficultyLevel + 3);
objProducerP->xDisableTime = OBJECT_PRODUCER_LIFE;

//	Create a bright CObject in the CSegment.
pos = objProducerP->vCenter;
delta = gameData.segs.vertices [SEGMENTS [nSegment].m_vertices [0]] - objProducerP->vCenter;
pos += delta * (I2X (1)/2);
nObject = CreateLight (SINGLE_LIGHT_ID, nSegment, pos);
if (nObject != -1) {
	OBJECTS [nObject].SetLife (OBJECT_PRODUCER_LIFE);
	OBJECTS [nObject].cType.lightInfo.intensity = I2X (8);	//	Light cast by a producer.
	}
return 0;
}

//	----------------------------------------------------------------------------------------------------------

int GetObjProducerObjType (tProducerInfo *objProducerP, int *objFlags, int maxType)
{
	int	i, nObjIndex, nTypes = 0;
	uint	flags;
	ubyte	objTypes [64];

memset (objTypes, 0, sizeof (objTypes));
for (i = 0; i < 3; i++) {
	nObjIndex = i * 32;
	flags = (uint) objFlags [i];
	while (flags && (nObjIndex < maxType)) {
		if (flags & 1)
			objTypes [nTypes++] = nObjIndex;
		flags >>= 1;
		nObjIndex++;
		}
	}
if (!nTypes)
	return -1;
if (nTypes == 1)
	return objTypes [0];
return objTypes [(RandShort () * nTypes) / 32768];
}

//------------------------------------------------------------
//	Trigger (enable) the materialization center in CSegment nSegment
void OperateRobotMaker (CObject *objP, short nSegment)
{
	CSegment*		segP = &SEGMENTS [nSegment];
	tProducerInfo*	objProducerP;
	short				nType;

if (nSegment < 0)
	nType = 255;
else {
	objProducerP = gameData.producers.producers + gameData.producers.robotMakers [segP->m_nObjProducer].nProducer;
	nType = GetObjProducerObjType (objProducerP, gameData.producers.robotMakers [segP->m_nObjProducer].objFlags, MAX_ROBOT_TYPES);
	if (nType < 0)
		nType = 255;
	}
objP->BossSpewRobot (NULL, nType, 1);
}

//	----------------------------------------------------------------------------------------------------------

CObject *CreateMorphRobot (CSegment *segP, CFixVector *vObjPosP, ubyte nObjId)
{
	short			nObject;
	CObject		*objP;
	tRobotInfo	*botInfoP;
	ubyte			default_behavior;

LOCALPLAYER.numRobotsLevel++;
LOCALPLAYER.numRobotsTotal++;
nObject = CreateRobot (nObjId, segP->Index (), *vObjPosP);
if (nObject < 0) {
#if TRACE
	console.printf (1, "Can't create morph robot.  Aborting morph.\n");
#endif
	Int3 ();
	return NULL;
	}
objP = OBJECTS + nObject;
//Set polygon-CObject-specific data
botInfoP = &ROBOTINFO (objP->info.nId);
objP->rType.polyObjInfo.nModel = botInfoP->nModel;
objP->rType.polyObjInfo.nSubObjFlags = 0;
//set Physics info
objP->mType.physInfo.mass = botInfoP->mass;
objP->mType.physInfo.drag = botInfoP->drag;
objP->mType.physInfo.flags |= (PF_LEVELLING);
objP->SetShield (RobotDefaultShield (objP));
default_behavior = botInfoP->behavior;
if (ROBOTINFO (objP->info.nId).bossFlag)
	gameData.bosses.Add (nObject);
InitAIObject (objP->Index (), default_behavior, -1);		//	Note, -1 = CSegment this robot goes to to hide, should probably be something useful
CreateNSegmentPath (objP, 6, -1);		//	Create a 6 CSegment path from creation point.
gameData.ai.localInfo [nObject].mode = AIBehaviorToMode (default_behavior);
return objP;
}

int Num_extryRobots = 15;

#if DBG
int	FrameCount_last_msg = 0;
#endif

//	----------------------------------------------------------------------------------------------------------

void CreateObjectProducerEffect (tProducerInfo *objProducerP, ubyte nVideoClip)
{
CFixVector vPos = SEGMENTS [objProducerP->nSegment].Center ();
// HACK!!!The 10 under here should be something equal to the 1/2 the size of the CSegment.
CObject* objP = CreateExplosion ((short) objProducerP->nSegment, vPos, I2X (10), nVideoClip);
if (objP) {
	ExtractOrientFromSegment (&objP->info.position.mOrient, SEGMENTS + objProducerP->nSegment);
	if (gameData.effects.vClips [0][nVideoClip].nSound > -1)
		audio.CreateSegmentSound (gameData.effects.vClips [0][nVideoClip].nSound, (short) objProducerP->nSegment, 0, vPos, 0, I2X (1));
	objProducerP->bFlag	= 1;
	objProducerP->xTimer = 0;
	}
}

//	----------------------------------------------------------------------------------------------------------

void EquipmentMakerHandler (tProducerInfo * objProducerP)
{
	int			nObject, nObjProducer, nType;
	CObject		*objP;
	CFixVector	vPos;
	fix			topTime;

if (!objProducerP->bEnabled)
	return;
nObjProducer = SEGMENTS [objProducerP->nSegment].m_nObjProducer;
if (nObjProducer == -1) {
#if TRACE
	console.printf (CON_DBG, "Dysfunctional robot generator at %d\n", objProducerP->nSegment);
#endif
	return;
	}
objProducerP->xTimer += gameData.time.xFrame;
if (!objProducerP->bFlag) {
	topTime = EQUIP_GEN_TIME;
	if (objProducerP->xTimer < topTime)
		return;
	nObject = SEGMENTS [objProducerP->nSegment].m_objects;
	while (nObject >= 0) {
		objP = OBJECTS + nObject;
		if ((objP->info.nType == OBJ_POWERUP) || (objP->info.nId == OBJ_PLAYER)) {
			objProducerP->xTimer = 0;
			return;
			}
		nObject = objP->info.nNextInSeg;
		}
	CreateObjectProducerEffect (objProducerP, VCLIP_MORPHING_ROBOT);
	}
else if (objProducerP->bFlag == 1) {			// Wait until 1/2 second after VCLIP started.
	if (objProducerP->xTimer < (gameData.effects.vClips [0][VCLIP_MORPHING_ROBOT].xTotalTime / 2))
		return;
	objProducerP->bFlag = 0;
	objProducerP->xTimer = 0;
	nType = GetObjProducerObjType (objProducerP, gameData.producers.equipmentMakers [nObjProducer].objFlags, MAX_POWERUP_TYPES);
	if (nType < 0)
		return;
	vPos = SEGMENTS [objProducerP->nSegment].Center ();
	// If this is the first materialization, set to valid robot.
	nObject = CreatePowerup (nType, -1, (short) objProducerP->nSegment, vPos, 1, true);
	if (nObject < 0)
		return;
	objP = OBJECTS + nObject;
	if (IsMultiGame) {
		gameData.multiplayer.maxPowerupsAllowed [nType]++;
		gameData.multigame.create.nObjNums [gameData.multigame.create.nCount++] = nObject;
		}
	objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->info.nId].nClipIndex;
	objP->rType.vClipInfo.xFrameTime = gameData.effects.vClips [0][objP->rType.vClipInfo.nClipIndex].xFrameTime;
	objP->rType.vClipInfo.nCurFrame = 0;
	objP->info.nCreator = SEGMENTS [objProducerP->nSegment].m_owner;
	objP->SetLife (IMMORTAL_TIME);
	}
else {
	objProducerP->bFlag = 0;
	objProducerP->xTimer = 0;
	}
}

//	----------------------------------------------------------------------------------------------------------

void VirusGenHandler (tProducerInfo * objProducerP)
{
	int			nObject, nObjProducer;
	CObject		*objP;
	CFixVector	vPos;
	fix			topTime;

if (gameStates.entropy.bExitSequence || (SEGMENTS [objProducerP->nSegment].m_owner <= 0))
	return;
nObjProducer = SEGMENTS [objProducerP->nSegment].m_nObjProducer;
if (nObjProducer == -1) {
#if TRACE
	console.printf (CON_DBG, "Dysfunctional robot generator at %d\n", objProducerP->nSegment);
#endif
	return;
	}
objProducerP->xTimer += gameData.time.xFrame;
if (!objProducerP->bFlag) {
	topTime = I2X (extraGameInfo [1].entropy.nVirusGenTime);
	if (objProducerP->xTimer < topTime)
		return;
	nObject = SEGMENTS [objProducerP->nSegment].m_objects;
	while (nObject >= 0) {
		objP = OBJECTS + nObject;
		if ((objP->info.nType == OBJ_POWERUP) && (objP->info.nId == POW_ENTROPY_VIRUS)) {
			objProducerP->xTimer = 0;
			return;
			}
		nObject = objP->info.nNextInSeg;
		}
	CreateObjectProducerEffect (objProducerP, VCLIP_POWERUP_DISAPPEARANCE);
	}
else if (objProducerP->bFlag == 1) {			// Wait until 1/2 second after VCLIP started.
	if (objProducerP->xTimer < (gameData.effects.vClips [0][VCLIP_POWERUP_DISAPPEARANCE].xTotalTime / 2))
		return;
	objProducerP->bFlag = 0;
	objProducerP->xTimer = 0;
	vPos = SEGMENTS [objProducerP->nSegment].Center ();
	// If this is the first materialization, set to valid robot.
	nObject = CreatePowerup (POW_ENTROPY_VIRUS, -1, (short) objProducerP->nSegment, vPos, 1);
	if (nObject >= 0) {
		objP = OBJECTS + nObject;
		if (IsMultiGame)
			gameData.multigame.create.nObjNums [gameData.multigame.create.nCount++] = nObject;
		objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->info.nId].nClipIndex;
		objP->rType.vClipInfo.xFrameTime = gameData.effects.vClips [0][objP->rType.vClipInfo.nClipIndex].xFrameTime;
		objP->rType.vClipInfo.nCurFrame = 0;
		objP->info.nCreator = SEGMENTS [objProducerP->nSegment].m_owner;
		objP->SetLife (IMMORTAL_TIME);
		}
	}
else {
	objProducerP->bFlag = 0;
	objProducerP->xTimer = 0;
	}
}

//	----------------------------------------------------------------------------------------------------------

inline int VertigoObjFlags (tObjectProducerInfo *miP)
{
return miP->objFlags [2] = gameData.objs.nVertigoBotFlags;
}

//	----------------------------------------------------------------------------------------------------------

void RobotMakerHandler (tProducerInfo * objProducerP)
{
	fix			xDistToPlayer;
	CFixVector	vPos, vDir;
	int			nObjProducer, nSegment, nObject;
	CObject		*objP;
	fix			topTime;
	int			nType, nMyStation, nCount;
	//int			i;

if (gameStates.app.bGameSuspended & SUSP_ROBOTS)
	return;
if (!objProducerP->bEnabled)
	return;
#if DBG
if (objProducerP->nSegment == nDbgSeg)
	nDbgSeg = nDbgSeg;
#endif
if (objProducerP->xDisableTime > 0) {
	objProducerP->xDisableTime -= gameData.time.xFrame;
	if (objProducerP->xDisableTime <= 0) {
#if TRACE
		console.printf (CON_DBG, "Robot center #%i gets disabled due to time running out.\n",PRODUCER_IDX (objProducerP));
#endif
		objProducerP->bEnabled = 0;
		}
	}
//	No robot making in multiplayer mode.
if (IsMultiGame && (!gameData.app.GameMode (GM_MULTI_ROBOTS) || !IAmGameHost ()))
	return;
// Wait until transmorgafier has capacity to make a robot...
if (objProducerP->xCapacity <= 0)
	return;
nObjProducer = SEGMENTS [objProducerP->nSegment].m_nObjProducer;
if (nObjProducer == -1) {
#if TRACE
	console.printf (CON_DBG, "Dysfunctional robot generator at %d\n", objProducerP->nSegment);
#endif
	return;
	}
if (!(gameData.producers.robotMakers [nObjProducer].objFlags [0] ||
		gameData.producers.robotMakers [nObjProducer].objFlags [1] ||
		VertigoObjFlags (gameData.producers.robotMakers + nObjProducer)))
	return;

// Wait until we have a free slot for this puppy...
if (/*!gameStates.app.bD2XLevel &&*/ (LOCALPLAYER.RemainingRobots () >= gameData.objs.nInitialRobots + MAX_EXCESS_OBJECTS)) {
#if DBG
	if (gameData.app.nFrameCount > FrameCount_last_msg + 20) {
#if TRACE
		console.printf (CON_DBG, "Cannot morph until you kill one!\n");
#endif
		FrameCount_last_msg = gameData.app.nFrameCount;
		}
#endif
	return;
	}
objProducerP->xTimer += gameData.time.xFrame;
if (!objProducerP->bFlag) {
	if (IsMultiGame)
		topTime = ROBOT_GEN_TIME;
	else {
		xDistToPlayer = CFixVector::Dist(gameData.objs.consoleP->info.position.vPos, objProducerP->vCenter);
		topTime = xDistToPlayer / 64 + RandShort () * 2 + I2X (2);
		if (topTime > ROBOT_GEN_TIME)
			topTime = ROBOT_GEN_TIME + RandShort ();
		if (topTime < I2X (2))
			topTime = I2X (3)/2 + RandShort ()*2;
		}
	if (objProducerP->xTimer < topTime)
		return;
	nMyStation = PRODUCER_IDX (objProducerP);

	//	Make sure this robotmaker hasn't put out its max without having any of them killed.
	nCount = 0;
	FORALL_ROBOT_OBJS (objP, i)
		if ((objP->info.nCreator ^ 0x80) == nMyStation)
			nCount++;
	if (nCount > gameStates.app.nDifficultyLevel + 3) {
#if TRACE
		console.printf (CON_DBG, "Cannot morph: center %i has already put out %i robots.\n", nMyStation, nCount);
#endif
		objProducerP->xTimer /= 2;
		return;
		}
	nCount = 0;
	nSegment = objProducerP->nSegment;
#if DBG
	if (nSegment == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	for (nObject = SEGMENTS [nSegment].m_objects; nObject != -1; nObject = OBJECTS [nObject].info.nNextInSeg) {
		nCount++;
		if (nCount > LEVEL_OBJECTS) {
#if TRACE
			console.printf (CON_DBG, "Object list in CSegment %d is circular.", nSegment);
#endif
			Int3 ();
			return;
			}
		if (OBJECTS [nObject].info.nType == OBJ_ROBOT) {
			OBJECTS [nObject].CollideRobotAndObjProducer ();
			objProducerP->xTimer = topTime / 2;
			return;
			}
		else if (OBJECTS [nObject].info.nType == OBJ_PLAYER) {
			OBJECTS [nObject].CollidePlayerAndObjProducer ();
			objProducerP->xTimer = topTime / 2;
			return;
			}
		}
	CreateObjectProducerEffect (objProducerP, VCLIP_MORPHING_ROBOT);
	}
else if (objProducerP->bFlag == 1) {			// Wait until 1/2 second after VCLIP started.
	if (objProducerP->xTimer <= (gameData.effects.vClips [0][VCLIP_MORPHING_ROBOT].xTotalTime / 2))
		return;
	objProducerP->xCapacity -= gameData.producers.xEnergyToCreateOneRobot;
	objProducerP->bFlag = 0;
	objProducerP->xTimer = 0;
	vPos = SEGMENTS [objProducerP->nSegment].Center ();
	// If this is the first materialization, set to valid robot.
	nType = (int) GetObjProducerObjType (objProducerP, gameData.producers.robotMakers [nObjProducer].objFlags, MAX_ROBOT_TYPES);
	if (nType < 0)
		return;
#if TRACE
	console.printf (CON_DBG, "Morph: (nType = %i) (seg = %i) (capacity = %08x)\n", nType, objProducerP->nSegment, objProducerP->xCapacity);
#endif
	if (!(objP = CreateMorphRobot (SEGMENTS + objProducerP->nSegment, &vPos, nType))) {
#if TRACE
		console.printf (CON_DBG, "Warning: CreateMorphRobot returned NULL (no OBJECTS left?)\n");
#endif
		return;
		}
	if (IsMultiGame)
		MultiSendCreateRobot (PRODUCER_IDX (objProducerP), objP->Index (), nType);
	objP->info.nCreator = (PRODUCER_IDX (objProducerP)) | 0x80;
	// Make object face player...
	vDir = gameData.objs.consoleP->info.position.vPos - objP->info.position.vPos;
	objP->info.position.mOrient = CFixMatrix::CreateFU(vDir, objP->info.position.mOrient.m.dir.u);
	//objP->info.position.mOrient = CFixMatrix::CreateFU(vDir, &objP->info.position.mOrient.m.v.u, NULL);
	objP->MorphStart ();
	}
else {
	objProducerP->bFlag = 0;
	objProducerP->xTimer = 0;
	}
}


//-------------------------------------------------------------
// Called once per frame, replenishes fuel supply.
void UpdateAllProducers (void)
{
	int				i, t;
	tProducerInfo*	producerP = &gameData.producers.producers [0];
	fix				xAmountToReplenish = FixMul (gameData.time.xFrame, gameData.producers.xFuelRefillSpeed);

for (i = 0; i < gameData.producers.nProducers; i++, producerP++) {
	t = producerP->nType;
	if (t == SEGMENT_FUNC_ROBOTMAKER) {
		if (IsMultiGame && IsEntropyGame)
			VirusGenHandler (gameData.producers.producers + i);
		else if (!(gameStates.app.bGameSuspended & SUSP_ROBOTS))
			RobotMakerHandler (gameData.producers.producers + i);
		}
	else if (t == SEGMENT_FUNC_EQUIPMAKER) {
		if (!(gameStates.app.bGameSuspended & SUSP_POWERUPS))
			EquipmentMakerHandler (gameData.producers.producers + i);
		}
	else if (t == SEGMENT_FUNC_REACTOR) {
		//controlcen_proc (gameData.producers.producers + i);
		}
	else if ((producerP->xMaxCapacity > 0) &&
				(gameData.producers.playerSegP != SEGMENTS + producerP->nSegment)) {
		if (producerP->xCapacity < producerP->xMaxCapacity) {
 			producerP->xCapacity += xAmountToReplenish;
			if (producerP->xCapacity >= producerP->xMaxCapacity) {
				producerP->xCapacity = producerP->xMaxCapacity;
				//gauge_message ("Fuel center is fully recharged!   ");
				}
			}
		}
	}
}

#define PRODUCER_SOUND_DELAY (I2X (1)/4)		//play every half second

//-------------------------------------------------------------

fix CSegment::ShieldDamage (fix xMaxDamage)
{
	static fix lastPlayTime = 0;

if (!(m_xDamage [0] || IsEntropyGame))
	return 0;
int bEntropy = !m_xDamage [0];
if (bEntropy && ((m_owner < 1) || (m_owner == GetTeam (N_LOCALPLAYER) + 1)))
	return 0;
fix rate = bEntropy ? I2X (extraGameInfo [1].entropy.nShieldDamageRate) : m_xDamage [0];
fix amount = FixMul (gameData.time.xFrame, rate);
if (amount > xMaxDamage)
	amount = xMaxDamage;
if (bEntropy) {
	if (lastPlayTime > gameData.time.xGame)
		lastPlayTime = 0;
	if (gameData.time.xGame > lastPlayTime + PRODUCER_SOUND_DELAY) {
		audio.PlaySound (SOUND_PLAYER_GOT_HIT, SOUNDCLASS_GENERIC, I2X (1) / 2);
		if (IsMultiGame)
			MultiSendPlaySound (SOUND_PLAYER_GOT_HIT, I2X (1) / 2);
		lastPlayTime = gameData.time.xGame;
		}
	}
return amount;
}

//-------------------------------------------------------------

fix CSegment::EnergyDamage (fix xMaxDamage)
{
if (!m_xDamage [1])
	return 0;
fix amount = FixMul (gameData.time.xFrame, m_xDamage [1]);
if (amount > xMaxDamage)
	amount = xMaxDamage;
return amount;
}

//-------------------------------------------------------------

fix CSegment::Refuel (fix nMaxFuel)
{
	fix	amount;

	static fix lastPlayTime = 0;

gameData.producers.playerSegP = this;
if (IsEntropyGame && ((m_owner < 0) ||
	 ((m_owner > 0) && (m_owner != GetTeam (N_LOCALPLAYER) + 1))))
	return 0;
if (m_function != SEGMENT_FUNC_PRODUCERTER)
	return 0;
DetectEscortGoalAccomplished (-4);	//	UGLY!Hack!-4 means went through producer.
if (nMaxFuel <= 0)
	return 0;
if (IsEntropyGame)
	amount = FixMul (gameData.time.xFrame, I2X (gameData.producers.xFuelGiveAmount));
else
	amount = FixMul (gameData.time.xFrame, gameData.producers.xFuelGiveAmount);
if (amount > nMaxFuel)
	amount = nMaxFuel;
if (lastPlayTime > gameData.time.xGame)
	lastPlayTime = 0;
if (gameData.time.xGame > lastPlayTime + PRODUCER_SOUND_DELAY) {
	audio.PlaySound (SOUND_REFUEL_STATION_GIVING_FUEL, SOUNDCLASS_GENERIC, I2X (1) / 2);
	if (IsMultiGame)
		MultiSendPlaySound (SOUND_REFUEL_STATION_GIVING_FUEL, I2X (1) / 2);
	lastPlayTime = gameData.time.xGame;
	}
return amount;
}

//-------------------------------------------------------------
// DM/050904
// Repair centers
// use same values as fuel centers
fix CSegment::Repair (fix nMaxShield)
{
	static fix lastPlayTime = 0;
	fix amount;

if (gameOpts->legacy.bProducers)
	return 0;
gameData.producers.playerSegP = this;
if (IsEntropyGame && ((m_owner < 0) ||
	 ((m_owner > 0) && (m_owner != GetTeam (N_LOCALPLAYER) + 1))))
	return 0;
if (m_function != SEGMENT_FUNC_REPAIRCENTER)
	return 0;
if (nMaxShield <= 0) {
	return 0;
}
amount = FixMul (gameData.time.xFrame, I2X (extraGameInfo [IsMultiGame].entropy.nShieldFillRate));
if (amount > nMaxShield)
	amount = nMaxShield;
if (lastPlayTime > gameData.time.xGame)
	lastPlayTime = 0;
if (gameData.time.xGame > lastPlayTime + PRODUCER_SOUND_DELAY) {
	audio.PlaySound (SOUND_REFUEL_STATION_GIVING_FUEL, SOUNDCLASS_GENERIC, I2X (1)/2);
	if (IsMultiGame)
		MultiSendPlaySound (SOUND_REFUEL_STATION_GIVING_FUEL, I2X (1)/2);
	lastPlayTime = gameData.time.xGame;
	}
return amount;
}

//	--------------------------------------------------------------------------------------------

void DisableObjectProducers (void)
{
for (int i = 0; i < gameData.producers.nBotCenters; i++) {
	if (gameData.producers.producers [i].nType != SEGMENT_FUNC_EQUIPMAKER) {
		gameData.producers.producers [i].bEnabled = 0;
		gameData.producers.producers [i].xDisableTime = 0;
		}
	}
}

//	--------------------------------------------------------------------------------------------
//	Initialize all materialization centers.
//	Give them all the right number of lives.
void InitAllObjectProducers (void)
{
for (int i = 0; i < gameData.producers.nProducers; i++)
	if (gameData.producers.producers [i].nType == SEGMENT_FUNC_ROBOTMAKER) {
		 gameData.producers.producers [i].nLives = 3;
		 gameData.producers.producers [i].bEnabled = 0;
		 gameData.producers.producers [i].xDisableTime = 0;
		 }
}

//-----------------------------------------------------------------------------

short flagGoalList [MAX_SEGMENTS_D2X];
short flagGoalRoots [2] = {-1, -1};
short blueFlagGoals = -1;

// create linked lists of all segments with special nType blue and red goal

int GatherFlagGoals (void)
{
	int			h, i, j;
	CSegment*	segP = SEGMENTS.Buffer ();

memset (flagGoalList, 0xff, sizeof (flagGoalList));
for (h = i = 0; i <= gameData.segs.nLastSegment; i++, segP++) {
	if (segP->m_function == SEGMENT_FUNC_GOAL_BLUE) {
		j = 0;
		h |= 1;
		}
	else if (segP->m_function == SEGMENT_FUNC_GOAL_RED) {
		h |= 2;
		j = 1;
		}
	else
		continue;
	flagGoalList [i] = flagGoalRoots [j];
	flagGoalRoots [j] = i;
	}
return h;
}

//--------------------------------------------------------------------

void MultiSendCaptureBonus (char);

int FlagAtHome (int nFlagId)
{
	int		i, j;
	CObject	*objP;

for (i = flagGoalRoots [nFlagId - POW_BLUEFLAG]; i >= 0; i = flagGoalList [i])
	for (j = SEGMENTS [i].m_objects; j >= 0; j = objP->info.nNextInSeg) {
		objP = OBJECTS + j;
		if ((objP->info.nType == OBJ_POWERUP) && (objP->info.nId == nFlagId))
			return 1;
		}
return 0;
}

//--------------------------------------------------------------------

int CSegment::CheckFlagDrop (int nTeamId, int nFlagId, int nGoalId)
{
if (m_function != nGoalId)
	return 0;
if (GetTeam (N_LOCALPLAYER) != nTeamId)
	return 0;
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_FLAG))
	return 0;
if (gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bEnhancedCTF &&
	 !FlagAtHome ((nFlagId == POW_BLUEFLAG) ? POW_REDFLAG : POW_BLUEFLAG))
	return 0;
MultiSendCaptureBonus (char (N_LOCALPLAYER));
LOCALPLAYER.flags &= (~(PLAYER_FLAGS_FLAG));
MaybeDropNetPowerup (-1, nFlagId, FORCE_DROP);
return 1;
}

//--------------------------------------------------------------------

void CSegment::CheckForGoal (void)
{
	Assert (gameData.app.GameMode (GM_CAPTURE));

#if 1
CheckFlagDrop (TEAM_BLUE, POW_REDFLAG, SEGMENT_FUNC_GOAL_BLUE);
CheckFlagDrop (TEAM_RED, POW_BLUEFLAG, SEGMENT_FUNC_GOAL_RED);
#else
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_FLAG))
	return;
if (segP->m_function == SEGMENT_FUNC_GOAL_BLUE) {
	if (GetTeam (N_LOCALPLAYER) == TEAM_BLUE) && FlagAtHome (POW_BLUEFLAG)) {
		MultiSendCaptureBonus (N_LOCALPLAYER);
		LOCALPLAYER.flags &= (~(PLAYER_FLAGS_FLAG);
		MaybeDropNetPowerup (-1, POW_REDFLAG, FORCE_DROP);
		}
	}
else if (segP->m_function == SEGMENT_FUNC_GOAL_RED) {
	if (GetTeam (N_LOCALPLAYER) == TEAM_RED) && FlagAtHome (POW_REDFLAG)) {
		MultiSendCaptureBonus (N_LOCALPLAYER);
		LOCALPLAYER.flags &= (~(PLAYER_FLAGS_FLAG);
		MaybeDropNetPowerup (-1, POW_BLUEFLAG, FORCE_DROP);
		}
	}
#endif
}

//--------------------------------------------------------------------

void CSegment::CheckForHoardGoal (void)
{
Assert (IsHoardGame);
if (gameStates.app.bPlayerIsDead)
	return;
if ((m_function != SEGMENT_FUNC_GOAL_BLUE) && (m_function != SEGMENT_FUNC_GOAL_RED))
	return;
if (!LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX])
	return;
#if TRACE
console.printf (CON_DBG,"In orb goal!\n");
#endif
MultiSendOrbBonus ((char) N_LOCALPLAYER);
LOCALPLAYER.flags &= (~(PLAYER_FLAGS_FLAG));
LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX]=0;
}

//--------------------------------------------------------------------
#if 1//ndef FAST_FILE_IO /*permanently enabled for a reason!*/
/*
 * reads an old_tObjProducerInfo structure from a CFile
 */
/*
 * reads a tObjectProducerInfo structure from a CFile
 */
void ReadObjectProducerInfo (tObjectProducerInfo *opi, CFile& cf, bool bOldFormat)
{
opi->objFlags [0] = cf.ReadInt ();
opi->objFlags [1] = bOldFormat ? 0 : cf.ReadInt ();
opi->xHitPoints = cf.ReadFix ();
opi->xInterval = cf.ReadFix ();
opi->nSegment = cf.ReadShort ();
opi->nProducer = cf.ReadShort ();
}
#endif

//--------------------------------------------------------------------
