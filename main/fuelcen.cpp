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

#define MATCEN_HP_DEFAULT			I2X (500) // Hitpoints
#define MATCEN_INTERVAL_DEFAULT	I2X (5)	//  5 seconds

//------------------------------------------------------------
// Resets all fuel center info
void ResetGenerators (void)
{
	int i;

for (i = 0; i < LEVEL_SEGMENTS; i++)
	SEGMENTS [i].m_function = SEGMENT_FUNC_NONE;
gameData.matCens.nFuelCenters = 0;
gameData.matCens.nBotCenters = 0;
gameData.matCens.nEquipCenters = 0;
gameData.matCens.nRepairCenters = 0;
}

#if DBG		//this is sometimes called by people from the debugger
void reset_allRobot_centers ()
{
	int i;

	// Remove all materialization centers
for (i = 0; i < gameData.segs.nSegments; i++)
	if ((SEGMENTS [i].m_function == SEGMENT_FUNC_ROBOTMAKER) || (SEGMENTS [i].m_function == SEGMENT_FUNC_EQUIPMAKER)) {
		SEGMENTS [i].m_function = SEGMENT_FUNC_NONE;
		SEGMENTS [i].m_nMatCen = -1;
		}
}
#endif

//------------------------------------------------------------
// Turns a CSegment into a fully charged up fuel center...
bool CSegment::CreateFuelCen (int nOldFunction)
{
if ((m_function != SEGMENT_FUNC_FUELCEN) &&
	 (m_function != SEGMENT_FUNC_REPAIRCEN) &&
	 (m_function != SEGMENT_FUNC_CONTROLCEN) &&
	 (m_function != SEGMENT_FUNC_ROBOTMAKER) &&
	 (m_function != SEGMENT_FUNC_EQUIPMAKER)) {
	PrintLog (0, "Segment %d has invalid function %d in fuelcen.cpp\n", Index (), m_function);
	return false;
	}

if (!CreateMatCen (nOldFunction, MAX_FUEL_CENTERS))
	return false;

if (nOldFunction == SEGMENT_FUNC_EQUIPMAKER) {
	gameData.matCens.origStationTypes [m_value] = SEGMENT_FUNC_NONE;
	if (m_nMatCen < --gameData.matCens.nEquipCenters) {
		gameData.matCens.equipGens [m_nMatCen] = gameData.matCens.equipGens [gameData.matCens.nEquipCenters];
		SEGMENTS [gameData.matCens.equipGens [gameData.matCens.nEquipCenters].nSegment].m_nMatCen = m_nMatCen;
		m_nMatCen = -1;
		}
	}
else if (nOldFunction == SEGMENT_FUNC_ROBOTMAKER) {
	gameData.matCens.origStationTypes [m_value] = SEGMENT_FUNC_NONE;
	if (m_nMatCen < --gameData.matCens.nBotCenters) {
		gameData.matCens.botGens [m_nMatCen] = gameData.matCens.botGens [gameData.matCens.nBotCenters];
		SEGMENTS [gameData.matCens.botGens [gameData.matCens.nBotCenters].nSegment].m_nMatCen = m_nMatCen;
		m_nMatCen = -1;
		}
	}
return true;
}

//------------------------------------------------------------

bool CSegment::CreateMatCen (int nOldFunction, int nMaxCount)
{
if ((nMaxCount > 0) && (m_nMatCen >= nMaxCount)) {
	m_function = SEGMENT_FUNC_NONE;
	m_nMatCen = -1;
	return false;
	}

if ((nOldFunction != SEGMENT_FUNC_FUELCEN) && 
	 (nOldFunction != SEGMENT_FUNC_REPAIRCEN) && 
	 (nOldFunction != SEGMENT_FUNC_ROBOTMAKER) && 
	 (nOldFunction != SEGMENT_FUNC_EQUIPMAKER)) {
	if (gameData.matCens.nFuelCenters >= MAX_FUEL_CENTERS)
		return false;
	m_value = gameData.matCens.nFuelCenters++; // hasn't already been a matcen, so allocate a new one
	}

gameData.matCens.origStationTypes [m_value] = (nOldFunction == m_function) ? SEGMENT_FUNC_NONE : nOldFunction;
tFuelCenInfo& fuelCen = gameData.matCens.fuelCenters [m_value];
fuelCen.nType = m_function;
fuelCen.xMaxCapacity = 
fuelCen.xCapacity = I2X (gameStates.app.nDifficultyLevel + 3);
fuelCen.nSegment = Index ();
fuelCen.xTimer = -1;
fuelCen.bFlag = (m_function == SEGMENT_FUNC_ROBOTMAKER) ? 1 : (m_function == SEGMENT_FUNC_EQUIPMAKER) ? 2 : 0;
fuelCen.vCenter = m_vCenter;
return true;
}

