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

#include "inferno.h"
#include "error.h"
#include "state.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

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

int CSaveGameHandler::LoadAIBinFormat (void)
{
	int	i;

gameData.ai.localInfo.Clear ();
gameData.ai.pointSegs.Clear ();
m_cf.Read (&gameData.ai.bInitialized, sizeof (int), 1);
m_cf.Read (&gameData.ai.nOverallAgitation, sizeof (int), 1);
m_cf.Read (gameData.ai.localInfo.Buffer (), sizeof (tAILocalInfo), (m_nVersion > 22) ? MAX_OBJECTS : MAX_OBJECTS_D2);
m_cf.Read (gameData.ai.pointSegs.Buffer (), sizeof (tPointSeg), (m_nVersion > 22) ? MAX_POINT_SEGS : MAX_POINT_SEGS_D2);
m_cf.Read (gameData.ai.cloakInfo.Buffer (), sizeof (tAICloakInfo), MAX_AI_CLOAK_INFO);
if (m_nVersion < 29) {
	m_cf.Read (&gameData.boss [0].nCloakStartTime, sizeof (fix), 1);
	m_cf.Read (&gameData.boss [0].nCloakEndTime , sizeof (fix), 1);
	m_cf.Read (&gameData.boss [0].nLastTeleportTime , sizeof (fix), 1);
	m_cf.Read (&gameData.boss [0].nTeleportInterval, sizeof (fix), 1);
	m_cf.Read (&gameData.boss [0].nCloakInterval, sizeof (fix), 1);
	m_cf.Read (&gameData.boss [0].nCloakDuration, sizeof (fix), 1);
	m_cf.Read (&gameData.boss [0].nLastGateTime, sizeof (fix), 1);
	m_cf.Read (&gameData.boss [0].nGateInterval, sizeof (fix), 1);
	m_cf.Read (&gameData.boss [0].nDyingStartTime, sizeof (fix), 1);
	m_cf.Read (&gameData.boss [0].nDying, sizeof (int), 1);
	m_cf.Read (&gameData.boss [0].bDyingSoundPlaying, sizeof (int), 1);
	m_cf.Read (&gameData.boss [0].nHitTime, sizeof (fix), 1);
	for (i = 1; i < MAX_BOSS_COUNT; i++) {
		gameData.boss [i].nCloakStartTime = 0;
		gameData.boss [i].nCloakEndTime = 0;
		gameData.boss [i].nLastTeleportTime = 0;
		gameData.boss [i].nTeleportInterval = 0;
		gameData.boss [i].nCloakInterval = 0;
		gameData.boss [i].nCloakDuration = 0;
		gameData.boss [i].nLastGateTime = 0;
		gameData.boss [i].nGateInterval = 0;
		gameData.boss [i].nDyingStartTime = 0;
		gameData.boss [i].nDying = 0;
		gameData.boss [i].bDyingSoundPlaying = 0;
		gameData.boss [i].nHitTime = 0;
		}
	}
else {
	for (i = 0; i < MAX_BOSS_COUNT; i++) {
		m_cf.Read (&gameData.boss [i].nCloakStartTime, sizeof (fix), 1);
		m_cf.Read (&gameData.boss [i].nCloakEndTime , sizeof (fix), 1);
		m_cf.Read (&gameData.boss [i].nLastTeleportTime , sizeof (fix), 1);
		m_cf.Read (&gameData.boss [i].nTeleportInterval, sizeof (fix), 1);
		m_cf.Read (&gameData.boss [i].nCloakInterval, sizeof (fix), 1);
		m_cf.Read (&gameData.boss [i].nCloakDuration, sizeof (fix), 1);
		m_cf.Read (&gameData.boss [i].nLastGateTime, sizeof (fix), 1);
		m_cf.Read (&gameData.boss [i].nGateInterval, sizeof (fix), 1);
		m_cf.Read (&gameData.boss [i].nDyingStartTime, sizeof (fix), 1);
		m_cf.Read (&gameData.boss [i].nDying, sizeof (int), 1);
		m_cf.Read (&gameData.boss [i].bDyingSoundPlaying, sizeof (int), 1);
		m_cf.Read (&gameData.boss [i].nHitTime, sizeof (fix), 1);
		}
	}
// -- MK, 10/21/95, unused!-- m_cf.Read (&Boss_been_hit, sizeof (int), 1);

if (m_nVersion >= 8) {
	m_cf.Read (&gameData.escort.nKillObject, sizeof (gameData.escort.nKillObject), 1);
	m_cf.Read (&gameData.escort.xLastPathCreated, sizeof (gameData.escort.xLastPathCreated), 1);
	m_cf.Read (&gameData.escort.nGoalObject, sizeof (gameData.escort.nGoalObject), 1);
	m_cf.Read (&gameData.escort.nSpecialGoal, sizeof (gameData.escort.nSpecialGoal), 1);
	m_cf.Read (&gameData.escort.nGoalIndex, sizeof (gameData.escort.nGoalIndex), 1);
	m_cf.Read (&gameData.thief.stolenItems, sizeof (gameData.thief.stolenItems [0]), MAX_STOLEN_ITEMS);
	}
else {
	int i;

	gameData.escort.nKillObject = -1;
	gameData.escort.xLastPathCreated = 0;
	gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
	gameData.escort.nSpecialGoal = -1;
	gameData.escort.nGoalIndex = -1;
	for (i=0; i<MAX_STOLEN_ITEMS; i++) {
		gameData.thief.stolenItems [i] = 255;
		}
	}
if (m_nVersion >= 15) {
	int temp;
	m_cf.Read (&temp, sizeof (int), 1);
	gameData.ai.freePointSegs = gameData.ai.pointSegs + temp;
	}
else
	AIResetAllPaths ();
if (m_nVersion >= 24) {
	for (i = 0; i < MAX_BOSS_COUNT; i++)
		m_cf.Read (&gameData.boss [i].nTeleportSegs, sizeof (gameData.boss [i].nTeleportSegs), 1);
	for (i = 0; i < MAX_BOSS_COUNT; i++)
		m_cf.Read (&gameData.boss [i].nGateSegs, sizeof (gameData.boss [i].nGateSegs), 1);
	for (i = 0; i < MAX_BOSS_COUNT; i++) {
		if (gameData.boss [i].nGateSegs)
			m_cf.Read (gameData.boss [i].gateSegs, sizeof (gameData.boss [i].gateSegs [0]), gameData.boss [i].nGateSegs);
		if (gameData.boss [i].nTeleportSegs)
			m_cf.Read (gameData.boss [i].teleportSegs, sizeof (gameData.boss [i].teleportSegs [0]), gameData.boss [i].nTeleportSegs);
		}
	}
else if (m_nVersion >= 21) {
	short nTeleportSegs, nGateSegs;

	m_cf.Read (&nTeleportSegs, sizeof (nTeleportSegs), 1);
	m_cf.Read (&nGateSegs, sizeof (nGateSegs), 1);

	if (nGateSegs) {
		gameData.boss [0].nGateSegs = nGateSegs;
		m_cf.Read (gameData.boss [0].gateSegs, sizeof (gameData.boss [0].gateSegs [0]), nGateSegs);
		}
	if (nTeleportSegs) {
		gameData.boss [0].nTeleportSegs = nTeleportSegs;
		m_cf.Read (gameData.boss [0].teleportSegs, sizeof (gameData.boss [0].teleportSegs [0]), nTeleportSegs);
		}
} else {
	// -- gameData.boss.nTeleportSegs = 1;
	// -- gameData.boss.nGateSegs = 1;
	// -- gameData.boss.teleportSegs [0] = 0;
	// -- gameData.boss.gateSegs [0] = 0;
	// Note: Maybe better to leave alone...will probably be ok.
#if TRACE
	con_printf (1, "Warning: If you fight the boss, he might teleport to CSegment #0!\n");
#endif
	}
return 1;
}
//	-------------------------------------------------------------------------------------------------

