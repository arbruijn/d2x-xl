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

int AISaveBinState (CFILE *fp)
{
	int	i;

CFWrite (&gameData.ai.bInitialized, sizeof (int), 1, fp);
CFWrite (&gameData.ai.nOverallAgitation, sizeof (int), 1, fp);
CFWrite (gameData.ai.localInfo, sizeof (tAILocalInfo), MAX_OBJECTS, fp);
CFWrite (gameData.ai.pointSegs, sizeof (tPointSeg), MAX_POINT_SEGS, fp);
CFWrite (gameData.ai.cloakInfo, sizeof (tAICloakInfo), MAX_AI_CLOAK_INFO, fp);
for (i = 0; i < MAX_BOSS_COUNT; i++) {
	CFWrite (&gameData.boss [i].nCloakStartTime, sizeof (fix), 1, fp);
	CFWrite (&gameData.boss [i].nCloakEndTime , sizeof (fix), 1, fp);
	CFWrite (&gameData.boss [i].nLastTeleportTime , sizeof (fix), 1, fp);
	CFWrite (&gameData.boss [i].nTeleportInterval, sizeof (fix), 1, fp);
	CFWrite (&gameData.boss [i].nCloakInterval, sizeof (fix), 1, fp);
	CFWrite (&gameData.boss [i].nCloakDuration, sizeof (fix), 1, fp);
	CFWrite (&gameData.boss [i].nLastGateTime, sizeof (fix), 1, fp);
	CFWrite (&gameData.boss [i].nGateInterval, sizeof (fix), 1, fp);
	CFWrite (&gameData.boss [i].nDyingStartTime, sizeof (fix), 1, fp);
	CFWrite (&gameData.boss [i].nDying, sizeof (int), 1, fp);
	CFWrite (&gameData.boss [i].bDyingSoundPlaying, sizeof (int), 1, fp);
	CFWrite (&gameData.boss [i].nHitTime, sizeof (fix), 1, fp);
	}
CFWrite (&gameData.escort.nKillObject, sizeof (gameData.escort.nKillObject), 1, fp);
CFWrite (&gameData.escort.xLastPathCreated, sizeof (gameData.escort.xLastPathCreated), 1, fp);
CFWrite (&gameData.escort.nGoalObject, sizeof (gameData.escort.nGoalObject), 1, fp);
CFWrite (&gameData.escort.nSpecialGoal, sizeof (gameData.escort.nSpecialGoal), 1, fp);
CFWrite (&gameData.escort.nGoalIndex, sizeof (gameData.escort.nGoalIndex), 1, fp);
CFWrite (&gameData.thief.stolenItems, sizeof (gameData.thief.stolenItems [0]), MAX_STOLEN_ITEMS, fp);

i = (int) (gameData.ai.freePointSegs - gameData.ai.pointSegs);
CFWrite (&i, sizeof (int), 1, fp);

for (i = 0; i < MAX_BOSS_COUNT; i++)
	CFWrite (&gameData.boss [i].nTeleportSegs, sizeof (gameData.boss [i].nTeleportSegs), 1, fp);
for (i = 0; i < MAX_BOSS_COUNT; i++)
	CFWrite (&gameData.boss [i].nGateSegs, sizeof (gameData.boss [i].nGateSegs), 1, fp);
for (i = 0; i < MAX_BOSS_COUNT; i++) {
	if (gameData.boss [i].nGateSegs)
		CFWrite (gameData.boss [i].gateSegs, sizeof (gameData.boss [i].gateSegs [0]), gameData.boss [i].nGateSegs, fp);
	if (gameData.boss [i].nTeleportSegs)
		CFWrite (gameData.boss [i].teleportSegs, sizeof (gameData.boss [i].teleportSegs [0]), gameData.boss [i].nTeleportSegs, fp);
	}
return 1;
}

//	-------------------------------------------------------------------------------------------------