//------------------------------------------------------------
// Adds a matcen that already is a special nType into the gameData.matCens.fuelCenters array.
// This function is separate from other fuelcens because we don't want values reset.
bool CSegment::CreateBotGen (int nOldFunction)
{
m_nMatCen = gameData.matCens.nBotCenters;
if (!CreateMatCen (nOldFunction, gameFileInfo.botGen.count))
	return false;
++gameData.matCens.nBotCenters;
gameData.matCens.botGens [m_nMatCen].xHitPoints = MATCEN_HP_DEFAULT;
gameData.matCens.botGens [m_nMatCen].xInterval = MATCEN_INTERVAL_DEFAULT;
gameData.matCens.botGens [m_nMatCen].nSegment = Index ();
gameData.matCens.botGens [m_nMatCen].nFuelCen = m_value;
return true;
}


//------------------------------------------------------------
// Adds a matcen that already is a special nType into the gameData.matCens.fuelCenters array.
// This function is separate from other fuelcens because we don't want values reset.
bool CSegment::CreateEquipGen (int nOldFunction)
{
m_nMatCen = gameData.matCens.nEquipCenters;
if (!CreateMatCen (nOldFunction, gameFileInfo.equipGen.count))
	return false;
++gameData.matCens.nEquipCenters;
gameData.matCens.equipGens [m_nMatCen].xHitPoints = MATCEN_HP_DEFAULT;
gameData.matCens.equipGens [m_nMatCen].xInterval = MATCEN_INTERVAL_DEFAULT;
gameData.matCens.equipGens [m_nMatCen].nSegment = Index ();
gameData.matCens.equipGens [m_nMatCen].nFuelCen = m_value;
return true;
}

//------------------------------------------------------------

void SetEquipGenStates (void)
{
	int	i;

for (i = 0; i < gameData.matCens.nEquipCenters; i++)
	gameData.matCens.fuelCenters [gameData.matCens.equipGens [i].nFuelCen].bEnabled =
		FindTriggerTarget (gameData.matCens.fuelCenters [i].nSegment, -1) == 0;
}

//------------------------------------------------------------
// Adds a CSegment that already is a special nType into the gameData.matCens.fuelCenters array.
bool CSegment::CreateGenerator (int nType)
{
m_function = nType;
if (m_function == SEGMENT_FUNC_ROBOTMAKER)
	return CreateBotGen (SEGMENT_FUNC_NONE);
else if (m_function == SEGMENT_FUNC_EQUIPMAKER)
	return CreateEquipGen (SEGMENT_FUNC_NONE);
else if ((m_function == SEGMENT_FUNC_FUELCEN) || (m_function == SEGMENT_FUNC_REPAIRCEN)) {
	if (!CreateFuelCen (SEGMENT_FUNC_NONE))
		return false;
	if (m_function == SEGMENT_FUNC_REPAIRCEN)
		m_nMatCen = gameData.matCens.nRepairCenters++;
	return true;
	}
return false;
}

//	The lower this number is, the more quickly the center can be re-triggered.
//	If it's too low, it can mean all the robots won't be put out, but for about 5
//	robots, that's not real likely.
#define	MATCEN_LIFE (I2X (30 - 2 * gameStates.app.nDifficultyLevel))

