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

int AISaveBinState (CFile& cf)
{
	int	i;

cf.Write (&gameData.ai.bInitialized, sizeof (int), 1);
cf.Write (&gameData.ai.nOverallAgitation, sizeof (int), 1);
cf.Write (gameData.ai.localInfo, sizeof (tAILocalInfo), MAX_OBJECTS);
cf.Write (gameData.ai.pointSegs, sizeof (tPointSeg), MAX_POINT_SEGS);
cf.Write (gameData.ai.cloakInfo, sizeof (tAICloakInfo), MAX_AI_CLOAK_INFO);
for (i = 0; i < MAX_BOSS_COUNT; i++) {
	cf.Write (&gameData.boss [i].nCloakStartTime, sizeof (fix), 1);
	cf.Write (&gameData.boss [i].nCloakEndTime , sizeof (fix), 1);
	cf.Write (&gameData.boss [i].nLastTeleportTime , sizeof (fix), 1);
	cf.Write (&gameData.boss [i].nTeleportInterval, sizeof (fix), 1);
	cf.Write (&gameData.boss [i].nCloakInterval, sizeof (fix), 1);
	cf.Write (&gameData.boss [i].nCloakDuration, sizeof (fix), 1);
	cf.Write (&gameData.boss [i].nLastGateTime, sizeof (fix), 1);
	cf.Write (&gameData.boss [i].nGateInterval, sizeof (fix), 1);
	cf.Write (&gameData.boss [i].nDyingStartTime, sizeof (fix), 1);
	cf.Write (&gameData.boss [i].nDying, sizeof (int), 1);
	cf.Write (&gameData.boss [i].bDyingSoundPlaying, sizeof (int), 1);
	cf.Write (&gameData.boss [i].nHitTime, sizeof (fix), 1);
	}
cf.Write (&gameData.escort.nKillObject, sizeof (gameData.escort.nKillObject), 1);
cf.Write (&gameData.escort.xLastPathCreated, sizeof (gameData.escort.xLastPathCreated), 1);
cf.Write (&gameData.escort.nGoalObject, sizeof (gameData.escort.nGoalObject), 1);
cf.Write (&gameData.escort.nSpecialGoal, sizeof (gameData.escort.nSpecialGoal), 1);
cf.Write (&gameData.escort.nGoalIndex, sizeof (gameData.escort.nGoalIndex), 1);
cf.Write (&gameData.thief.stolenItems, sizeof (gameData.thief.stolenItems [0]), MAX_STOLEN_ITEMS);

i = (int) (gameData.ai.freePointSegs - gameData.ai.pointSegs);
cf.Write (&i, sizeof (int), 1);

for (i = 0; i < MAX_BOSS_COUNT; i++)
	cf.Write (&gameData.boss [i].nTeleportSegs, sizeof (gameData.boss [i].nTeleportSegs), 1);
for (i = 0; i < MAX_BOSS_COUNT; i++)
	cf.Write (&gameData.boss [i].nGateSegs, sizeof (gameData.boss [i].nGateSegs), 1);
for (i = 0; i < MAX_BOSS_COUNT; i++) {
	if (gameData.boss [i].nGateSegs)
		cf.Write (gameData.boss [i].gateSegs, sizeof (gameData.boss [i].gateSegs [0]), gameData.boss [i].nGateSegs);
	if (gameData.boss [i].nTeleportSegs)
		cf.Write (gameData.boss [i].teleportSegs, sizeof (gameData.boss [i].teleportSegs [0]), gameData.boss [i].nTeleportSegs);
	}
return 1;
}

//	-------------------------------------------------------------------------------------------------

