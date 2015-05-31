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
#include <time.h>

#include "descent.h"
#include "error.h"
#include "savegame.h"

#include "string.h"

#if DBG
#include <time.h>
#endif

#undef DBG
#if DBG
#	define DBG(_expr)	_expr
#else
#	define DBG(_expr)
#endif

//	-------------------------------------------------------------------------------------------------

int32_t CSaveGameManager::LoadAIBinFormat (void)
{
	int32_t	h, i, j;

gameData.aiData.localInfo.Clear ();
gameData.aiData.routeSegs.Clear ();
m_cf.Read (&gameData.aiData.bInitialized, sizeof (int32_t), 1);
m_cf.Read (&gameData.aiData.nOverallAgitation, sizeof (int32_t), 1);
gameData.aiData.localInfo.Read (m_cf, (m_nVersion > 39) ? LEVEL_OBJECTS : (m_nVersion > 22) ? MAX_OBJECTS : MAX_OBJECTS_D2);
h = (m_nVersion > 39) ? LEVEL_POINT_SEGS : (m_nVersion > 22) ? MAX_POINT_SEGS : MAX_POINT_SEGS_D2;
j = (h > int32_t (gameData.aiData.routeSegs.Length ())) ? int32_t (gameData.aiData.routeSegs.Length ()) : h;
for (i = 0; i < j; i++) {
	m_cf.Read (&gameData.aiData.routeSegs [i].nSegment, sizeof (gameData.aiData.routeSegs [i].nSegment), 1);
	m_cf.Read (&gameData.aiData.routeSegs [i].point.v.coord, sizeof (gameData.aiData.routeSegs [i].point.v.coord), 1);
	}
if (j < h)
	m_cf.Seek ((h - j) * 4 * sizeof (int32_t), SEEK_CUR);
gameData.aiData.cloakInfo.Read (m_cf, MAX_AI_CLOAK_INFO_D2);
gameData.bossData.Destroy ();
if (m_nVersion < 29)
	j = 1;
else 
	j = MAX_BOSS_COUNT;
if (!gameData.bossData.Create (j))
	return 0;
gameData.bossData.Grow (j);
gameData.bossData.LoadBinStates (m_cf);

if (m_nVersion >= 8) {
	m_cf.Read (&gameData.escortData.nKillObject, sizeof (gameData.escortData.nKillObject), 1);
	m_cf.Read (&gameData.escortData.xLastPathCreated, sizeof (gameData.escortData.xLastPathCreated), 1);
	m_cf.Read (&gameData.escortData.nGoalObject, sizeof (gameData.escortData.nGoalObject), 1);
	m_cf.Read (&gameData.escortData.nSpecialGoal, sizeof (gameData.escortData.nSpecialGoal), 1);
	m_cf.Read (&gameData.escortData.nGoalIndex, sizeof (gameData.escortData.nGoalIndex), 1);
	gameData.thiefData.stolenItems.Read (m_cf);
	}
else {
	gameData.escortData.nKillObject = -1;
	gameData.escortData.xLastPathCreated = 0;
	gameData.escortData.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
	gameData.escortData.nSpecialGoal = -1;
	gameData.escortData.nGoalIndex = -1;
	gameData.thiefData.stolenItems.Clear (0xff);
	}
if (m_nVersion >= 15) {
	int32_t temp;
	m_cf.Read (&temp, sizeof (int32_t), 1);
	gameData.aiData.freePointSegs = gameData.aiData.routeSegs + temp;
	}
else
	AIResetAllPaths ();
if (m_nVersion >= 21) {
	for (i = 0; i < j; i++) {
		m_cf.Read (&h, sizeof (h), 1);
		gameData.bossData [i].m_nTeleportSegs = int16_t (h);
		}
	for (i = 0; i < j; i++) {
		m_cf.Read (&h, sizeof (h), 1);
		gameData.bossData [i].m_nGateSegs = int16_t (h);
		}
	for (i = 0; i < j; i++) {
		if (gameData.bossData [i].m_nGateSegs && gameData.bossData [i].m_gateSegs.Create (gameData.bossData [i].m_nGateSegs))
			gameData.bossData [i].m_gateSegs.Read (m_cf);
		else
			return 0;
		if (gameData.bossData [i].m_nTeleportSegs && gameData.bossData [i].m_teleportSegs.Create (gameData.bossData [i].m_nTeleportSegs))
			gameData.bossData [i].m_teleportSegs.Read (m_cf);
		else
			return 0;
		}
	}
else {
#if TRACE
	console.printf (1, "Warning: If you fight the boss, he might teleport to CSegment #0!\n");
#endif
	}
return 1;
}
//	-------------------------------------------------------------------------------------------------