//------------------------------------------------------------
//	Trigger (enable) the materialization center in CSegment nSegment
int StartMatCen (short nSegment)
{
	// -- CSegment		*segP = &SEGMENTS [nSegment];
	CSegment*		segP = &SEGMENTS [nSegment];
	CFixVector		pos, delta;
	tFuelCenInfo	*matCenP;
	int				nObject;

#if TRACE
console.printf (CON_DBG, "Trigger matcen, CSegment %i\n", nSegment);
#endif
if (segP->m_nMatCen < 0)
	return 0;
if (segP->m_function == SEGMENT_FUNC_EQUIPMAKER) {	// toggle it on or off
	if (segP->m_nMatCen >= gameData.matCens.nEquipCenters)
		return 0;
	matCenP = gameData.matCens.fuelCenters + gameData.matCens.equipGens [segP->m_nMatCen].nFuelCen;
	return (matCenP->bEnabled = !matCenP->bEnabled) ? 1 : 2;
	}
if (segP->m_nMatCen >= gameData.matCens.nBotCenters)
	return 0;
matCenP = gameData.matCens.fuelCenters + gameData.matCens.botGens [segP->m_nMatCen].nFuelCen;
if (matCenP->bEnabled)
	return 0;
//	MK: 11/18/95, At insane, matcens work forever!
if (gameStates.app.bD1Mission || (gameStates.app.nDifficultyLevel + 1 < NDL)) {
	if (!matCenP->nLives)
		return 0;
	--matCenP->nLives;
	}

matCenP->xTimer = I2X (1000);	//	Make sure the first robot gets emitted right away.
matCenP->bEnabled = 1;			//	Say this center is enabled, it can create robots.
matCenP->xCapacity = I2X (gameStates.app.nDifficultyLevel + 3);
matCenP->xDisableTime = MATCEN_LIFE;

//	Create a bright CObject in the CSegment.
pos = matCenP->vCenter;
delta = gameData.segs.vertices[SEGMENTS [nSegment].m_verts [0]] - matCenP->vCenter;
pos += delta * (I2X (1)/2);
nObject = CreateLight (SINGLE_LIGHT_ID, nSegment, pos);
if (nObject != -1) {
	OBJECTS [nObject].SetLife (MATCEN_LIFE);
	OBJECTS [nObject].cType.lightInfo.intensity = I2X (8);	//	Light cast by a fuelcen.
	}
return 0;
}

//	----------------------------------------------------------------------------------------------------------

int GetMatCenObjType (tFuelCenInfo *matCenP, int *objFlags, int maxType)
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
void OperateBotGen (CObject *objP, short nSegment)
{
	CSegment*		segP = &SEGMENTS [nSegment];
	tFuelCenInfo*	matCenP;
	short				nType;

if (nSegment < 0)
	nType = 255;
else {
	matCenP = gameData.matCens.fuelCenters + gameData.matCens.botGens [segP->m_nMatCen].nFuelCen;
	nType = GetMatCenObjType (matCenP, gameData.matCens.botGens [segP->m_nMatCen].objFlags, MAX_ROBOT_TYPES);
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

void CreateMatCenEffect (tFuelCenInfo *matCenP, ubyte nVideoClip)
{
	CFixVector	vPos;
	CObject		*objP;

vPos = SEGMENTS [matCenP->nSegment].Center ();
// HACK!!!The 10 under here should be something equal to the 1/2 the size of the CSegment.
objP = CreateExplosion ((short) matCenP->nSegment, vPos, I2X (10), nVideoClip);
if (objP) {
	ExtractOrientFromSegment (&objP->info.position.mOrient, SEGMENTS + matCenP->nSegment);
	if (gameData.eff.vClips [0][nVideoClip].nSound > -1)
		audio.CreateSegmentSound (gameData.eff.vClips [0][nVideoClip].nSound, (short) matCenP->nSegment, 0, vPos, 0, I2X (1));
	matCenP->bFlag	= 1;
	matCenP->xTimer = 0;
	}
}

//	----------------------------------------------------------------------------------------------------------

void EquipGenHandler (tFuelCenInfo * matCenP)
{
	int			nObject, nMatCen, nType;
	CObject		*objP;
	CFixVector	vPos;
	fix			topTime;

if (!matCenP->bEnabled)
	return;
nMatCen = SEGMENTS [matCenP->nSegment].m_nMatCen;
if (nMatCen == -1) {
#if TRACE
	console.printf (CON_DBG, "Dysfunctional robot generator at %d\n", matCenP->nSegment);
#endif
	return;
	}
matCenP->xTimer += gameData.time.xFrame;
if (!matCenP->bFlag) {
	topTime = EQUIP_GEN_TIME;
	if (matCenP->xTimer < topTime)
		return;
	nObject = SEGMENTS [matCenP->nSegment].m_objects;
	while (nObject >= 0) {
		objP = OBJECTS + nObject;
		if ((objP->info.nType == OBJ_POWERUP) || (objP->info.nId == OBJ_PLAYER)) {
			matCenP->xTimer = 0;
			return;
			}
		nObject = objP->info.nNextInSeg;
		}
	CreateMatCenEffect (matCenP, VCLIP_MORPHING_ROBOT);
	}
else if (matCenP->bFlag == 1) {			// Wait until 1/2 second after VCLIP started.
	if (matCenP->xTimer < (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].xTotalTime / 2))
		return;
	matCenP->bFlag = 0;
	matCenP->xTimer = 0;
	nType = GetMatCenObjType (matCenP, gameData.matCens.equipGens [nMatCen].objFlags, MAX_POWERUP_TYPES);
	if (nType < 0)
		return;
	vPos = SEGMENTS [matCenP->nSegment].Center ();
	// If this is the first materialization, set to valid robot.
	nObject = CreatePowerup (nType, -1, (short) matCenP->nSegment, vPos, 1, true);
	if (nObject < 0)
		return;
	objP = OBJECTS + nObject;
	if (IsMultiGame) {
		gameData.multiplayer.maxPowerupsAllowed [nType]++;
		gameData.multigame.create.nObjNums [gameData.multigame.create.nCount++] = nObject;
		}
	objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->info.nId].nClipIndex;
	objP->rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][objP->rType.vClipInfo.nClipIndex].xFrameTime;
	objP->rType.vClipInfo.nCurFrame = 0;
	objP->info.nCreator = SEGMENTS [matCenP->nSegment].m_owner;
	objP->SetLife (IMMORTAL_TIME);
	}
