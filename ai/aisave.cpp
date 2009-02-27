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
#include "state.h"

#include "string.h"
//#define _DEBUG
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

int CSaveGameManager::LoadAIBinFormat (void)
{
	int	i, j;

gameData.ai.localInfo.Clear ();
gameData.ai.routeSegs.Clear ();
m_cf.Read (&gameData.ai.bInitialized, sizeof (int), 1);
m_cf.Read (&gameData.ai.nOverallAgitation, sizeof (int), 1);
gameData.ai.localInfo.Read (m_cf, (m_nVersion > 39) ? LEVEL_OBJECTS : (m_nVersion > 22) ? MAX_OBJECTS : MAX_OBJECTS_D2);
gameData.ai.routeSegs.Read (m_cf, (m_nVersion > 39) ? LEVEL_POINT_SEGS : (m_nVersion > 22) ? MAX_POINT_SEGS : MAX_POINT_SEGS_D2);
gameData.ai.cloakInfo.Read (m_cf);
gameData.bosses.Destroy ();
	return 0;
if (m_nVersion < 29)
	j = 1;
else 
	j = MAX_BOSS_COUNT;
if (!gameData.bosses.Create (j))
	return 0;
gameData.bosses.Grow (j);
gameData.bosses.LoadBinStates (m_cf);

if (m_nVersion >= 8) {
	m_cf.Read (&gameData.escort.nKillObject, sizeof (gameData.escort.nKillObject), 1);
	m_cf.Read (&gameData.escort.xLastPathCreated, sizeof (gameData.escort.xLastPathCreated), 1);
	m_cf.Read (&gameData.escort.nGoalObject, sizeof (gameData.escort.nGoalObject), 1);
	m_cf.Read (&gameData.escort.nSpecialGoal, sizeof (gameData.escort.nSpecialGoal), 1);
	m_cf.Read (&gameData.escort.nGoalIndex, sizeof (gameData.escort.nGoalIndex), 1);
	gameData.thief.stolenItems.Read (m_cf);
	}
else {
	gameData.escort.nKillObject = -1;
	gameData.escort.xLastPathCreated = 0;
	gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
	gameData.escort.nSpecialGoal = -1;
	gameData.escort.nGoalIndex = -1;
	gameData.thief.stolenItems.Clear (0xff);
	}
if (m_nVersion >= 15) {
	int temp;
	m_cf.Read (&temp, sizeof (int), 1);
	gameData.ai.freePointSegs = gameData.ai.routeSegs + temp;
	}
else
	AIResetAllPaths ();
if (m_nVersion >= 21) {
	for (i = 0; i < j; i++)
		m_cf.Read (&gameData.bosses [i].m_nTeleportSegs, sizeof (gameData.bosses [i].m_nTeleportSegs), 1);
	for (i = 0; i < j; i++)
		m_cf.Read (&gameData.bosses [i].m_nGateSegs, sizeof (gameData.bosses [i].m_nGateSegs), 1);
	for (i = 0; i < j; i++) {
		if (gameData.bosses [i].m_nGateSegs && gameData.bosses [i].m_gateSegs.Create (gameData.bosses [i].m_nGateSegs))
			gameData.bosses [i].m_gateSegs.Read (m_cf);
		else
			return 0;
		if (gameData.bosses [i].m_nTeleportSegs && gameData.bosses [i].m_teleportSegs.Create (gameData.bosses [i].m_nTeleportSegs))
			gameData.bosses [i].m_teleportSegs.Read (m_cf);
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

void CSaveGameManager::SaveAILocalInfo (tAILocalInfo *ailP)
{
	int	i;

m_cf.WriteInt (ailP->playerAwarenessType);
m_cf.WriteInt (ailP->nRetryCount);
m_cf.WriteInt (ailP->nConsecutiveRetries);
m_cf.WriteInt (ailP->mode);
m_cf.WriteInt (ailP->nPrevVisibility);
m_cf.WriteInt (ailP->nRapidFireCount);
m_cf.WriteInt (ailP->nGoalSegment);
m_cf.WriteFix (ailP->nextActionTime);
m_cf.WriteFix (ailP->nextPrimaryFire);
m_cf.WriteFix (ailP->nextSecondaryFire);
m_cf.WriteFix (ailP->playerAwarenessTime);
m_cf.WriteFix (ailP->timePlayerSeen);
m_cf.WriteFix (ailP->timePlayerSoundAttacked);
m_cf.WriteFix (ailP->nextMiscSoundTime);
m_cf.WriteFix (ailP->timeSinceProcessed);
for (i = 0; i < MAX_SUBMODELS; i++) {
	m_cf.WriteAngVec (ailP->goalAngles [i]);
	m_cf.WriteAngVec (ailP->deltaAngles [i]);
	}
m_cf.Write (ailP->goalState, sizeof (ailP->goalState [0]), 1);
m_cf.Write (ailP->achievedState, sizeof (ailP->achievedState [0]), 1);
}

//	-------------------------------------------------------------------------------------------------

void CSaveGameManager::SaveAIPointSeg (tPointSeg *psegP)
{
m_cf.WriteInt (psegP->nSegment);
m_cf.WriteVector (psegP->point);
}

//	-------------------------------------------------------------------------------------------------

void CSaveGameManager::SaveAICloakInfo (tAICloakInfo *ciP)
{
m_cf.WriteFix (ciP->lastTime);
m_cf.WriteInt (ciP->nLastSeg);
m_cf.WriteVector (ciP->vLastPos);
}

//	-------------------------------------------------------------------------------------------------

int CSaveGameManager::SaveAI (void)
{
	int	h, i;

m_cf.WriteInt (gameData.ai.bInitialized);
m_cf.WriteInt (gameData.ai.nOverallAgitation);
for (i = 0; i < LEVEL_OBJECTS; i++)
	SaveAILocalInfo (gameData.ai.localInfo + i);
for (i = 0; i < LEVEL_POINT_SEGS; i++)
	SaveAIPointSeg (gameData.ai.routeSegs + i);
for (i = 0; i < MAX_AI_CLOAK_INFO; i++)
	SaveAICloakInfo (gameData.ai.cloakInfo + i);
m_cf.WriteInt (h = int (gameData.bosses.Count ()));
if (h)
	gameData.bosses.SaveStates (m_cf);
m_cf.WriteInt (gameData.escort.nKillObject);
m_cf.WriteFix (gameData.escort.xLastPathCreated);
m_cf.WriteInt (gameData.escort.nGoalObject);
m_cf.WriteInt (gameData.escort.nSpecialGoal);
m_cf.WriteInt (gameData.escort.nGoalIndex);
gameData.thief.stolenItems.Write (m_cf);
#if DBG
i = CFTell ();
#endif
m_cf.WriteInt (int (gameData.ai.routeSegs.Index (gameData.ai.freePointSegs)));
if (h) {
	gameData.bosses.SaveSizeStates (m_cf);
	gameData.bosses.SaveBufferStates (m_cf);
	}
return 1;
}

//	-------------------------------------------------------------------------------------------------

void CSaveGameManager::LoadAILocalInfo (tAILocalInfo *ailP)
{
	int	i;

ailP->playerAwarenessType = m_cf.ReadInt ();
ailP->nRetryCount = m_cf.ReadInt ();
ailP->nConsecutiveRetries = m_cf.ReadInt ();
ailP->mode = m_cf.ReadInt ();
ailP->nPrevVisibility = m_cf.ReadInt ();
ailP->nRapidFireCount = m_cf.ReadInt ();
ailP->nGoalSegment = m_cf.ReadInt ();
ailP->nextActionTime = m_cf.ReadFix ();
ailP->nextPrimaryFire = m_cf.ReadFix ();
ailP->nextSecondaryFire = m_cf.ReadFix ();
ailP->playerAwarenessTime = m_cf.ReadFix ();
ailP->timePlayerSeen = m_cf.ReadFix ();
ailP->timePlayerSoundAttacked = m_cf.ReadFix ();
ailP->nextMiscSoundTime = m_cf.ReadFix ();
ailP->timeSinceProcessed = m_cf.ReadFix ();
for (i = 0; i < MAX_SUBMODELS; i++) {
	m_cf.ReadAngVec (ailP->goalAngles [i]);
	m_cf.ReadAngVec (ailP->deltaAngles [i]);
	}
m_cf.Read (ailP->goalState, sizeof (ailP->goalState [0]), 1);
m_cf.Read (ailP->achievedState, sizeof (ailP->achievedState [0]), 1);
}

//	-------------------------------------------------------------------------------------------------

void CSaveGameManager::LoadAIPointSeg (tPointSeg *psegP)
{
psegP->nSegment = m_cf.ReadInt ();
m_cf.ReadVector (psegP->point);
}

//	-------------------------------------------------------------------------------------------------

void CSaveGameManager::LoadAICloakInfo (tAICloakInfo *ciP)
{
ciP->lastTime = m_cf.ReadFix ();
ciP->nLastSeg = m_cf.ReadInt ();
m_cf.ReadVector (ciP->vLastPos);
}

//	-------------------------------------------------------------------------------------------------

int CSaveGameManager::LoadAIUniFormat (void)
{
	int	h, i, j, fPos, nRouteSegs;

gameData.ai.bInitialized = m_cf.ReadInt ();
gameData.ai.nOverallAgitation = m_cf.ReadInt ();

h = (m_nVersion > 39) ? LEVEL_OBJECTS : (m_nVersion > 22) ? MAX_OBJECTS : MAX_OBJECTS_D2;
fPos = m_cf.Tell ();
j = min (h, int (gameData.ai.localInfo.Length ()));
gameData.ai.localInfo.Clear ();
for (i = 0; i < j; i++)
	LoadAILocalInfo (gameData.ai.localInfo + i);
if (i < h)
	m_cf.Seek ((h - i) * (m_cf.Tell () - fPos) / i, SEEK_CUR);

h = (m_nVersion > 39) ? LEVEL_POINT_SEGS : (m_nVersion > 22) ? MAX_POINT_SEGS : MAX_POINT_SEGS_D2;
nRouteSegs = h;
fPos = m_cf.Tell ();
gameData.ai.routeSegs.Clear ();
j = min (h, int (gameData.ai.routeSegs.Length ()));
for (i = 0; i < j; i++)
	LoadAIPointSeg (gameData.ai.routeSegs + i);
if (i < h)
	m_cf.Seek ((h - i) * (m_cf.Tell () - fPos) / i, SEEK_CUR);

j = (m_nVersion < 42) ? 8 : MAX_AI_CLOAK_INFO;
gameData.ai.cloakInfo.Clear ();
for (i = 0; i < j; i++)
	LoadAICloakInfo (gameData.ai.cloakInfo + i);

gameData.models.Destroy ();
gameData.models.Prepare ();

h = (m_nVersion < 24) ? 1 : (m_nVersion < 42) ? MAX_BOSS_COUNT : m_cf.ReadFix ();
gameData.bosses.Destroy ();
if (h) {
	if (!gameData.bosses.Create (h))
		return 0;
	gameData.bosses.Grow (h);
	gameData.bosses.LoadStates (m_cf, m_nVersion);
	}
if (m_nVersion >= 8) {
	gameData.escort.nKillObject = m_cf.ReadInt ();
	gameData.escort.xLastPathCreated = m_cf.ReadFix ();
	gameData.escort.nGoalObject = m_cf.ReadInt ();
	gameData.escort.nSpecialGoal = m_cf.ReadInt ();
	gameData.escort.nGoalIndex = m_cf.ReadInt ();
	gameData.thief.stolenItems.Read (m_cf);
	}
else {
	gameData.escort.nKillObject = -1;
	gameData.escort.xLastPathCreated = 0;
	gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
	gameData.escort.nSpecialGoal = -1;
	gameData.escort.nGoalIndex = -1;
	gameData.thief.stolenItems.Clear (char (0xff));
	}

if (m_nVersion < 15) 
	AIResetAllPaths ();
else {
	DBG (i = CFTell (fp));
	i = m_cf.ReadInt ();
	if ((i >= 0) && (i < nRouteSegs))
		gameData.ai.freePointSegs = gameData.ai.routeSegs + i;
	else
		AIResetAllPaths ();
	}

if (m_nVersion < 21) {
	#if TRACE
	console.printf (1, "Warning: If you fight the boss, he might teleport to CSegment #0!\n");
	#endif
	}
else if (h && (m_nVersion < 43)) {
	gameData.bosses.LoadSizeStates (m_cf);
	if (!gameData.bosses.LoadBufferStates (m_cf))
		return 0;
	}
return 1;
}

//	-------------------------------------------------------------------------------------------------