int AIRestoreBinState (CFile& cf, int version)
{
	int	i;

memset (gameData.ai.localInfo, 0, sizeof (tAILocalInfo) * MAX_OBJECTS);
memset (gameData.ai.pointSegs, 0, sizeof (gameData.ai.pointSegs));
cf.Read (&gameData.ai.bInitialized, sizeof (int), 1);
cf.Read (&gameData.ai.nOverallAgitation, sizeof (int), 1);
cf.Read (gameData.ai.localInfo, sizeof (tAILocalInfo), (version > 22) ? MAX_OBJECTS : MAX_OBJECTS_D2);
cf.Read (gameData.ai.pointSegs, sizeof (tPointSeg), (version > 22) ? MAX_POINT_SEGS : MAX_POINT_SEGS_D2);
cf.Read (gameData.ai.cloakInfo, sizeof (tAICloakInfo), MAX_AI_CLOAK_INFO);
if (version < 29) {
	cf.Read (&gameData.boss [0].nCloakStartTime, sizeof (fix), 1);
	cf.Read (&gameData.boss [0].nCloakEndTime , sizeof (fix), 1);
	cf.Read (&gameData.boss [0].nLastTeleportTime , sizeof (fix), 1);
	cf.Read (&gameData.boss [0].nTeleportInterval, sizeof (fix), 1);
	cf.Read (&gameData.boss [0].nCloakInterval, sizeof (fix), 1);
	cf.Read (&gameData.boss [0].nCloakDuration, sizeof (fix), 1);
	cf.Read (&gameData.boss [0].nLastGateTime, sizeof (fix), 1);
	cf.Read (&gameData.boss [0].nGateInterval, sizeof (fix), 1);
	cf.Read (&gameData.boss [0].nDyingStartTime, sizeof (fix), 1);
	cf.Read (&gameData.boss [0].nDying, sizeof (int), 1);
	cf.Read (&gameData.boss [0].bDyingSoundPlaying, sizeof (int), 1);
	cf.Read (&gameData.boss [0].nHitTime, sizeof (fix), 1);
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
		cf.Read (&gameData.boss [i].nCloakStartTime, sizeof (fix), 1);
		cf.Read (&gameData.boss [i].nCloakEndTime , sizeof (fix), 1);
		cf.Read (&gameData.boss [i].nLastTeleportTime , sizeof (fix), 1);
		cf.Read (&gameData.boss [i].nTeleportInterval, sizeof (fix), 1);
		cf.Read (&gameData.boss [i].nCloakInterval, sizeof (fix), 1);
		cf.Read (&gameData.boss [i].nCloakDuration, sizeof (fix), 1);
		cf.Read (&gameData.boss [i].nLastGateTime, sizeof (fix), 1);
		cf.Read (&gameData.boss [i].nGateInterval, sizeof (fix), 1);
		cf.Read (&gameData.boss [i].nDyingStartTime, sizeof (fix), 1);
		cf.Read (&gameData.boss [i].nDying, sizeof (int), 1);
		cf.Read (&gameData.boss [i].bDyingSoundPlaying, sizeof (int), 1);
		cf.Read (&gameData.boss [i].nHitTime, sizeof (fix), 1);
		}
	}
// -- MK, 10/21/95, unused!-- cf.Read (&Boss_been_hit, sizeof (int), 1);

if (version >= 8) {
	cf.Read (&gameData.escort.nKillObject, sizeof (gameData.escort.nKillObject), 1);
	cf.Read (&gameData.escort.xLastPathCreated, sizeof (gameData.escort.xLastPathCreated), 1);
	cf.Read (&gameData.escort.nGoalObject, sizeof (gameData.escort.nGoalObject), 1);
	cf.Read (&gameData.escort.nSpecialGoal, sizeof (gameData.escort.nSpecialGoal), 1);
	cf.Read (&gameData.escort.nGoalIndex, sizeof (gameData.escort.nGoalIndex), 1);
	cf.Read (&gameData.thief.stolenItems, sizeof (gameData.thief.stolenItems [0]), MAX_STOLEN_ITEMS);
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
if (version >= 15) {
	int temp;
	cf.Read (&temp, sizeof (int), 1);
	gameData.ai.freePointSegs = gameData.ai.pointSegs + temp;
	}
else
	AIResetAllPaths ();
if (version >= 24) {
	for (i = 0; i < MAX_BOSS_COUNT; i++)
		cf.Read (&gameData.boss [i].nTeleportSegs, sizeof (gameData.boss [i].nTeleportSegs), 1);
	for (i = 0; i < MAX_BOSS_COUNT; i++)
		cf.Read (&gameData.boss [i].nGateSegs, sizeof (gameData.boss [i].nGateSegs), 1);
	for (i = 0; i < MAX_BOSS_COUNT; i++) {
		if (gameData.boss [i].nGateSegs)
			cf.Read (gameData.boss [i].gateSegs, sizeof (gameData.boss [i].gateSegs [0]), gameData.boss [i].nGateSegs);
		if (gameData.boss [i].nTeleportSegs)
			cf.Read (gameData.boss [i].teleportSegs, sizeof (gameData.boss [i].teleportSegs [0]), gameData.boss [i].nTeleportSegs);
		}
	}
else if (version >= 21) {
	short nTeleportSegs, nGateSegs;

	cf.Read (&nTeleportSegs, sizeof (nTeleportSegs), 1);
	cf.Read (&nGateSegs, sizeof (nGateSegs), 1);

	if (nGateSegs) {
		gameData.boss [0].nGateSegs = nGateSegs;
		cf.Read (gameData.boss [0].gateSegs, sizeof (gameData.boss [0].gateSegs [0]), nGateSegs);
		}
	if (nTeleportSegs) {
		gameData.boss [0].nTeleportSegs = nTeleportSegs;
		cf.Read (gameData.boss [0].teleportSegs, sizeof (gameData.boss [0].teleportSegs [0]), nTeleportSegs);
		}
} else {
	// -- gameData.boss.nTeleportSegs = 1;
	// -- gameData.boss.nGateSegs = 1;
	// -- gameData.boss.teleportSegs [0] = 0;
	// -- gameData.boss.gateSegs [0] = 0;
	// Note: Maybe better to leave alone...will probably be ok.
#if TRACE
	con_printf (1, "Warning: If you fight the boss, he might teleport to tSegment #0!\n");
#endif
	}
return 1;
}
//	-------------------------------------------------------------------------------------------------