else {
	matCenP->bFlag = 0;
	matCenP->xTimer = 0;
	}
}

//	----------------------------------------------------------------------------------------------------------

void VirusGenHandler (tFuelCenInfo * matCenP)
{
	int			nObject, nMatCen;
	CObject		*objP;
	CFixVector	vPos;
	fix			topTime;

if (gameStates.entropy.bExitSequence || (SEGMENTS [matCenP->nSegment].m_owner <= 0))
	return;
nMatCen = SEGMENTS [matCenP->nSegment].m_nMatCen;
if (nMatCen == -1) {
#if TRACE
	console.printf (CON_DBG, "Dysfunctional robot generator at %d\n", matCenP->nSegment);
#endif
	return;
	}
matCenP->xTimer += gameData.time.xFrame;
if (!matCenP->bFlag) {
	topTime = I2X (extraGameInfo [1].entropy.nVirusGenTime);
	if (matCenP->xTimer < topTime)
		return;
	nObject = SEGMENTS [matCenP->nSegment].m_objects;
	while (nObject >= 0) {
		objP = OBJECTS + nObject;
		if ((objP->info.nType == OBJ_POWERUP) && (objP->info.nId == POW_ENTROPY_VIRUS)) {
			matCenP->xTimer = 0;
			return;
			}
		nObject = objP->info.nNextInSeg;
		}
	CreateMatCenEffect (matCenP, VCLIP_POWERUP_DISAPPEARANCE);
	}
else if (matCenP->bFlag == 1) {			// Wait until 1/2 second after VCLIP started.
	if (matCenP->xTimer < (gameData.eff.vClips [0][VCLIP_POWERUP_DISAPPEARANCE].xTotalTime / 2))
		return;
	matCenP->bFlag = 0;
	matCenP->xTimer = 0;
	vPos = SEGMENTS [matCenP->nSegment].Center ();
	// If this is the first materialization, set to valid robot.
	nObject = CreatePowerup (POW_ENTROPY_VIRUS, -1, (short) matCenP->nSegment, vPos, 1);
	if (nObject >= 0) {
		objP = OBJECTS + nObject;
		if (IsMultiGame)
			gameData.multigame.create.nObjNums [gameData.multigame.create.nCount++] = nObject;
		objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->info.nId].nClipIndex;
		objP->rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][objP->rType.vClipInfo.nClipIndex].xFrameTime;
		objP->rType.vClipInfo.nCurFrame = 0;
		objP->info.nCreator = SEGMENTS [matCenP->nSegment].m_owner;
		objP->SetLife (IMMORTAL_TIME);
		}
	}
else {
	matCenP->bFlag = 0;
	matCenP->xTimer = 0;
	}
}

//	----------------------------------------------------------------------------------------------------------

inline int VertigoObjFlags (tMatCenInfo *miP)
{
return miP->objFlags [2] = gameData.objs.nVertigoBotFlags;
}

//	----------------------------------------------------------------------------------------------------------