int AIRestoreBinState (CFILE *fp, int version)
{
	int	i;

memset (gameData.ai.localInfo, 0, sizeof (tAILocalInfo) * MAX_OBJECTS);
memset (gameData.ai.pointSegs, 0, sizeof (gameData.ai.pointSegs));
CFRead (&gameData.ai.bInitialized, sizeof (int), 1, fp);
CFRead (&gameData.ai.nOverallAgitation, sizeof (int), 1, fp);
CFRead (gameData.ai.localInfo, sizeof (tAILocalInfo), (version > 22) ? MAX_OBJECTS : MAX_OBJECTS_D2, fp);
CFRead (gameData.ai.pointSegs, sizeof (tPointSeg), (version > 22) ? MAX_POINT_SEGS : MAX_POINT_SEGS_D2, fp);
CFRead (gameData.ai.cloakInfo, sizeof (tAICloakInfo), MAX_AI_CLOAK_INFO, fp);
if (version < 29) {
	CFRead (&gameData.boss [0].nCloakStartTime, sizeof (fix), 1, fp);
	CFRead (&gameData.boss [0].nCloakEndTime , sizeof (fix), 1, fp);
	CFRead (&gameData.boss [0].nLastTeleportTime , sizeof (fix), 1, fp);
	CFRead (&gameData.boss [0].nTeleportInterval, sizeof (fix), 1, fp);
	CFRead (&gameData.boss [0].nCloakInterval, sizeof (fix), 1, fp);
	CFRead (&gameData.boss [0].nCloakDuration, sizeof (fix), 1, fp);
	CFRead (&gameData.boss [0].nLastGateTime, sizeof (fix), 1, fp);
	CFRead (&gameData.boss [0].nGateInterval, sizeof (fix), 1, fp);
	CFRead (&gameData.boss [0].nDyingStartTime, sizeof (fix), 1, fp);
	CFRead (&gameData.boss [0].nDying, sizeof (int), 1, fp);
	CFRead (&gameData.boss [0].bDyingSoundPlaying, sizeof (int), 1, fp);
	CFRead (&gameData.boss [0].nHitTime, sizeof (fix), 1, fp);
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
		CFRead (&gameData.boss [i].nCloakStartTime, sizeof (fix), 1, fp);
		CFRead (&gameData.boss [i].nCloakEndTime , sizeof (fix), 1, fp);
		CFRead (&gameData.boss [i].nLastTeleportTime , sizeof (fix), 1, fp);
		CFRead (&gameData.boss [i].nTeleportInterval, sizeof (fix), 1, fp);
		CFRead (&gameData.boss [i].nCloakInterval, sizeof (fix), 1, fp);
		CFRead (&gameData.boss [i].nCloakDuration, sizeof (fix), 1, fp);
		CFRead (&gameData.boss [i].nLastGateTime, sizeof (fix), 1, fp);
		CFRead (&gameData.boss [i].nGateInterval, sizeof (fix), 1, fp);
		CFRead (&gameData.boss [i].nDyingStartTime, sizeof (fix), 1, fp);
		CFRead (&gameData.boss [i].nDying, sizeof (int), 1, fp);
		CFRead (&gameData.boss [i].bDyingSoundPlaying, sizeof (int), 1, fp);
		CFRead (&gameData.boss [i].nHitTime, sizeof (fix), 1, fp);
		}
	}
// -- MK, 10/21/95, unused!-- CFRead (&Boss_been_hit, sizeof (int), 1, fp);

if (version >= 8) {
	CFRead (&gameData.escort.nKillObject, sizeof (gameData.escort.nKillObject), 1, fp);
	CFRead (&gameData.escort.xLastPathCreated, sizeof (gameData.escort.xLastPathCreated), 1, fp);
	CFRead (&gameData.escort.nGoalObject, sizeof (gameData.escort.nGoalObject), 1, fp);
	CFRead (&gameData.escort.nSpecialGoal, sizeof (gameData.escort.nSpecialGoal), 1, fp);
	CFRead (&gameData.escort.nGoalIndex, sizeof (gameData.escort.nGoalIndex), 1, fp);
	CFRead (&gameData.thief.stolenItems, sizeof (gameData.thief.stolenItems [0]), MAX_STOLEN_ITEMS, fp);
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
	CFRead (&temp, sizeof (int), 1, fp);
	gameData.ai.freePointSegs = gameData.ai.pointSegs + temp;
	}
else
	AIResetAllPaths ();