void AISaveLocalInfo (tAILocalInfo *ailP, CFile& cf)
{
	int	i;

cf.WriteInt (ailP->playerAwarenessType);
cf.WriteInt (ailP->nRetryCount);
cf.WriteInt (ailP->nConsecutiveRetries);
cf.WriteInt (ailP->mode);
cf.WriteInt (ailP->nPrevVisibility);
cf.WriteInt (ailP->nRapidFireCount);
cf.WriteInt (ailP->nGoalSegment);
cf.WriteFix (ailP->nextActionTime);
cf.WriteFix (ailP->nextPrimaryFire);
cf.WriteFix (ailP->nextSecondaryFire);
cf.WriteFix (ailP->playerAwarenessTime);
cf.WriteFix (ailP->timePlayerSeen);
cf.WriteFix (ailP->timePlayerSoundAttacked);
cf.WriteFix (ailP->nextMiscSoundTime);
cf.WriteFix (ailP->timeSinceProcessed);
for (i = 0; i < MAX_SUBMODELS; i++) {
	cf.WriteAngVec (ailP->goalAngles[i]);
	cf.WriteAngVec (ailP->deltaAngles[i]);
	}
cf.Write (ailP->goalState, sizeof (ailP->goalState [0]), 1);
cf.Write (ailP->achievedState, sizeof (ailP->achievedState [0]), 1);
}

//	-------------------------------------------------------------------------------------------------

void AISavePointSeg (tPointSeg *psegP, CFile& cf)
{
cf.WriteInt (psegP->nSegment);
cf.WriteVector (psegP->point);
}

//	-------------------------------------------------------------------------------------------------

void AISaveCloakInfo (tAICloakInfo *ciP, CFile& cf)
{
cf.WriteFix (ciP->lastTime);
cf.WriteInt (ciP->nLastSeg);
cf.WriteVector (ciP->vLastPos);
}

//	-------------------------------------------------------------------------------------------------