void BotGenHandler (tFuelCenInfo * matCenP)
{
	fix			xDistToPlayer;
	CFixVector	vPos, vDir;
	int			nMatCen, nSegment, nObject;
	CObject		*objP;
	fix			topTime;
	int			nType, nMyStation, nCount;
	//int			i;

if (gameStates.app.bGameSuspended & SUSP_ROBOTS)
	return;
if (!matCenP->bEnabled)
	return;
#if DBG
if (matCenP->nSegment == nDbgSeg)
	nDbgSeg = nDbgSeg;
#endif
if (matCenP->xDisableTime > 0) {
	matCenP->xDisableTime -= gameData.time.xFrame;
	if (matCenP->xDisableTime <= 0) {
#if TRACE
		console.printf (CON_DBG, "Robot center #%i gets disabled due to time running out.\n",FUELCEN_IDX (matCenP));
#endif
		matCenP->bEnabled = 0;
		}
	}
//	No robot making in multiplayer mode.
if (IsMultiGame && (!(gameData.app.nGameMode & GM_MULTI_ROBOTS) || !IAmGameHost ()))
	return;
// Wait until transmorgafier has capacity to make a robot...
if (matCenP->xCapacity <= 0)
	return;
nMatCen = SEGMENTS [matCenP->nSegment].m_nMatCen;
if (nMatCen == -1) {
#if TRACE
	console.printf (CON_DBG, "Dysfunctional robot generator at %d\n", matCenP->nSegment);
#endif
	return;
	}
if (!(gameData.matCens.botGens [nMatCen].objFlags [0] ||
		gameData.matCens.botGens [nMatCen].objFlags [1] ||
		VertigoObjFlags (gameData.matCens.botGens + nMatCen)))
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
matCenP->xTimer += gameData.time.xFrame;
if (!matCenP->bFlag) {
	if (IsMultiGame)
		topTime = ROBOT_GEN_TIME;
	else {
		xDistToPlayer = CFixVector::Dist(gameData.objs.consoleP->info.position.vPos, matCenP->vCenter);
		topTime = xDistToPlayer / 64 + RandShort () * 2 + I2X (2);
		if (topTime > ROBOT_GEN_TIME)
			topTime = ROBOT_GEN_TIME + RandShort ();
		if (topTime < I2X (2))
			topTime = I2X (3)/2 + RandShort ()*2;
		}
	if (matCenP->xTimer < topTime)
		return;
	nMyStation = FUELCEN_IDX (matCenP);

	//	Make sure this robotmaker hasn't put out its max without having any of them killed.
	nCount = 0;
	FORALL_ROBOT_OBJS (objP, i)
		if ((objP->info.nCreator ^ 0x80) == nMyStation)
			nCount++;
	if (nCount > gameStates.app.nDifficultyLevel + 3) {
#if TRACE
		console.printf (CON_DBG, "Cannot morph: center %i has already put out %i robots.\n", nMyStation, nCount);
#endif
		matCenP->xTimer /= 2;
		return;
		}
		//	Whack on any robot or player in the matcen CSegment.
	nCount = 0;
	nSegment = matCenP->nSegment;
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
			OBJECTS [nObject].CollideRobotAndMatCen ();
			matCenP->xTimer = topTime / 2;
			return;
			}
		else if (OBJECTS [nObject].info.nType == OBJ_PLAYER) {
			OBJECTS [nObject].CollidePlayerAndMatCen ();
			matCenP->xTimer = topTime / 2;
			return;
			}
		}
	CreateMatCenEffect (matCenP, VCLIP_MORPHING_ROBOT);
	}
else if (matCenP->bFlag == 1) {			// Wait until 1/2 second after VCLIP started.
	if (matCenP->xTimer <= (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].xTotalTime / 2))
		return;
	matCenP->xCapacity -= gameData.matCens.xEnergyToCreateOneRobot;
	matCenP->bFlag = 0;
	matCenP->xTimer = 0;
	vPos = SEGMENTS [matCenP->nSegment].Center ();
	// If this is the first materialization, set to valid robot.
	nType = (int) GetMatCenObjType (matCenP, gameData.matCens.botGens [nMatCen].objFlags, MAX_ROBOT_TYPES);
	if (nType < 0)
		return;
#if TRACE
	console.printf (CON_DBG, "Morph: (nType = %i) (seg = %i) (capacity = %08x)\n", nType, matCenP->nSegment, matCenP->xCapacity);