void CSaveGameManager::SaveAILocalInfo (tAILocalInfo *pLocalInfo)
{
	int32_t	i;

m_cf.WriteInt (pLocalInfo->targetAwarenessType);
m_cf.WriteInt (pLocalInfo->nRetryCount);
m_cf.WriteInt (pLocalInfo->nConsecutiveRetries);
m_cf.WriteInt (pLocalInfo->mode);
m_cf.WriteInt (pLocalInfo->nPrevVisibility);
m_cf.WriteInt (pLocalInfo->nRapidFireCount);
m_cf.WriteInt (pLocalInfo->nGoalSegment);
m_cf.WriteFix (pLocalInfo->nextActionTime);
m_cf.WriteFix (pLocalInfo->pNextrimaryFire);
m_cf.WriteFix (pLocalInfo->nextSecondaryFire);
m_cf.WriteFix (pLocalInfo->targetAwarenessTime);
m_cf.WriteFix (pLocalInfo->timeTargetSeen);
m_cf.WriteFix (pLocalInfo->timeTargetSoundAttacked);
m_cf.WriteFix (pLocalInfo->nextMiscSoundTime);
m_cf.WriteFix (pLocalInfo->timeSinceProcessed);
for (i = 0; i < MAX_SUBMODELS; i++) {
	m_cf.WriteAngVec (pLocalInfo->goalAngles [i]);
	m_cf.WriteAngVec (pLocalInfo->deltaAngles [i]);
	}
m_cf.Write (pLocalInfo->goalState, sizeof (pLocalInfo->goalState [0]), 1);
m_cf.Write (pLocalInfo->achievedState, sizeof (pLocalInfo->achievedState [0]), 1);
}

//	-------------------------------------------------------------------------------------------------

void CSaveGameManager::SaveAIPointSeg (tPointSeg *pSeg)
{
m_cf.WriteInt (pSeg->nSegment);
m_cf.WriteVector (pSeg->point);
}

//	-------------------------------------------------------------------------------------------------

void CSaveGameManager::SaveAICloakInfo (tAICloakInfo *pCloakInfo)
{
m_cf.WriteFix (pCloakInfo->lastTime);
m_cf.WriteInt (pCloakInfo->nLastSeg);
m_cf.WriteVector (pCloakInfo->vLastPos);
}

//	-------------------------------------------------------------------------------------------------

int32_t CSaveGameManager::SaveAI (void)
{
	int32_t	h, i;

m_cf.WriteInt (gameData.aiData.bInitialized);
m_cf.WriteInt (gameData.aiData.nOverallAgitation);
for (i = 0; i < LEVEL_OBJECTS; i++)
	SaveAILocalInfo (gameData.aiData.localInfo + i);
for (i = 0; i < LEVEL_POINT_SEGS; i++)
	SaveAIPointSeg (gameData.aiData.routeSegs + i);
for (i = 0; i < MAX_AI_CLOAK_INFO; i++)
	SaveAICloakInfo (gameData.aiData.cloakInfo + i);
m_cf.WriteInt (h = int32_t (gameData.bossData.Count ()));
if (h)
	gameData.bossData.SaveStates (m_cf);
m_cf.WriteInt (gameData.escortData.nKillObject);
m_cf.WriteFix (gameData.escortData.xLastPathCreated);
m_cf.WriteInt (gameData.escortData.nGoalObject);
m_cf.WriteInt (gameData.escortData.nSpecialGoal);
m_cf.WriteInt (gameData.escortData.nGoalIndex);
gameData.thiefData.stolenItems.Write (m_cf);
#if DBG
i = CFTell ();
#endif
m_cf.WriteInt (int32_t (gameData.aiData.routeSegs.Index (gameData.aiData.freePointSegs)));
if (h) {
	gameData.bossData.SaveSizeStates (m_cf);
	gameData.bossData.SaveBufferStates (m_cf);
	}
return 1;
}

//	-------------------------------------------------------------------------------------------------

void CSaveGameManager::LoadAILocalInfo (tAILocalInfo *pLocalInfo)
{
	int32_t	i;

pLocalInfo->targetAwarenessType = m_cf.ReadInt ();
pLocalInfo->nRetryCount = m_cf.ReadInt ();
pLocalInfo->nConsecutiveRetries = m_cf.ReadInt ();
pLocalInfo->mode = m_cf.ReadInt ();
pLocalInfo->nPrevVisibility = m_cf.ReadInt ();
pLocalInfo->nRapidFireCount = m_cf.ReadInt ();
pLocalInfo->nGoalSegment = m_cf.ReadInt ();
pLocalInfo->nextActionTime = m_cf.ReadFix ();
pLocalInfo->pNextrimaryFire = m_cf.ReadFix ();
pLocalInfo->nextSecondaryFire = m_cf.ReadFix ();
pLocalInfo->targetAwarenessTime = m_cf.ReadFix ();
pLocalInfo->timeTargetSeen = m_cf.ReadFix ();
pLocalInfo->timeTargetSoundAttacked = m_cf.ReadFix ();
pLocalInfo->nextMiscSoundTime = m_cf.ReadFix ();
pLocalInfo->timeSinceProcessed = m_cf.ReadFix ();
for (i = 0; i < MAX_SUBMODELS; i++) {
	m_cf.ReadAngVec (pLocalInfo->goalAngles [i]);
	m_cf.ReadAngVec (pLocalInfo->deltaAngles [i]);
	}
m_cf.Read (pLocalInfo->goalState, sizeof (pLocalInfo->goalState [0]), 1);
m_cf.Read (pLocalInfo->achievedState, sizeof (pLocalInfo->achievedState [0]), 1);
}