int AISaveUniState (CFile& cf)
{
	int	h, i, j;

cf.WriteInt (gameData.ai.bInitialized);
cf.WriteInt (gameData.ai.nOverallAgitation);
for (i = 0; i < MAX_OBJECTS; i++)
	AISaveLocalInfo (gameData.ai.localInfo + i, cf);
for (i = 0; i < MAX_POINT_SEGS; i++)
	AISavePointSeg (gameData.ai.pointSegs + i, cf);
for (i = 0; i < MAX_AI_CLOAK_INFO; i++)
	AISaveCloakInfo (gameData.ai.cloakInfo + i, cf);
for (i = 0; i < MAX_BOSS_COUNT; i++) {
	cf.WriteShort (gameData.boss [i].nObject);
	cf.WriteFix (gameData.boss [i].nCloakStartTime);
	cf.WriteFix (gameData.boss [i].nCloakEndTime );
	cf.WriteFix (gameData.boss [i].nLastTeleportTime );
	cf.WriteFix (gameData.boss [i].nTeleportInterval);
	cf.WriteFix (gameData.boss [i].nCloakInterval);
	cf.WriteFix (gameData.boss [i].nCloakDuration);
	cf.WriteFix (gameData.boss [i].nLastGateTime);
	cf.WriteFix (gameData.boss [i].nGateInterval);
	cf.WriteFix (gameData.boss [i].nDyingStartTime);
	cf.WriteInt (gameData.boss [i].nDying);
	cf.WriteInt (gameData.boss [i].bDyingSoundPlaying);
	cf.WriteFix (gameData.boss [i].nHitTime);
	}
cf.WriteInt (gameData.escort.nKillObject);
cf.WriteFix (gameData.escort.xLastPathCreated);
cf.WriteInt (gameData.escort.nGoalObject);
cf.WriteInt (gameData.escort.nSpecialGoal);
cf.WriteInt (gameData.escort.nGoalIndex);
cf.Write (gameData.thief.stolenItems, sizeof (gameData.thief.stolenItems [0]), MAX_STOLEN_ITEMS);
#if DBG
i = CFTell ();
#endif
cf.WriteInt ((int) (gameData.ai.freePointSegs - gameData.ai.pointSegs));
for (i = 0; i < MAX_BOSS_COUNT; i++) {
	cf.WriteShort (gameData.boss [i].nTeleportSegs);
	cf.WriteShort (gameData.boss [i].nGateSegs);
	}
for (i = 0; i < MAX_BOSS_COUNT; i++) {
	if ((h = gameData.boss [i].nGateSegs))
		for (j = 0; j < h; j++)
			cf.WriteShort (gameData.boss [i].gateSegs [j]);
	if ((h = gameData.boss [i].nTeleportSegs))
		for (j = 0; j < h; j++)
			cf.WriteShort (gameData.boss [i].teleportSegs [j]);
	}
return 1;
}

//	-------------------------------------------------------------------------------------------------

void AIRestoreLocalInfo (tAILocalInfo *ailP, CFile& cf)
{
	int	i;

ailP->playerAwarenessType = cf.ReadInt ();
ailP->nRetryCount = cf.ReadInt ();
ailP->nConsecutiveRetries = cf.ReadInt ();
ailP->mode = cf.ReadInt ();
ailP->nPrevVisibility = cf.ReadInt ();
ailP->nRapidFireCount = cf.ReadInt ();
ailP->nGoalSegment = cf.ReadInt ();
ailP->nextActionTime = cf.ReadFix ();
ailP->nextPrimaryFire = cf.ReadFix ();
ailP->nextSecondaryFire = cf.ReadFix ();
ailP->playerAwarenessTime = cf.ReadFix ();
ailP->timePlayerSeen = cf.ReadFix ();
ailP->timePlayerSoundAttacked = cf.ReadFix ();
ailP->nextMiscSoundTime = cf.ReadFix ();
ailP->timeSinceProcessed = cf.ReadFix ();
for (i = 0; i < MAX_SUBMODELS; i++) {
	cf.ReadAngVec (ailP->goalAngles [i]);
	cf.ReadAngVec (ailP->deltaAngles [i]);
	}
cf.Read (ailP->goalState, sizeof (ailP->goalState [0]), 1);
cf.Read (ailP->achievedState, sizeof (ailP->achievedState [0]), 1);
}

//	-------------------------------------------------------------------------------------------------

void AIRestorePointSeg (tPointSeg *psegP, CFile& cf)
{
psegP->nSegment = cf.ReadInt ();
cf.ReadVector (psegP->point);
}

//	-------------------------------------------------------------------------------------------------

void AIRestoreCloakInfo (tAICloakInfo *ciP, CFile& cf)
{
ciP->lastTime = cf.ReadFix ();
ciP->nLastSeg = cf.ReadInt ();
cf.ReadVector (ciP->vLastPos);
}

//	-------------------------------------------------------------------------------------------------