#endif
	if (!(objP = CreateMorphRobot (SEGMENTS + matCenP->nSegment, &vPos, nType))) {
#if TRACE
		console.printf (CON_DBG, "Warning: CreateMorphRobot returned NULL (no OBJECTS left?)\n");
#endif
		return;
		}
	if (IsMultiGame)
		MultiSendCreateRobot (FUELCEN_IDX (matCenP), objP->Index (), nType);
	objP->info.nCreator = (FUELCEN_IDX (matCenP)) | 0x80;
	// Make object face player...
	vDir = gameData.objs.consoleP->info.position.vPos - objP->info.position.vPos;
	objP->info.position.mOrient = CFixMatrix::CreateFU(vDir, objP->info.position.mOrient.m.dir.u);
	//objP->info.position.mOrient = CFixMatrix::CreateFU(vDir, &objP->info.position.mOrient.m.v.u, NULL);
	objP->MorphStart ();
	}
else {
	matCenP->bFlag = 0;
	matCenP->xTimer = 0;
	}
}


//-------------------------------------------------------------
// Called once per frame, replenishes fuel supply.
void FuelcenUpdateAll (void)
{
	int				i, t;
	tFuelCenInfo*	fuelCenP = &gameData.matCens.fuelCenters [0];
	fix				xAmountToReplenish = FixMul (gameData.time.xFrame, gameData.matCens.xFuelRefillSpeed);

for (i = 0; i < gameData.matCens.nFuelCenters; i++, fuelCenP++) {
	t = fuelCenP->nType;
	if (t == SEGMENT_FUNC_ROBOTMAKER) {
		if (IsMultiGame && (gameData.app.nGameMode & GM_ENTROPY))
			VirusGenHandler (gameData.matCens.fuelCenters + i);
		else if (!(gameStates.app.bGameSuspended & SUSP_ROBOTS))
			BotGenHandler (gameData.matCens.fuelCenters + i);
		}
	else if (t == SEGMENT_FUNC_EQUIPMAKER) {
		if (!(gameStates.app.bGameSuspended & SUSP_POWERUPS))
			EquipGenHandler (gameData.matCens.fuelCenters + i);
		}
	else if (t == SEGMENT_FUNC_CONTROLCEN) {
		//controlcen_proc (gameData.matCens.fuelCenters + i);
		}
	else if ((fuelCenP->xMaxCapacity > 0) &&
				(gameData.matCens.playerSegP != SEGMENTS + fuelCenP->nSegment)) {
		if (fuelCenP->xCapacity < fuelCenP->xMaxCapacity) {
 			fuelCenP->xCapacity += xAmountToReplenish;
			if (fuelCenP->xCapacity >= fuelCenP->xMaxCapacity) {
				fuelCenP->xCapacity = fuelCenP->xMaxCapacity;
				//gauge_message ("Fuel center is fully recharged!   ");
				}
			}
		}
	}
}

#define FUELCEN_SOUND_DELAY (I2X (1)/4)		//play every half second

//-------------------------------------------------------------

