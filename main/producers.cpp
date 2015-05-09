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
	int32_t i;

for (i = 0; i < LEVEL_SEGMENTS; i++)
	SEGMENT (i)->m_function = SEGMENT_FUNC_NONE;
gameData.producers.nProducers = 0;
gameData.producers.nRobotMakers = 0;
gameData.producers.nEquipmentMakers = 0;
gameData.producers.nRepairCenters = 0;
}

//------------------------------------------------------------
// Turns a CSegment into a fully charged up fuel center...
bool CSegment::CreateProducer (int32_t nOldFunction)
{
if ((m_function != SEGMENT_FUNC_FUELCENTER) &&
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
	gameData.producers.origProducerTypes [m_value] = SEGMENT_FUNC_NONE;
	if (m_nObjProducer < --gameData.producers.nEquipmentMakers) {
		gameData.producers.equipmentMakers [m_nObjProducer] = gameData.producers.equipmentMakers [gameData.producers.nEquipmentMakers];
		SEGMENT (gameData.producers.equipmentMakers [gameData.producers.nEquipmentMakers].nSegment)->m_nObjProducer = m_nObjProducer;
		m_nObjProducer = -1;
		}
	}
else if (nOldFunction == SEGMENT_FUNC_ROBOTMAKER) {
	gameData.producers.origProducerTypes [m_value] = SEGMENT_FUNC_NONE;
	if (m_nObjProducer < --gameData.producers.nRobotMakers) {
		gameData.producers.robotMakers [m_nObjProducer] = gameData.producers.robotMakers [gameData.producers.nRobotMakers];
		SEGMENT (gameData.producers.robotMakers [gameData.producers.nRobotMakers].nSegment)->m_nObjProducer = m_nObjProducer;
		m_nObjProducer = -1;
		}
	}
return true;
}

//------------------------------------------------------------