if (version >= 24) {
	for (i = 0; i < MAX_BOSS_COUNT; i++)
		CFRead (&gameData.boss [i].nTeleportSegs, sizeof (gameData.boss [i].nTeleportSegs), 1, fp);
	for (i = 0; i < MAX_BOSS_COUNT; i++)
		CFRead (&gameData.boss [i].nGateSegs, sizeof (gameData.boss [i].nGateSegs), 1, fp);
	for (i = 0; i < MAX_BOSS_COUNT; i++) {
		if (gameData.boss [i].nGateSegs)
			CFRead (gameData.boss [i].gateSegs, sizeof (gameData.boss [i].gateSegs [0]), gameData.boss [i].nGateSegs, fp);
		if (gameData.boss [i].nTeleportSegs)
			CFRead (gameData.boss [i].teleportSegs, sizeof (gameData.boss [i].teleportSegs [0]), gameData.boss [i].nTeleportSegs, fp);
		}
	}
else if (version >= 21) {
	short nTeleportSegs, nGateSegs;

	CFRead (&nTeleportSegs, sizeof (nTeleportSegs), 1, fp);
	CFRead (&nGateSegs, sizeof (nGateSegs), 1, fp);

	if (nGateSegs) {
		gameData.boss [0].nGateSegs = nGateSegs;
		CFRead (gameData.boss [0].gateSegs, sizeof (gameData.boss [0].gateSegs [0]), nGateSegs, fp);
		}
	if (nTeleportSegs) {
		gameData.boss [0].nTeleportSegs = nTeleportSegs;
		CFRead (gameData.boss [0].teleportSegs, sizeof (gameData.boss [0].teleportSegs [0]), nTeleportSegs, fp);
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

void AISaveLocalInfo (tAILocalInfo *ailP, CFILE *fp)
{
	int	i;

CFWriteInt (ailP->playerAwarenessType, fp);
CFWriteInt (ailP->nRetryCount, fp);
CFWriteInt (ailP->nConsecutiveRetries, fp);
CFWriteInt (ailP->mode, fp);
CFWriteInt (ailP->nPrevVisibility, fp);
CFWriteInt (ailP->nRapidFireCount, fp);
CFWriteInt (ailP->nGoalSegment, fp);
CFWriteFix (ailP->nextActionTime, fp);
CFWriteFix (ailP->nextPrimaryFire, fp);
CFWriteFix (ailP->nextSecondaryFire, fp);
CFWriteFix (ailP->playerAwarenessTime, fp);
CFWriteFix (ailP->timePlayerSeen, fp);
CFWriteFix (ailP->timePlayerSoundAttacked, fp);
CFWriteFix (ailP->nextMiscSoundTime, fp);
CFWriteFix (ailP->timeSinceProcessed, fp);
for (i = 0; i < MAX_SUBMODELS; i++) {
	CFWriteAngVec (ailP->goalAngles[i], fp);
	CFWriteAngVec (ailP->deltaAngles[i], fp);
	}
CFWrite (ailP->goalState, sizeof (ailP->goalState [0]), 1, fp);
CFWrite (ailP->achievedState, sizeof (ailP->achievedState [0]), 1, fp);
}

//	-------------------------------------------------------------------------------------------------

void AISavePointSeg (tPointSeg *psegP, CFILE *fp)
{
CFWriteInt (psegP->nSegment, fp);
CFWriteVector (psegP->point, fp);
}

//	-------------------------------------------------------------------------------------------------

void AISaveCloakInfo (tAICloakInfo *ciP, CFILE *fp)
{
CFWriteFix (ciP->lastTime, fp);
CFWriteInt (ciP->nLastSeg, fp);
CFWriteVector (ciP->vLastPos, fp);
}

//	-------------------------------------------------------------------------------------------------

int AISaveUniState (CFILE *fp)
{
	int	h, i, j;

CFWriteInt (gameData.ai.bInitialized, fp);
CFWriteInt (gameData.ai.nOverallAgitation, fp);
for (i = 0; i < MAX_OBJECTS; i++)
	AISaveLocalInfo (gameData.ai.localInfo + i, fp);
for (i = 0; i < MAX_POINT_SEGS; i++)
	AISavePointSeg (gameData.ai.pointSegs + i, fp);
for (i = 0; i < MAX_AI_CLOAK_INFO; i++)
	AISaveCloakInfo (gameData.ai.cloakInfo + i, fp);
for (i = 0; i < MAX_BOSS_COUNT; i++) {
	CFWriteShort (gameData.boss [i].nObject, fp);
	CFWriteFix (gameData.boss [i].nCloakStartTime, fp);
	CFWriteFix (gameData.boss [i].nCloakEndTime , fp);
	CFWriteFix (gameData.boss [i].nLastTeleportTime , fp);
	CFWriteFix (gameData.boss [i].nTeleportInterval, fp);
	CFWriteFix (gameData.boss [i].nCloakInterval, fp);
	CFWriteFix (gameData.boss [i].nCloakDuration, fp);
	CFWriteFix (gameData.boss [i].nLastGateTime, fp);
	CFWriteFix (gameData.boss [i].nGateInterval, fp);
	CFWriteFix (gameData.boss [i].nDyingStartTime, fp);
	CFWriteInt (gameData.boss [i].nDying, fp);
	CFWriteInt (gameData.boss [i].bDyingSoundPlaying, fp);
	CFWriteFix (gameData.boss [i].nHitTime, fp);
	}
CFWriteInt (gameData.escort.nKillObject, fp);
CFWriteFix (gameData.escort.xLastPathCreated, fp);
CFWriteInt (gameData.escort.nGoalObject, fp);
CFWriteInt (gameData.escort.nSpecialGoal, fp);
CFWriteInt (gameData.escort.nGoalIndex, fp);
CFWrite (gameData.thief.stolenItems, sizeof (gameData.thief.stolenItems [0]), MAX_STOLEN_ITEMS, fp);
#if DBG
i = CFTell (fp);
#endif
CFWriteInt ((int) (gameData.ai.freePointSegs - gameData.ai.pointSegs), fp);
for (i = 0; i < MAX_BOSS_COUNT; i++) {
	CFWriteShort (gameData.boss [i].nTeleportSegs, fp);
	CFWriteShort (gameData.boss [i].nGateSegs, fp);
	}
for (i = 0; i < MAX_BOSS_COUNT; i++) {
	if ((h = gameData.boss [i].nGateSegs))
		for (j = 0; j < h; j++)
			CFWriteShort (gameData.boss [i].gateSegs [j], fp);
	if ((h = gameData.boss [i].nTeleportSegs))
		for (j = 0; j < h; j++)
			CFWriteShort (gameData.boss [i].teleportSegs [j], fp);
	}
return 1;
}

//	-------------------------------------------------------------------------------------------------

void AIRestoreLocalInfo (tAILocalInfo *ailP, CFILE *fp)
{
	int	i;

ailP->playerAwarenessType = CFReadInt (fp);
ailP->nRetryCount = CFReadInt (fp);
ailP->nConsecutiveRetries = CFReadInt (fp);
ailP->mode = CFReadInt (fp);
ailP->nPrevVisibility = CFReadInt (fp);
ailP->nRapidFireCount = CFReadInt (fp);
ailP->nGoalSegment = CFReadInt (fp);
ailP->nextActionTime = CFReadFix (fp);
ailP->nextPrimaryFire = CFReadFix (fp);
ailP->nextSecondaryFire = CFReadFix (fp);
ailP->playerAwarenessTime = CFReadFix (fp);
ailP->timePlayerSeen = CFReadFix (fp);
ailP->timePlayerSoundAttacked = CFReadFix (fp);
ailP->nextMiscSoundTime = CFReadFix (fp);
ailP->timeSinceProcessed = CFReadFix (fp);
for (i = 0; i < MAX_SUBMODELS; i++) {
	CFReadAngVec (ailP->goalAngles [i], fp);
	CFReadAngVec (ailP->deltaAngles [i], fp);
	}
CFRead (ailP->goalState, sizeof (ailP->goalState [0]), 1, fp);
CFRead (ailP->achievedState, sizeof (ailP->achievedState [0]), 1, fp);
}

//	-------------------------------------------------------------------------------------------------

void AIRestorePointSeg (tPointSeg *psegP, CFILE *fp)
{
psegP->nSegment = CFReadInt (fp);
CFReadVector (psegP->point, fp);
}

//	-------------------------------------------------------------------------------------------------

void AIRestoreCloakInfo (tAICloakInfo *ciP, CFILE *fp)
{
ciP->lastTime = CFReadFix (fp);
ciP->nLastSeg = CFReadInt (fp);
CFReadVector (ciP->vLastPos, fp);
}

//	-------------------------------------------------------------------------------------------------

int AIRestoreUniState (CFILE *fp, int version)
{
	int	h, i, j, nMaxBossCount, nMaxPointSegs;

memset (gameData.ai.localInfo, 0, sizeof (*gameData.ai.localInfo) * MAX_OBJECTS);
gameData.ai.bInitialized = CFReadInt (fp);
gameData.ai.nOverallAgitation = CFReadInt (fp);
h = (version > 22) ? MAX_OBJECTS : MAX_OBJECTS_D2;
DBG (i = CFTell (fp));
for (i = 0; i < h; i++)
	AIRestoreLocalInfo (gameData.ai.localInfo + i, fp);
nMaxPointSegs = (version > 22) ? MAX_POINT_SEGS : MAX_POINT_SEGS_D2;
memset (gameData.ai.pointSegs, 0, sizeof (*gameData.ai.pointSegs) * nMaxPointSegs);
DBG (i = CFTell (fp));
for (i = 0; i < nMaxPointSegs; i++)
	AIRestorePointSeg (gameData.ai.pointSegs + i, fp);
DBG (i = CFTell (fp));
for (i = 0; i < MAX_AI_CLOAK_INFO; i++)
	AIRestoreCloakInfo (gameData.ai.cloakInfo + i, fp);
DBG (i = CFTell (fp));
if (version < 29) {
	gameData.boss [0].nCloakStartTime = CFReadFix (fp);
	gameData.boss [0].nCloakEndTime = CFReadFix (fp);
	gameData.boss [0].nLastTeleportTime = CFReadFix (fp);
	gameData.boss [0].nTeleportInterval = CFReadFix (fp);
	gameData.boss [0].nCloakInterval = CFReadFix (fp);
	gameData.boss [0].nCloakDuration = CFReadFix (fp);
	gameData.boss [0].nLastGateTime = CFReadFix (fp);
	gameData.boss [0].nGateInterval = CFReadFix (fp);
	gameData.boss [0].nDyingStartTime = CFReadFix (fp);
	gameData.boss [0].nDying = CFReadInt (fp);
	gameData.boss [0].bDyingSoundPlaying = CFReadInt (fp);
	gameData.boss [0].nHitTime = CFReadFix (fp);
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
			gameData.boss [i].nObject = CFReadShort (fp);
		gameData.boss [i].nCloakStartTime = CFReadFix (fp);
		gameData.boss [i].nCloakEndTime = CFReadFix (fp);
		gameData.boss [i].nLastTeleportTime = CFReadFix (fp);
		gameData.boss [i].nTeleportInterval = CFReadFix (fp);
		gameData.boss [i].nCloakInterval = CFReadFix (fp);
		gameData.boss [i].nCloakDuration = CFReadFix (fp);
		gameData.boss [i].nLastGateTime = CFReadFix (fp);
		gameData.boss [i].nGateInterval = CFReadFix (fp);
		gameData.boss [i].nDyingStartTime = CFReadFix (fp);
		gameData.boss [i].nDying = CFReadInt (fp);
		gameData.boss [i].bDyingSoundPlaying = CFReadInt (fp);
		gameData.boss [i].nHitTime = CFReadFix (fp);
		}
	}
if (version >= 8) {
	gameData.escort.nKillObject = CFReadInt (fp);
	gameData.escort.xLastPathCreated = CFReadFix (fp);
	gameData.escort.nGoalObject = CFReadInt (fp);
	gameData.escort.nSpecialGoal = CFReadInt (fp);
	gameData.escort.nGoalIndex = CFReadInt (fp);
	CFRead (&gameData.thief.stolenItems, sizeof (gameData.thief.stolenItems [0]), MAX_STOLEN_ITEMS, fp);
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
	i = CFReadInt (fp);
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
		gameData.boss [i].nTeleportSegs = CFReadShort (fp);
		gameData.boss [i].nGateSegs = CFReadShort (fp);
		}
	for (i = 0; i < MAX_BOSS_COUNT; i++) {
		if ((h = gameData.boss [i].nGateSegs))
			for (j = 0; j < h; j++)
				gameData.boss [i].gateSegs [j] = CFReadShort (fp);
		if ((h = gameData.boss [i].nTeleportSegs))
			for (j = 0; j < h; j++)
				gameData.boss [i].teleportSegs [j] = CFReadShort (fp);
		}
	}
return 1;
}

//	-------------------------------------------------------------------------------------------------