int AIRestoreUniState (CFile& cf, int version)
{
	int	h, i, j, nMaxBossCount, nMaxPointSegs;

memset (gameData.ai.localInfo, 0, sizeof (*gameData.ai.localInfo) * MAX_OBJECTS);
gameData.ai.bInitialized = cf.ReadInt ();
gameData.ai.nOverallAgitation = cf.ReadInt ();
h = (version > 22) ? MAX_OBJECTS : MAX_OBJECTS_D2;
DBG (i = CFTell (fp));
for (i = 0; i < h; i++)
	AIRestoreLocalInfo (gameData.ai.localInfo + i, cf);
nMaxPointSegs = (version > 22) ? MAX_POINT_SEGS : MAX_POINT_SEGS_D2;
memset (gameData.ai.pointSegs, 0, sizeof (*gameData.ai.pointSegs) * nMaxPointSegs);
DBG (i = CFTell (fp));
for (i = 0; i < nMaxPointSegs; i++)
	AIRestorePointSeg (gameData.ai.pointSegs + i, cf);
DBG (i = CFTell (fp));
for (i = 0; i < MAX_AI_CLOAK_INFO; i++)
	AIRestoreCloakInfo (gameData.ai.cloakInfo + i, cf);
DBG (i = CFTell (fp));
if (version < 29) {
	gameData.boss [0].nCloakStartTime = cf.ReadFix ();
	gameData.boss [0].nCloakEndTime = cf.ReadFix ();
	gameData.boss [0].nLastTeleportTime = cf.ReadFix ();
	gameData.boss [0].nTeleportInterval = cf.ReadFix ();
	gameData.boss [0].nCloakInterval = cf.ReadFix ();
	gameData.boss [0].nCloakDuration = cf.ReadFix ();
	gameData.boss [0].nLastGateTime = cf.ReadFix ();
	gameData.boss [0].nGateInterval = cf.ReadFix ();
	gameData.boss [0].nDyingStartTime = cf.ReadFix ();
	gameData.boss [0].nDying = cf.ReadInt ();
	gameData.boss [0].bDyingSoundPlaying = cf.ReadInt ();
	gameData.boss [0].nHitTime = cf.ReadFix ();
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
		if (version > 31)
			gameData.boss [i].nObject = cf.ReadShort ();
		gameData.boss [i].nCloakStartTime = cf.ReadFix ();
		gameData.boss [i].nCloakEndTime = cf.ReadFix ();
		gameData.boss [i].nLastTeleportTime = cf.ReadFix ();
		gameData.boss [i].nTeleportInterval = cf.ReadFix ();
		gameData.boss [i].nCloakInterval = cf.ReadFix ();
		gameData.boss [i].nCloakDuration = cf.ReadFix ();
		gameData.boss [i].nLastGateTime = cf.ReadFix ();
		gameData.boss [i].nGateInterval = cf.ReadFix ();
		gameData.boss [i].nDyingStartTime = cf.ReadFix ();
		gameData.boss [i].nDying = cf.ReadInt ();
		gameData.boss [i].bDyingSoundPlaying = cf.ReadInt ();
		gameData.boss [i].nHitTime = cf.ReadFix ();
		}
	}
if (version >= 8) {
	gameData.escort.nKillObject = cf.ReadInt ();
	gameData.escort.xLastPathCreated = cf.ReadFix ();
	gameData.escort.nGoalObject = cf.ReadInt ();
	gameData.escort.nSpecialGoal = cf.ReadInt ();
	gameData.escort.nGoalIndex = cf.ReadInt ();
	cf.Read (&gameData.thief.stolenItems, sizeof (gameData.thief.stolenItems [0]), MAX_STOLEN_ITEMS);
	}
else {
	gameData.escort.nKillObject = -1;
	gameData.escort.xLastPathCreated = 0;
	gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
	gameData.escort.nSpecialGoal = -1;
	gameData.escort.nGoalIndex = -1;
	memset (gameData.thief.stolenItems, 255, sizeof (gameData.thief.stolenItems));
	}

if (version >= 15) {
	DBG (i = CFTell (fp));
	i = cf.ReadInt ();
	if ((i >= 0) && (i < nMaxPointSegs))
		gameData.ai.freePointSegs = gameData.ai.pointSegs + i;
	else
		AIResetAllPaths ();
	}
else
	AIResetAllPaths ();

if (version < 21) {
	#if TRACE
	con_printf (1, "Warning: If you fight the boss, he might teleport to tSegment #0!\n");
	#endif
	}
else {
	nMaxBossCount = (version >= 24) ? MAX_BOSS_COUNT : 1;
	for (i = 0; i < MAX_BOSS_COUNT; i++) {
		gameData.boss [i].nTeleportSegs = cf.ReadShort ();
		gameData.boss [i].nGateSegs = cf.ReadShort ();
		}
	for (i = 0; i < MAX_BOSS_COUNT; i++) {
		if ((h = gameData.boss [i].nGateSegs))
			for (j = 0; j < h; j++)
				gameData.boss [i].gateSegs [j] = cf.ReadShort ();
		if ((h = gameData.boss [i].nTeleportSegs))
			for (j = 0; j < h; j++)
				gameData.boss [i].teleportSegs [j] = cf.ReadShort ();
		}
	}
return 1;
}

//	-------------------------------------------------------------------------------------------------