bool CSegment::CreateObjectProducer (int32_t nOldFunction, int32_t nMaxCount)
{
if ((nMaxCount > 0) && (m_nObjProducer >= nMaxCount)) {
	m_function = SEGMENT_FUNC_NONE;
	m_nObjProducer = -1;
	return false;
	}

if ((nOldFunction != SEGMENT_FUNC_FUELCENTER) && 
	 (nOldFunction != SEGMENT_FUNC_REPAIRCENTER) && 
	 (nOldFunction != SEGMENT_FUNC_ROBOTMAKER) && 
	 (nOldFunction != SEGMENT_FUNC_EQUIPMAKER)) {
	if (gameData.producers.nProducers >= MAX_FUEL_CENTERS)
		return false;
	m_value = gameData.producers.nProducers++; // hasn't already been a producer, so allocate a new one
	}

gameData.producers.origProducerTypes [m_value] = (nOldFunction == m_function) ? SEGMENT_FUNC_NONE : nOldFunction;
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
bool CSegment::CreateRobotMaker (int32_t nOldFunction)
{
m_nObjProducer = gameData.producers.nRobotMakers;
if (!CreateObjectProducer (nOldFunction, gameFileInfo.botGen.count))
	return false;
++gameData.producers.nRobotMakers;
tObjectProducerInfo& producer = gameData.producers.robotMakers [m_nObjProducer];
producer.xHitPoints = OBJ_PRODUCER_HP_DEFAULT;
producer.xInterval = OBJ_PRODUCER_INTERVAL_DEFAULT;
producer.nSegment = Index ();
producer.nProducer = m_value;
producer.bAssigned = false;
return true;
}


//------------------------------------------------------------
// Adds a producer that already is a special type into the gameData.producers.producers array.
// This function is separate from other producers because we don't want values reset.
bool CSegment::CreateEquipmentMaker (int32_t nOldFunction)
{
m_nObjProducer = gameData.producers.nEquipmentMakers;
if (!CreateObjectProducer (nOldFunction, gameFileInfo.equipGen.count))
	return false;
++gameData.producers.nEquipmentMakers;
tObjectProducerInfo& producer = gameData.producers.equipmentMakers [m_nObjProducer];
producer.xHitPoints = OBJ_PRODUCER_HP_DEFAULT;
producer.xInterval = OBJ_PRODUCER_INTERVAL_DEFAULT;
producer.nSegment = Index ();
producer.nProducer = m_value;
producer.bAssigned = false;
return true;
}

//------------------------------------------------------------

void SetEquipmentMakerStates (void)
{
for (int32_t i = 0; i < gameData.producers.nEquipmentMakers; i++)
	gameData.producers.producers [gameData.producers.equipmentMakers [i].nProducer].bEnabled =
		FindTriggerTarget (gameData.producers.producers [i].nSegment, -1) == 0;
}

//------------------------------------------------------------
// Adds a CSegment that already is a special nType into the gameData.producers.producers array.
bool CSegment::CreateGenerator (int32_t nType)
{
m_function = nType;
if (m_function == SEGMENT_FUNC_ROBOTMAKER)
	return CreateRobotMaker (SEGMENT_FUNC_NONE);
else if (m_function == SEGMENT_FUNC_EQUIPMAKER)
	return CreateEquipmentMaker (SEGMENT_FUNC_NONE);
else if ((m_function == SEGMENT_FUNC_FUELCENTER) || (m_function == SEGMENT_FUNC_REPAIRCENTER)) {
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
int32_t StartObjectProducer (int16_t nSegment)
{
	CSegment*		pSeg = SEGMENT (nSegment);
	CFixVector		pos, delta;
	tProducerInfo	*pObjProducer;
	int32_t			nObject;

if (pSeg->m_nObjProducer < 0)
	return 0;
if (pSeg->m_function == SEGMENT_FUNC_EQUIPMAKER) {	// toggle it on or off
	if (pSeg->m_nObjProducer >= gameData.producers.nEquipmentMakers)
		return 0;
	pObjProducer = gameData.producers.producers + gameData.producers.equipmentMakers [pSeg->m_nObjProducer].nProducer;
	return (pObjProducer->bEnabled = !pObjProducer->bEnabled) ? 1 : 2;
	}
if (pSeg->m_nObjProducer >= gameData.producers.nRobotMakers)
	return 0;
pObjProducer = gameData.producers.producers + gameData.producers.robotMakers [pSeg->m_nObjProducer].nProducer;
if (pObjProducer->bEnabled)
	return 0;
//	MK: 11/18/95, At insane, object producers work forever!
if (gameStates.app.bD1Mission || (gameStates.app.nDifficultyLevel + 1 < DIFFICULTY_LEVEL_COUNT)) {
	if (!pObjProducer->nLives)
		return 0;
	--pObjProducer->nLives;
	}

pObjProducer->xTimer = I2X (1000);	//	Make sure the first robot gets emitted right away.
pObjProducer->bEnabled = 1;			//	Say this center is enabled, it can create robots.
pObjProducer->xCapacity = I2X (gameStates.app.nDifficultyLevel + 3);
pObjProducer->xDisableTime = OBJECT_PRODUCER_LIFE;

//	Create a bright CObject in the CSegment.
pos = pObjProducer->vCenter;
delta = gameData.segData.vertices [SEGMENT (nSegment)->m_vertices [0]] - pObjProducer->vCenter;
pos += delta * (I2X (1)/2);
nObject = CreateLight (SINGLE_LIGHT_ID, nSegment, pos);
if (nObject != -1) {
	OBJECT (nObject)->SetLife (OBJECT_PRODUCER_LIFE);
	OBJECT (nObject)->cType.lightInfo.intensity = I2X (8);	//	Light cast by a producer.
	}
return 0;
}

//	----------------------------------------------------------------------------------------------------------

int32_t GetObjProducerObjType (tProducerInfo *pObjProducer, int32_t *objFlags, int32_t maxType)
{
	int32_t	i, nObjIndex, nTypes = 0;
	uint32_t	flags;
	uint8_t	objTypes [64];

memset (objTypes, 0, sizeof (objTypes));
for (i = 0; i < 3; i++) {
	nObjIndex = i * 32;
	flags = (uint32_t) objFlags [i];
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
void OperateRobotMaker (CObject *pObj, int16_t nSegment)
{
	CSegment*		pSeg = SEGMENT (nSegment);
	tProducerInfo*	pObjProducer;
	int16_t				nType;

if (nSegment < 0)
	nType = 255;
else {
	pObjProducer = gameData.producers.producers + gameData.producers.robotMakers [pSeg->m_nObjProducer].nProducer;
	nType = GetObjProducerObjType (pObjProducer, gameData.producers.robotMakers [pSeg->m_nObjProducer].objFlags, MAX_ROBOT_TYPES);
	if (nType < 0)
		nType = 255;
	}
pObj->BossSpewRobot (NULL, nType, 1);
}

//	----------------------------------------------------------------------------------------------------------

CObject *CreateMorphRobot (CSegment *pSeg, CFixVector *vObjPosP, uint8_t nObjId)
{
	int16_t		nObject;
	CObject		*pObj;
	tRobotInfo	*pRobotInfo;
	uint8_t		default_behavior;

LOCALPLAYER.numRobotsLevel++;
LOCALPLAYER.numRobotsTotal++;
nObject = CreateRobot (nObjId, pSeg->Index (), *vObjPosP);
pObj = OBJECT (nObject);
if (!pObj) {
#if TRACE
	console.printf (1, "Can't create morph robot.  Aborting morph.\n");
#endif
	Int3 ();
	return NULL;
	}
//Set polygon-CObject-specific data
pRobotInfo = ROBOTINFO (pObj);
if (!pRobotInfo) {
	ReleaseObject (nObject);
	return NULL;
	}
pObj->rType.polyObjInfo.nModel = pRobotInfo->nModel;
pObj->rType.polyObjInfo.nSubObjFlags = 0;
//set Physics info
pObj->mType.physInfo.mass = pRobotInfo->mass;
pObj->mType.physInfo.drag = pRobotInfo->drag;
pObj->mType.physInfo.flags |= (PF_LEVELLING);
pObj->SetShield (RobotDefaultShield (pObj));
default_behavior = pRobotInfo->behavior;
if (pObj->IsBoss ())
	gameData.bosses.Add (nObject);
InitAIObject (pObj->Index (), default_behavior, -1);		//	Note, -1 = CSegment this robot goes to to hide, should probably be something useful
CreateNSegmentPath (pObj, 6, -1);		//	Create a 6 CSegment path from creation point.
gameData.ai.localInfo [nObject].mode = AIBehaviorToMode (default_behavior);
return pObj;
}

int32_t Num_extryRobots = 15;

#if DBG
int32_t	FrameCount_last_msg = 0;
#endif

//	----------------------------------------------------------------------------------------------------------

void CreateObjectProducerEffect (tProducerInfo *pObjProducer, uint8_t nVideoClip)
{
CFixVector vPos = SEGMENT (pObjProducer->nSegment)->Center ();
// HACK!!!The 10 under here should be something equal to the 1/2 the size of the CSegment.
CObject* pObj = CreateExplosion ((int16_t) pObjProducer->nSegment, vPos, I2X (10), nVideoClip);
if (pObj) {
	ExtractOrientFromSegment (&pObj->info.position.mOrient, SEGMENT (pObjProducer->nSegment));
	if (gameData.effects.animations [0][nVideoClip].nSound > -1)
		audio.CreateSegmentSound (gameData.effects.animations [0][nVideoClip].nSound, (int16_t) pObjProducer->nSegment, 0, vPos, 0, I2X (1));
	pObjProducer->bFlag	= 1;
	pObjProducer->xTimer = 0;
	}
}

//	----------------------------------------------------------------------------------------------------------

void EquipmentMakerHandler (tProducerInfo * pObjProducer)
{
	int32_t		nObject, nObjProducer, nType;
	CObject		*pObj;
	CFixVector	vPos;
	fix			topTime;

if (!pObjProducer->bEnabled)
	return;
nObjProducer = SEGMENT (pObjProducer->nSegment)->m_nObjProducer;
if (nObjProducer == -1) {
#if TRACE
	console.printf (CON_DBG, "Dysfunctional robot generator at %d\n", pObjProducer->nSegment);
#endif
	return;
	}
pObjProducer->xTimer += gameData.time.xFrame;
if (!pObjProducer->bFlag) {
	topTime = EQUIP_GEN_TIME;
	if (pObjProducer->xTimer < topTime)
		return;
	nObject = SEGMENT (pObjProducer->nSegment)->m_objects;
	while (nObject >= 0) {
		pObj = OBJECT (nObject);
		if ((pObj->info.nType == OBJ_POWERUP) || (pObj->info.nId == OBJ_PLAYER)) {
			pObjProducer->xTimer = 0;
			return;
			}
		nObject = pObj->info.nNextInSeg;
		}
	CreateObjectProducerEffect (pObjProducer, ANIM_MORPHING_ROBOT);
	}
else if (pObjProducer->bFlag == 1) {			// Wait until 1/2 second after VCLIP started.
	if (pObjProducer->xTimer < (gameData.effects.animations [0][ANIM_MORPHING_ROBOT].xTotalTime / 2))
		return;
	pObjProducer->bFlag = 0;
	pObjProducer->xTimer = 0;
	nType = GetObjProducerObjType (pObjProducer, gameData.producers.equipmentMakers [nObjProducer].objFlags, MAX_POWERUP_TYPES);
	if (nType < 0)
		return;
	vPos = SEGMENT (pObjProducer->nSegment)->Center ();
	// If this is the first materialization, set to valid robot.
	nObject = CreatePowerup (nType, -1, (int16_t) pObjProducer->nSegment, vPos, 1, true);
	pObj = OBJECT (nObject);
	if (!pObj)
		return;
	if (IsMultiGame) {
		gameData.multiplayer.maxPowerupsAllowed [nType]++;
		gameData.multigame.create.nObjNums [gameData.multigame.create.nCount++] = nObject;
		}
	pObj->rType.animationInfo.nClipIndex = gameData.objData.pwrUp.info [pObj->info.nId].nClipIndex;
	pObj->rType.animationInfo.xFrameTime = gameData.effects.animations [0][pObj->rType.animationInfo.nClipIndex].xFrameTime;
	pObj->rType.animationInfo.nCurFrame = 0;
	pObj->info.nCreator = SEGMENT (pObjProducer->nSegment)->m_owner;
	pObj->SetLife (IMMORTAL_TIME);
	}
else {
	pObjProducer->bFlag = 0;
	pObjProducer->xTimer = 0;
	}
}

//	----------------------------------------------------------------------------------------------------------

void VirusGenHandler (tProducerInfo * pObjProducer)
{
	int32_t			nObject, nObjProducer;
	CObject		*pObj;
	CFixVector	vPos;
	fix			topTime;

if (gameStates.entropy.bExitSequence || (SEGMENT (pObjProducer->nSegment)->m_owner <= 0))
	return;
nObjProducer = SEGMENT (pObjProducer->nSegment)->m_nObjProducer;
if (nObjProducer == -1) {
#if TRACE
	console.printf (CON_DBG, "Dysfunctional robot generator at %d\n", pObjProducer->nSegment);
#endif
	return;
	}
pObjProducer->xTimer += gameData.time.xFrame;
if (!pObjProducer->bFlag) {
	topTime = I2X (extraGameInfo [1].entropy.nVirusGenTime);
	if (pObjProducer->xTimer < topTime)
		return;
	nObject = SEGMENT (pObjProducer->nSegment)->m_objects;
	while (nObject >= 0) {
		pObj = OBJECT (nObject);
		if ((pObj->info.nType == OBJ_POWERUP) && (pObj->info.nId == POW_ENTROPY_VIRUS)) {
			pObjProducer->xTimer = 0;
			return;
			}
		nObject = pObj->info.nNextInSeg;
		}
	CreateObjectProducerEffect (pObjProducer, ANIM_POWERUP_DISAPPEARANCE);
	}
else if (pObjProducer->bFlag == 1) {			// Wait until 1/2 second after VCLIP started.
	if (pObjProducer->xTimer < (gameData.effects.animations [0][ANIM_POWERUP_DISAPPEARANCE].xTotalTime / 2))
		return;
	pObjProducer->bFlag = 0;
	pObjProducer->xTimer = 0;
	vPos = SEGMENT (pObjProducer->nSegment)->Center ();
	// If this is the first materialization, set to valid robot.
	nObject = CreatePowerup (POW_ENTROPY_VIRUS, -1, (int16_t) pObjProducer->nSegment, vPos, 1);
	if (nObject >= 0) {
		pObj = OBJECT (nObject);
		if (IsMultiGame)
			gameData.multigame.create.nObjNums [gameData.multigame.create.nCount++] = nObject;
		pObj->rType.animationInfo.nClipIndex = gameData.objData.pwrUp.info [pObj->info.nId].nClipIndex;
		pObj->rType.animationInfo.xFrameTime = gameData.effects.animations [0][pObj->rType.animationInfo.nClipIndex].xFrameTime;
		pObj->rType.animationInfo.nCurFrame = 0;
		pObj->info.nCreator = SEGMENT (pObjProducer->nSegment)->m_owner;
		pObj->SetLife (IMMORTAL_TIME);
		}
	}
else {
	pObjProducer->bFlag = 0;
	pObjProducer->xTimer = 0;
	}
}

//	----------------------------------------------------------------------------------------------------------

inline int32_t VertigoObjFlags (tObjectProducerInfo *pInfo)
{
return pInfo->objFlags [2] = gameData.objData.nVertigoBotFlags;
}

//	----------------------------------------------------------------------------------------------------------

void RobotMakerHandler (tProducerInfo * pObjProducer)
{
	fix			xDistToPlayer;
	CFixVector	vPos, vDir;
	int32_t		nObjProducer, nSegment, nObject;
	CObject*		pObj;
	fix			topTime;
	int32_t		nType, nMyStation, nCount;

if (gameStates.app.bGameSuspended & SUSP_ROBOTS)
	return;
if (!pObjProducer->bEnabled)
	return;
#if DBG
if (pObjProducer->nSegment == nDbgSeg)
	BRP;
#endif
if (pObjProducer->xDisableTime > 0) {
	pObjProducer->xDisableTime -= gameData.time.xFrame;
	if (pObjProducer->xDisableTime <= 0) {
#if TRACE
		console.printf (CON_DBG, "Robot center #%i gets disabled due to time running out.\n",PRODUCER_IDX (pObjProducer));
#endif
		pObjProducer->bEnabled = 0;
		}
	}
//	No robot making in multiplayer mode.
if (IsMultiGame && (!gameData.app.GameMode (GM_MULTI_ROBOTS) || !IAmGameHost ()))
	return;
// Wait until transmorgafier has capacity to make a robot...
if (pObjProducer->xCapacity <= 0)
	return;
nObjProducer = SEGMENT (pObjProducer->nSegment)->m_nObjProducer;
if (nObjProducer == -1) {
#if TRACE
	console.printf (CON_DBG, "Dysfunctional robot generator at %d\n", pObjProducer->nSegment);
#endif
	return;
	}
if (!(gameData.producers.robotMakers [nObjProducer].objFlags [0] ||
		gameData.producers.robotMakers [nObjProducer].objFlags [1] ||
		VertigoObjFlags (gameData.producers.robotMakers + nObjProducer)))
	return;

// Wait until we have a free slot for this puppy...
if (/*!gameStates.app.bD2XLevel &&*/ (LOCALPLAYER.RemainingRobots () >= gameData.objData.nInitialRobots + MAX_EXCESS_OBJECTS)) {
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
pObjProducer->xTimer += gameData.time.xFrame;
if (!pObjProducer->bFlag) {
	if (IsMultiGame)
		topTime = ROBOT_GEN_TIME;
	else {
		xDistToPlayer = CFixVector::Dist(gameData.objData.pConsole->info.position.vPos, pObjProducer->vCenter);
		topTime = xDistToPlayer / 64 + RandShort () * 2 + I2X (2);
		if (topTime > ROBOT_GEN_TIME)
			topTime = ROBOT_GEN_TIME + RandShort ();
		if (topTime < I2X (2))
			topTime = I2X (3)/2 + RandShort ()*2;
		}
	if (pObjProducer->xTimer < topTime)
		return;
	nMyStation = PRODUCER_IDX (pObjProducer);

	//	Make sure this robotmaker hasn't put out its max without having any of them killed.
	nCount = 0;
	FORALL_ROBOT_OBJS (pObj)
		if ((pObj->info.nCreator ^ 0x80) == nMyStation)
			nCount++;
	if (nCount > gameStates.app.nDifficultyLevel + 3) {
#if TRACE
		console.printf (CON_DBG, "Cannot morph: center %i has already put out %i robots.\n", nMyStation, nCount);
#endif
		pObjProducer->xTimer /= 2;
		return;
		}
	nCount = 0;
	nSegment = pObjProducer->nSegment;
#if DBG
	if (nSegment == nDbgSeg)
		BRP;
#endif
	for (nObject = SEGMENT (nSegment)->m_objects; nObject != -1; nObject = OBJECT (nObject)->info.nNextInSeg) {
		nCount++;
		if (nCount > LEVEL_OBJECTS) {
#if TRACE
			console.printf (CON_DBG, "Object list in CSegment %d is circular.", nSegment);
#endif
			Int3 ();
			return;
			}
		if (OBJECT (nObject)->info.nType == OBJ_ROBOT) {
			OBJECT (nObject)->CollideRobotAndObjProducer ();
			pObjProducer->xTimer = topTime / 2;
			return;
			}
		else if (OBJECT (nObject)->info.nType == OBJ_PLAYER) {
			OBJECT (nObject)->CollidePlayerAndObjProducer ();
			pObjProducer->xTimer = topTime / 2;
			return;
			}
		}
	CreateObjectProducerEffect (pObjProducer, ANIM_MORPHING_ROBOT);
	}
else if (pObjProducer->bFlag == 1) {			// Wait until 1/2 second after VCLIP started.
	if (pObjProducer->xTimer <= (gameData.effects.animations [0][ANIM_MORPHING_ROBOT].xTotalTime / 2))
		return;
	pObjProducer->xCapacity -= gameData.producers.xEnergyToCreateOneRobot;
	pObjProducer->bFlag = 0;
	pObjProducer->xTimer = 0;
	vPos = SEGMENT (pObjProducer->nSegment)->Center ();
	// If this is the first materialization, set to valid robot.
	nType = (int32_t) GetObjProducerObjType (pObjProducer, gameData.producers.robotMakers [nObjProducer].objFlags, MAX_ROBOT_TYPES);
	if (nType < 0)
		return;
#if TRACE
	console.printf (CON_DBG, "Morph: (nType = %i) (seg = %i) (capacity = %08x)\n", nType, pObjProducer->nSegment, pObjProducer->xCapacity);
#endif
	if (!(pObj = CreateMorphRobot (SEGMENT (pObjProducer->nSegment), &vPos, nType))) {
#if TRACE
		console.printf (CON_DBG, "Warning: CreateMorphRobot returned NULL (no OBJECTS left?)\n");
#endif
		return;
		}
	if (IsMultiGame)
		MultiSendCreateRobot (PRODUCER_IDX (pObjProducer), pObj->Index (), nType);
	pObj->info.nCreator = (PRODUCER_IDX (pObjProducer)) | 0x80;
	// Make object face player...
	vDir = gameData.objData.pConsole->info.position.vPos - pObj->info.position.vPos;
	pObj->info.position.mOrient = CFixMatrix::CreateFU(vDir, pObj->info.position.mOrient.m.dir.u);
	//pObj->info.position.mOrient = CFixMatrix::CreateFU(vDir, &pObj->info.position.mOrient.m.v.u, NULL);
	pObj->MorphStart ();
	}
else {
	pObjProducer->bFlag = 0;
	pObjProducer->xTimer = 0;
	}
}


//-------------------------------------------------------------
// Called once per frame, replenishes fuel supply.
void UpdateAllProducers (void)
{
	int32_t				i, t;
	tProducerInfo*	pProducer = &gameData.producers.producers [0];
	fix				xAmountToReplenish = FixMul (gameData.time.xFrame, gameData.producers.xFuelRefillSpeed);

for (i = 0; i < gameData.producers.nProducers; i++, pProducer++) {
	t = pProducer->nType;
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
	else if ((pProducer->xMaxCapacity > 0) &&
				(gameData.producers.playerSegP != SEGMENT (pProducer->nSegment))) {
		if (pProducer->xCapacity < pProducer->xMaxCapacity) {
 			pProducer->xCapacity += xAmountToReplenish;
			if (pProducer->xCapacity >= pProducer->xMaxCapacity) {
				pProducer->xCapacity = pProducer->xMaxCapacity;
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
int32_t bEntropy = !m_xDamage [0];
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
if (m_function != SEGMENT_FUNC_FUELCENTER)
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
for (int32_t i = 0; i < gameData.producers.nRobotMakers; i++) {
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
for (int32_t i = 0; i < gameData.producers.nProducers; i++)
	if (gameData.producers.producers [i].nType == SEGMENT_FUNC_ROBOTMAKER) {
		 gameData.producers.producers [i].nLives = 3;
		 gameData.producers.producers [i].bEnabled = 0;
		 gameData.producers.producers [i].xDisableTime = 0;
		 }
}

//-----------------------------------------------------------------------------

int16_t flagGoalList [MAX_SEGMENTS_D2X];
int16_t flagGoalRoots [2] = {-1, -1};
int16_t blueFlagGoals = -1;

// create linked lists of all segments with special nType blue and red goal

int32_t GatherFlagGoals (void)
{
	int32_t			h, i, j;
	CSegment*	pSeg = SEGMENTS.Buffer ();

memset (flagGoalList, 0xff, sizeof (flagGoalList));
for (h = i = 0; i <= gameData.segData.nLastSegment; i++, pSeg++) {
	if (pSeg->m_function == SEGMENT_FUNC_GOAL_BLUE) {
		j = 0;
		h |= 1;
		}
	else if (pSeg->m_function == SEGMENT_FUNC_GOAL_RED) {
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

void MultiSendCaptureBonus (uint8_t);

int32_t FlagAtHome (int32_t nFlagId)
{
	int32_t		i, j;
	CObject	*pObj;

for (i = flagGoalRoots [nFlagId - POW_BLUEFLAG]; i >= 0; i = flagGoalList [i])
	for (j = SEGMENT (i)->m_objects; j >= 0; j = pObj->info.nNextInSeg) {
		pObj = OBJECT (j);
		if ((pObj->info.nType == OBJ_POWERUP) && (pObj->info.nId == nFlagId))
			return 1;
		}
return 0;
}

//--------------------------------------------------------------------

int32_t CSegment::CheckFlagDrop (int32_t nTeamId, int32_t nFlagId, int32_t nGoalId)
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
MultiSendCaptureBonus (N_LOCALPLAYER);
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
if (pSeg->m_function == SEGMENT_FUNC_GOAL_BLUE) {
	if (GetTeam (N_LOCALPLAYER) == TEAM_BLUE) && FlagAtHome (POW_BLUEFLAG)) {
		MultiSendCaptureBonus (N_LOCALPLAYER);
		LOCALPLAYER.flags &= (~(PLAYER_FLAGS_FLAG);
		MaybeDropNetPowerup (-1, POW_REDFLAG, FORCE_DROP);
		}
	}
else if (pSeg->m_function == SEGMENT_FUNC_GOAL_RED) {
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
MultiSendOrbBonus (N_LOCALPLAYER);
LOCALPLAYER.flags &= (~(PLAYER_FLAGS_FLAG));
LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX]=0;
}

//--------------------------------------------------------------------
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

//--------------------------------------------------------------------

tObjectProducerInfo* CProducerData::Find (int16_t nSegment, CStaticArray< tObjectProducerInfo, MAX_ROBOT_CENTERS >& producers)
{
for (uint32_t i = 0; i < producers.Length (); i++)
	if (producers [i].nSegment == nSegment)
		return producers + i;
return NULL;
}

//--------------------------------------------------------------------

tObjectProducerInfo* CProducerData::Find (int16_t nSegment)
{
	tObjectProducerInfo* p = Find (nSegment, robotMakers);

return p ? p : Find (nSegment, equipmentMakers);
}

//--------------------------------------------------------------------