fix CSegment::ShieldDamage (fix xMaxDamage)
{
	static fix lastPlayTime = 0;

if (!(m_xDamage [0] || (gameData.app.nGameMode & GM_ENTROPY)))
	return 0;
int bEntropy = !m_xDamage [0];
if (bEntropy && ((m_owner < 1) || (m_owner == GetTeam (gameData.multiplayer.nLocalPlayer) + 1)))
	return 0;
fix rate = bEntropy ? I2X (extraGameInfo [1].entropy.nShieldDamageRate) : m_xDamage [0];
fix amount = FixMul (gameData.time.xFrame, rate);
if (amount > xMaxDamage)
	amount = xMaxDamage;
if (bEntropy) {
	if (lastPlayTime > gameData.time.xGame)
		lastPlayTime = 0;
	if (gameData.time.xGame > lastPlayTime + FUELCEN_SOUND_DELAY) {
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

gameData.matCens.playerSegP = this;
if ((gameData.app.nGameMode & GM_ENTROPY) && ((m_owner < 0) ||
	 ((m_owner > 0) && (m_owner != GetTeam (gameData.multiplayer.nLocalPlayer) + 1))))
	return 0;
if (m_function != SEGMENT_FUNC_FUELCEN)
	return 0;
DetectEscortGoalAccomplished (-4);	//	UGLY!Hack!-4 means went through fuelcen.
#if 0
if (gameData.matCens.fuelCenters [segP->value].xMaxCapacity <= 0) {
	HUDInitMessage ("Fuelcenter %d is destroyed.", segP->value);
	return 0;
	}
if (gameData.matCens.fuelCenters [segP->value].xCapacity <= 0) {
	HUDInitMessage ("Fuelcenter %d is empty.", segP->value);
	return 0;
	}
#endif
if (nMaxFuel <= 0)
	return 0;
if (gameData.app.nGameMode & GM_ENTROPY)
	amount = FixMul (gameData.time.xFrame, I2X (gameData.matCens.xFuelGiveAmount));
else
	amount = FixMul (gameData.time.xFrame, gameData.matCens.xFuelGiveAmount);
if (amount > nMaxFuel)
	amount = nMaxFuel;
if (lastPlayTime > gameData.time.xGame)
	lastPlayTime = 0;
if (gameData.time.xGame > lastPlayTime + FUELCEN_SOUND_DELAY) {
	audio.PlaySound (SOUND_REFUEL_STATION_GIVING_FUEL, SOUNDCLASS_GENERIC, I2X (1) / 2);
	if (IsMultiGame)
		MultiSendPlaySound (SOUND_REFUEL_STATION_GIVING_FUEL, I2X (1) / 2);
	lastPlayTime = gameData.time.xGame;
	}
//HUDInitMessage ("Fuelcen %d has %d/%d fuel", segP->value,X2I (gameData.matCens.fuelCenters [segP->value].xCapacity),X2I (gameData.matCens.fuelCenters [segP->value].xMaxCapacity));
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

if (gameOpts->legacy.bFuelCens)
	return 0;
gameData.matCens.playerSegP = this;
if ((gameData.app.nGameMode & GM_ENTROPY) && ((m_owner < 0) ||
	 ((m_owner > 0) && (m_owner != GetTeam (gameData.multiplayer.nLocalPlayer) + 1))))
	return 0;
if (m_function != SEGMENT_FUNC_REPAIRCEN)
	return 0;
if (nMaxShield <= 0) {
	return 0;
}
amount = FixMul (gameData.time.xFrame, I2X (extraGameInfo [IsMultiGame].entropy.nShieldFillRate));
if (amount > nMaxShield)
	amount = nMaxShield;
if (lastPlayTime > gameData.time.xGame)
	lastPlayTime = 0;
if (gameData.time.xGame > lastPlayTime + FUELCEN_SOUND_DELAY) {
	audio.PlaySound (SOUND_REFUEL_STATION_GIVING_FUEL, SOUNDCLASS_GENERIC, I2X (1)/2);
	if (IsMultiGame)
		MultiSendPlaySound (SOUND_REFUEL_STATION_GIVING_FUEL, I2X (1)/2);
	lastPlayTime = gameData.time.xGame;
	}
return amount;
}

//	--------------------------------------------------------------------------------------------

void DisableMatCens (void)
{
	int	i;

for (i = 0; i < gameData.matCens.nBotCenters; i++) {
	if (gameData.matCens.fuelCenters [i].nType != SEGMENT_FUNC_EQUIPMAKER) {
		gameData.matCens.fuelCenters [i].bEnabled = 0;
		gameData.matCens.fuelCenters [i].xDisableTime = 0;
		}
	}
}

//	--------------------------------------------------------------------------------------------
//	Initialize all materialization centers.
//	Give them all the right number of lives.
void InitAllMatCens (void)
{
	int	i;

for (i = 0; i < gameData.matCens.nFuelCenters; i++)
	if (gameData.matCens.fuelCenters [i].nType == SEGMENT_FUNC_ROBOTMAKER) {
		 gameData.matCens.fuelCenters [i].nLives = 3;
		 gameData.matCens.fuelCenters [i].bEnabled = 0;
		 gameData.matCens.fuelCenters [i].xDisableTime = 0;
		 }

#if 0 //DBG

	int	j, nFuelCen;
for (i = 0; i < gameData.matCens.nFuelCenters; i++)
	if (gameData.matCens.fuelCenters [i].nType == SEGMENT_FUNC_ROBOTMAKER) {
		//	Make sure this fuelcen is pointed at by a matcen.
		for (j = 0; j < gameData.matCens.nBotCenters; j++)
			if (gameData.matCens.botGens [j].nFuelCen == i)
				break;
		if (j == gameData.matCens.nBotCenters)
			j = j;
		}
	else if (gameData.matCens.fuelCenters [i].nType == SEGMENT_FUNC_EQUIPMAKER) {
		//	Make sure this fuelcen is pointed at by a matcen.
		for (j = 0; j < gameData.matCens.nEquipCenters; j++)
			if (gameData.matCens.equipGens [j].nFuelCen == i)
				break;
		if (j == gameData.matCens.nEquipCenters)
			j = j;
		}

//	Make sure all matcens point at a fuelcen
for (i = 0; i < gameData.matCens.nBotCenters; i++) {
	nFuelCen = gameData.matCens.botGens [i].nFuelCen;
	Assert (nFuelCen < gameData.matCens.nFuelCenters);
	if (gameData.matCens.fuelCenters [nFuelCen].nType != SEGMENT_FUNC_ROBOTMAKER)
		i = i;
	}

for (i = 0; i < gameData.matCens.nEquipCenters; i++) {
	nFuelCen = gameData.matCens.equipGens [i].nFuelCen;
	Assert (nFuelCen < gameData.matCens.nFuelCenters);
	if (gameData.matCens.fuelCenters [nFuelCen].nType != SEGMENT_FUNC_EQUIPMAKER)
		i = i;
	}
#endif

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
if (GetTeam (gameData.multiplayer.nLocalPlayer) != nTeamId)
	return 0;
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_FLAG))
	return 0;
if (gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bEnhancedCTF &&
	 !FlagAtHome ((nFlagId == POW_BLUEFLAG) ? POW_REDFLAG : POW_BLUEFLAG))
	return 0;
MultiSendCaptureBonus (char (gameData.multiplayer.nLocalPlayer));
LOCALPLAYER.flags &= (~(PLAYER_FLAGS_FLAG));
MaybeDropNetPowerup (-1, nFlagId, FORCE_DROP);
return 1;
}