void CSaveGameHandler::SaveAILocalInfo (tAILocalInfo *ailP)
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
	m_cf.WriteAngVec (ailP->goalAngles[i]);
	m_cf.WriteAngVec (ailP->deltaAngles[i]);
	}
m_cf.Write (ailP->goalState, sizeof (ailP->goalState [0]), 1);
m_cf.Write (ailP->achievedState, sizeof (ailP->achievedState [0]), 1);
}

//	-------------------------------------------------------------------------------------------------

void CSaveGameHandler::SaveAIPointSeg (tPointSeg *psegP)
{
m_cf.WriteInt (psegP->nSegment);
m_cf.WriteVector (psegP->point);
}

//	-------------------------------------------------------------------------------------------------

void CSaveGameHandler::SaveAICloakInfo (tAICloakInfo *ciP)
{
m_cf.WriteFix (ciP->lastTime);
m_cf.WriteInt (ciP->nLastSeg);
m_cf.WriteVector (ciP->vLastPos);
}

//	-------------------------------------------------------------------------------------------------

int CSaveGameHandler::SaveAI (void)
{
	int	h, i, j;

m_cf.WriteInt (gameData.ai.bInitialized);
m_cf.WriteInt (gameData.ai.nOverallAgitation);
for (i = 0; i < MAX_OBJECTS; i++)
	SaveAILocalInfo (gameData.ai.localInfo + i);
for (i = 0; i < MAX_POINT_SEGS; i++)
	SaveAIPointSeg (gameData.ai.pointSegs + i);
for (i = 0; i < MAX_AI_CLOAK_INFO; i++)
	SaveAICloakInfo (gameData.ai.cloakInfo + i);
for (i = 0; i < MAX_BOSS_COUNT; i++) {
	m_cf.WriteShort (gameData.boss [i].nObject);
	m_cf.WriteFix (gameData.boss [i].nCloakStartTime);
	m_cf.WriteFix (gameData.boss [i].nCloakEndTime );
	m_cf.WriteFix (gameData.boss [i].nLastTeleportTime );
	m_cf.WriteFix (gameData.boss [i].nTeleportInterval);
	m_cf.WriteFix (gameData.boss [i].nCloakInterval);
	m_cf.WriteFix (gameData.boss [i].nCloakDuration);
	m_cf.WriteFix (gameData.boss [i].nLastGateTime);
	m_cf.WriteFix (gameData.boss [i].nGateInterval);
	m_cf.WriteFix (gameData.boss [i].nDyingStartTime);
	m_cf.WriteInt (gameData.boss [i].nDying);
	m_cf.WriteInt (gameData.boss [i].bDyingSoundPlaying);
	m_cf.WriteFix (gameData.boss [i].nHitTime);
	}
m_cf.WriteInt (gameData.escort.nKillObject);
m_cf.WriteFix (gameData.escort.xLastPathCreated);
m_cf.WriteInt (gameData.escort.nGoalObject);
m_cf.WriteInt (gameData.escort.nSpecialGoal);
m_cf.WriteInt (gameData.escort.nGoalIndex);
m_cf.Write (gameData.thief.stolenItems, sizeof (gameData.thief.stolenItems [0]), MAX_STOLEN_ITEMS);
#if DBG
i = CFTell ();
#endif
m_cf.WriteInt ((int) (gameData.ai.freePointSegs - gameData.ai.pointSegs));
for (i = 0; i < MAX_BOSS_COUNT; i++) {
	m_cf.WriteShort (gameData.boss [i].nTeleportSegs);
	m_cf.WriteShort (gameData.boss [i].nGateSegs);
	}
for (i = 0; i < MAX_BOSS_COUNT; i++) {
	if ((h = gameData.boss [i].nGateSegs))
		for (j = 0; j < h; j++)
			m_cf.WriteShort (gameData.boss [i].gateSegs [j]);
	if ((h = gameData.boss [i].nTeleportSegs))
		for (j = 0; j < h; j++)
			m_cf.WriteShort (gameData.boss [i].teleportSegs [j]);
	}
return 1;
}