//	-------------------------------------------------------------------------------------------------

void CSaveGameManager::LoadAIPointSeg (tPointSeg *pSeg)
{
pSeg->nSegment = m_cf.ReadInt ();
m_cf.ReadVector (pSeg->point);
}

//	-------------------------------------------------------------------------------------------------

void CSaveGameManager::LoadAICloakInfo (tAICloakInfo *pCloakInfo)
{
pCloakInfo->lastTime = m_cf.ReadFix ();
pCloakInfo->nLastSeg = m_cf.ReadInt ();
m_cf.ReadVector (pCloakInfo->vLastPos);
}

//	-------------------------------------------------------------------------------------------------

int32_t CSaveGameManager::LoadAIUniFormat (void)
{
	int32_t	h, i, j, fPos;

gameData.aiData.bInitialized = m_cf.ReadInt ();
gameData.aiData.nOverallAgitation = m_cf.ReadInt ();

h = (m_nVersion > 39) ? LEVEL_OBJECTS : (m_nVersion > 22) ? MAX_OBJECTS : MAX_OBJECTS_D2;
fPos = (int32_t) m_cf.Tell ();
j = Min (h, int32_t (gameData.aiData.localInfo.Length ()));
gameData.aiData.localInfo.Clear ();
for (i = 0; i < j; i++)
	LoadAILocalInfo (gameData.aiData.localInfo + i);
if (i < h)
	m_cf.Seek ((h - i) * ((int32_t) m_cf.Tell () - fPos) / i, SEEK_CUR);

h = (m_nVersion > 39) ? LEVEL_POINT_SEGS : (m_nVersion > 22) ? MAX_POINT_SEGS : MAX_POINT_SEGS_D2;
fPos = (int32_t) m_cf.Tell ();
gameData.aiData.routeSegs.Clear ();
j = Min (h, int32_t (gameData.aiData.routeSegs.Length ()));
for (i = 0; i < j; i++)
	LoadAIPointSeg (gameData.aiData.routeSegs + i);
if (i < h)
	m_cf.Seek ((h - i) * ((int32_t) m_cf.Tell () - fPos) / i, SEEK_CUR);

j = (m_nVersion < 42) ? 8 : MAX_AI_CLOAK_INFO;
gameData.aiData.cloakInfo.Clear ();
for (i = 0; i < j; i++)
	LoadAICloakInfo (gameData.aiData.cloakInfo + i);

gameData.modelData.Destroy ();
gameData.modelData.Prepare ();

h = (m_nVersion < 24) ? 1 : (m_nVersion < 42) ? MAX_BOSS_COUNT : m_cf.ReadFix ();
gameData.bossData.Destroy ();
if ((h < 0) > (h > 1000))
	return 0;
if (h) {
	if (!gameData.bossData.Create (h))
		return 0;
	gameData.bossData.Grow (h);
	gameData.bossData.LoadStates (m_cf, m_nVersion);
	}
if (m_nVersion >= 8) {
	gameData.escortData.nKillObject = m_cf.ReadInt ();
	gameData.escortData.xLastPathCreated = m_cf.ReadFix ();
	gameData.escortData.nGoalObject = m_cf.ReadInt ();
	gameData.escortData.nSpecialGoal = m_cf.ReadInt ();
	gameData.escortData.nGoalIndex = m_cf.ReadInt ();
	gameData.thiefData.stolenItems.Read (m_cf);
	}
else {
	gameData.escortData.nKillObject = -1;
	gameData.escortData.xLastPathCreated = 0;
	gameData.escortData.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
	gameData.escortData.nSpecialGoal = -1;
	gameData.escortData.nGoalIndex = -1;
	gameData.thiefData.stolenItems.Clear (char (0xff));
	}

if (m_nVersion < 15) 
	AIResetAllPaths ();
else {
	DBG (i = CFTell (fp));
	i = m_cf.ReadInt ();
	if ((i >= 0) && (i < int32_t (gameData.aiData.routeSegs.Length ())))
		gameData.aiData.freePointSegs = gameData.aiData.routeSegs + i;
	else
		AIResetAllPaths ();
	}

if (m_nVersion < 21) {
	#if TRACE
	console.printf (1, "Warning: If you fight the boss, he might teleport to CSegment #0!\n");
	#endif
	}
else if (h) {
	gameData.bossData.LoadSizeStates (m_cf);
	if (!gameData.bossData.LoadBufferStates (m_cf))
		return 0;
	}
return 1;
}

//	-------------------------------------------------------------------------------------------------