//--------------------------------------------------------------------

void CSegment::CheckForGoal (void)
{
	Assert (gameData.app.nGameMode & GM_CAPTURE);

#if 1
CheckFlagDrop (TEAM_BLUE, POW_REDFLAG, SEGMENT_FUNC_GOAL_BLUE);
CheckFlagDrop (TEAM_RED, POW_BLUEFLAG, SEGMENT_FUNC_GOAL_RED);
#else
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_FLAG))
	return;
if (segP->m_function == SEGMENT_FUNC_GOAL_BLUE) {
	if (GetTeam (gameData.multiplayer.nLocalPlayer) == TEAM_BLUE) && FlagAtHome (POW_BLUEFLAG)) {
		MultiSendCaptureBonus (gameData.multiplayer.nLocalPlayer);
		LOCALPLAYER.flags &= (~(PLAYER_FLAGS_FLAG);
		MaybeDropNetPowerup (-1, POW_REDFLAG, FORCE_DROP);
		}
	}
else if (segP->m_function == SEGMENT_FUNC_GOAL_RED) {
	if (GetTeam (gameData.multiplayer.nLocalPlayer) == TEAM_RED) && FlagAtHome (POW_REDFLAG)) {
		MultiSendCaptureBonus (gameData.multiplayer.nLocalPlayer);
		LOCALPLAYER.flags &= (~(PLAYER_FLAGS_FLAG);
		MaybeDropNetPowerup (-1, POW_BLUEFLAG, FORCE_DROP);
		}
	}
#endif
}

//--------------------------------------------------------------------

void CSegment::CheckForHoardGoal (void)
{
Assert (gameData.app.nGameMode & GM_HOARD);
if (gameStates.app.bPlayerIsDead)
	return;
if ((m_function != SEGMENT_FUNC_GOAL_BLUE) && (m_function != SEGMENT_FUNC_GOAL_RED))
	return;
if (!LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX])
	return;
#if TRACE
console.printf (CON_DBG,"In orb goal!\n");
#endif
MultiSendOrbBonus ((char) gameData.multiplayer.nLocalPlayer);
LOCALPLAYER.flags &= (~(PLAYER_FLAGS_FLAG));
LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX]=0;
}

//--------------------------------------------------------------------
#if 1//ndef FAST_FILE_IO /*permanently enabled for a reason!*/
/*
 * reads an old_tMatCenInfo structure from a CFile
 */
/*
 * reads a tMatCenInfo structure from a CFile
 */
void MatCenInfoRead (tMatCenInfo *mi, CFile& cf, bool bOldFormat)
{
mi->objFlags [0] = cf.ReadInt ();
mi->objFlags [1] = bOldFormat ? 0 : cf.ReadInt ();
mi->xHitPoints = cf.ReadFix ();
mi->xInterval = cf.ReadFix ();
mi->nSegment = cf.ReadShort ();
mi->nFuelCen = cf.ReadShort ();
}
#endif

//--------------------------------------------------------------------