//	-------------------------------------------------------------------------------------------------

void CSaveGameHandler::LoadAILocalInfo (tAILocalInfo *ailP)
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

void CSaveGameHandler::LoadAIPointSeg (tPointSeg *psegP)
{
psegP->nSegment = m_cf.ReadInt ();
m_cf.ReadVector (psegP->point);
}

//	-------------------------------------------------------------------------------------------------

void CSaveGameHandler::LoadAICloakInfo (tAICloakInfo *ciP)
{
ciP->lastTime = m_cf.ReadFix ();
ciP->nLastSeg = m_cf.ReadInt ();
m_cf.ReadVector (ciP->vLastPos);
}

//	-------------------------------------------------------------------------------------------------

int CSaveGameHandler::LoadAIUniFormat (void)
{
	int	h, i, j, nMaxBossCount, nMaxPointSegs;

gameData.ai.localInfo.Clear ();
gameData.ai.bInitialized = m_cf.ReadInt ();
gameData.ai.nOverallAgitation = m_cf.ReadInt ();
h = (m_nVersion > 22) ? MAX_OBJECTS : MAX_OBJECTS_D2;
DBG (i = CFTell (fp));
for (i = 0; i < h; i++)
	LoadAILocalInfo (gameData.ai.localInfo + i);
nMaxPointSegs = (m_nVersion > 22) ? MAX_POINT_SEGS : MAX_POINT_SEGS_D2;
gameData.ai.pointSegs.Clear ();
DBG (i = CFTell (fp));
for (i = 0; i < nMaxPointSegs; i++)
	LoadAIPointSeg (gameData.ai.pointSegs + i);
DBG (i = CFTell (fp));
for (i = 0; i < MAX_AI_CLOAK_INFO; i++)
	LoadAICloakInfo (gameData.ai.cloakInfo + i);
DBG (i = CFTell (fp));
if (m_nVersion < 29) {
	gameData.boss [0].nCloakStartTime = m_cf.ReadFix ();
	gameData.boss [0].nCloakEndTime = m_cf.ReadFix ();
	gameData.boss [0].nLastTeleportTime = m_cf.ReadFix ();
	gameData.boss [0].nTeleportInterval = m_cf.ReadFix ();
	gameData.boss [0].nCloakInterval = m_cf.ReadFix ();
	gameData.boss [0].nCloakDuration = m_cf.ReadFix ();
	gameData.boss [0].nLastGateTime = m_cf.ReadFix ();
	gameData.boss [0].nGateInterval = m_cf.ReadFix ();
	gameData.boss [0].nDyingStartTime = m_cf.ReadFix ();
	gameData.boss [0].nDying = m_cf.ReadInt ();
	gameData.boss [0].bDyingSoundPlaying = m_cf.ReadInt ();
	gameData.boss [0].nHitTime = m_cf.ReadFix ();
	for (i = 1; i < MAX_BOSS_COUNT; i++) {
		gameData.boss [i].nCloakStartTime = 0;
		gameData.boss [i].nCloakEndTime = 0;
		gameData.boss [i].nLastTeleportTime = 0;
		gameData.boss [i].nTeleportInterval = 0;
		gameData.boss [i].nCloakInterval = 0;
		gameData.boss [i].nCloakDuration = 0;
		gameData.boss [i].nLastGateTime = 0;
		gameData.boss [i].nGateInterval = 0;
		gameData.boss [i].nDyingStartTime = 0;
		gameData.boss [i].nDying = 0;
		gameData.boss [i].bDyingSoundPlaying = 0;
		gameData.boss [i].nHitTime = 0;
		}
	}
else {
	for (i = 0; i < MAX_BOSS_COUNT; i++) {
		if (m_nVersion > 31)
			gameData.boss [i].nObject = m_cf.ReadShort ();
		gameData.boss [i].nCloakStartTime = m_cf.ReadFix ();
		gameData.boss [i].nCloakEndTime = m_cf.ReadFix ();
		gameData.boss [i].nLastTeleportTime = m_cf.ReadFix ();
		gameData.boss [i].nTeleportInterval = m_cf.ReadFix ();
		gameData.boss [i].nCloakInterval = m_cf.ReadFix ();
		gameData.boss [i].nCloakDuration = m_cf.ReadFix ();
		gameData.boss [i].nLastGateTime = m_cf.ReadFix ();
		gameData.boss [i].nGateInterval = m_cf.ReadFix ();
		gameData.boss [i].nDyingStartTime = m_cf.ReadFix ();
		gameData.boss [i].nDying = m_cf.ReadInt ();
		gameData.boss [i].bDyingSoundPlaying = m_cf.ReadInt ();
		gameData.boss [i].nHitTime = m_cf.ReadFix ();
		}
	}
if (m_nVersion >= 8) {
	gameData.escort.nKillObject = m_cf.ReadInt ();
	gameData.escort.xLastPathCreated = m_cf.ReadFix ();
	gameData.escort.nGoalObject = m_cf.ReadInt ();
	gameData.escort.nSpecialGoal = m_cf.ReadInt ();
	gameData.escort.nGoalIndex = m_cf.ReadInt ();
	m_cf.Read (&gameData.thief.stolenItems, sizeof (gameData.thief.stolenItems [0]), MAX_STOLEN_ITEMS);
	}
else {
	gameData.escort.nKillObject = -1;
	gameData.escort.xLastPathCreated = 0;
	gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
	gameData.escort.nSpecialGoal = -1;
	gameData.escort.nGoalIndex = -1;
	memset (gameData.thief.stolenItems, 255, sizeof (gameData.thief.stolenItems));
	}

if (m_nVersion >= 15) {
	DBG (i = CFTell (fp));
	i = m_cf.ReadInt ();
	if ((i >= 0) && (i < nMaxPointSegs))
		gameData.ai.freePointSegs = gameData.ai.pointSegs + i;
	else
		AIResetAllPaths ();
	}
else
	AIResetAllPaths ();

if (m_nVersion < 21) {
	#if TRACE
	con_printf (1, "Warning: If you fight the boss, he might teleport to CSegment #0!\n");
	#endif
	}
else {
	nMaxBossCount = (m_nVersion >= 24) ? MAX_BOSS_COUNT : 1;
	for (i = 0; i < MAX_BOSS_COUNT; i++) {
		gameData.boss [i].nTeleportSegs = m_cf.ReadShort ();
		gameData.boss [i].nGateSegs = m_cf.ReadShort ();
		}
	for (i = 0; i < MAX_BOSS_COUNT; i++) {
		if ((h = gameData.boss [i].nGateSegs))
			for (j = 0; j < h; j++)
				gameData.boss [i].gateSegs [j] = m_cf.ReadShort ();
		if ((h = gameData.boss [i].nTeleportSegs))
			for (j = 0; j < h; j++)
				gameData.boss [i].teleportSegs [j] = m_cf.ReadShort ();
		}
	}
return 1;
}

//	-------------------------------------------------------------------------------------------------
