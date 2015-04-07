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
#include "network.h"

#include "string.h"

#if DBG
#include <time.h>
#endif

// ----------------------------------------------------------------------------

void AIDoCloakStuff (void)
{
	CObject*	objP = TARGETOBJ ? TARGETOBJ : gameData.objs.consoleP;

for (int32_t i = 0; i < MAX_AI_CLOAK_INFO; i++) {
	gameData.ai.cloakInfo [i].vLastPos = OBJPOS (objP)->vPos;
	gameData.ai.cloakInfo [i].nLastSeg = OBJSEG (objP);
	gameData.ai.cloakInfo [i].lastTime = gameData.time.xGame;
	}
// Make work for control centers.
gameData.ai.target.vBelievedPos = gameData.ai.cloakInfo [0].vLastPos;
gameData.ai.target.nBelievedSeg = gameData.ai.cloakInfo [0].nLastSeg;
}

// ----------------------------------------------------------------------------
// Returns false if awareness is considered too puny to add, else returns true.
int32_t AddAwarenessEvent (CObject *objP, int32_t nType)
{
	// If player cloaked and hit a robot, then increase awareness
if (nType >= WEAPON_WALL_COLLISION)
	AIDoCloakStuff ();

if (gameData.ai.nAwarenessEvents < MAX_AWARENESS_EVENTS) {
	if ((nType == WEAPON_WALL_COLLISION) || (nType == WEAPON_ROBOT_COLLISION))
		if (objP->info.nId == VULCAN_ID)
			if (RandShort () > 3276)
				return 0;       // For vulcan cannon, only about 1/10 actually cause awareness
	gameData.ai.awarenessEvents [gameData.ai.nAwarenessEvents].nSegment = objP->info.nSegment;
	gameData.ai.awarenessEvents [gameData.ai.nAwarenessEvents].pos = objP->info.position.vPos;
	gameData.ai.awarenessEvents [gameData.ai.nAwarenessEvents].nType = nType;
	gameData.ai.nAwarenessEvents++;
	} 
return 1;
}

// ----------------------------------------------------------------------------------
// Robots will become aware of the player based on something that occurred.
// The CObject (probably player or weapon) which created the awareness is objP.
void CreateAwarenessEvent (CObject *objP, int32_t nType)
{
	// If not in multiplayer, or in multiplayer with robots, do this, else unnecessary!
if (IsRobotGame) {
	if (AddAwarenessEvent (objP, nType)) {
		if (((RandShort () * (nType+4)) >> 15) > 4)
			gameData.ai.nOverallAgitation++;
		if (gameData.ai.nOverallAgitation > OVERALL_AGITATION_MAX)
			gameData.ai.nOverallAgitation = OVERALL_AGITATION_MAX;
		}
	}
}

int8_t newAwareness [MAX_SEGMENTS_D2X];

// ----------------------------------------------------------------------------------

void pae_aux (int32_t nSegment, int32_t nType, int32_t level)
{
if ((nSegment >= 0) && (nSegment < gameData.segs.nSegments)) {
	if (newAwareness [nSegment] < nType)
		newAwareness [nSegment] = nType;
	CSegment* segP = gameData.Segment (nSegment);
	for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++) {
		if (IS_CHILD (segP->m_children [i])) {
			if (level <= 3) {
				pae_aux (segP->m_children [i], (nType == 4) ? nType - 1 : nType, level + 1);
				}
			}
		}
	}
}

// ----------------------------------------------------------------------------------

void ProcessAwarenessEvents (void)
{
if (IsRobotGame) {
	memset (newAwareness, 0, sizeof (newAwareness [0]) * gameData.segs.nSegments);
	for (int32_t i = 0; i < gameData.ai.nAwarenessEvents; i++)
		pae_aux (gameData.ai.awarenessEvents [i].nSegment, gameData.ai.awarenessEvents [i].nType, 1);
	}
gameData.ai.nAwarenessEvents = 0;
}

// ----------------------------------------------------------------------------------

void SetPlayerAwarenessAll (void)
{
	int32_t		i;
	int16_t		nSegment;
	CObject	*objP;

ProcessAwarenessEvents ();
FORALL_OBJS (objP)
	if (objP->info.controlType == CT_AI) {
		i = objP->Index ();
		nSegment = gameData.Object (i)->info.nSegment;
		if (newAwareness [nSegment] > gameData.ai.localInfo [i].targetAwarenessType) {
			gameData.ai.localInfo [i].targetAwarenessType = newAwareness [nSegment];
			gameData.ai.localInfo [i].targetAwarenessTime = PLAYER_AWARENESS_INITIAL_TIME;
			}
		// Clear the bit that says this robot is only awake because a camera woke it up.
		if (newAwareness [nSegment] > gameData.ai.localInfo [i].targetAwarenessType)
			objP->cType.aiInfo.SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		}
}

// ----------------------------------------------------------------------------------
// Do things which need to get done for all AI OBJECTS each frame.
// This includes:
//  Setting player_awareness (a fix, time in seconds which CObject is aware of player)
void DoAIFrameAll (void)
{
	int32_t	h, j;
	CObject*	objP;

SetPlayerAwarenessAll ();
if (USE_D1_AI)
	return;
if (gameData.ai.nLastMissileCamera != -1) {
	// Clear if supposed misisle camera is not a weapon, or just every so often, just in case.
	if (((gameData.app.nFrameCount & 0x0f) == 0) || (gameData.Object (gameData.ai.nLastMissileCamera)->info.nType != OBJ_WEAPON)) {
		gameData.ai.nLastMissileCamera = -1;
		FORALL_ROBOT_OBJS (objP) {
			objP->cType.aiInfo.SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
			}
		}
	}
for (h = gameData.bosses.ToS (), j = 0; j < h; j++)
	if (gameData.bosses [j].m_nDying) {
		if (gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses)
			DoBossDyingFrame (gameData.Object (gameData.bosses [j].m_nDying));
		else {
			CObject *objP = OBJECTS.Buffer ();
			FORALL_ROBOT_OBJS (objP)
				if (ROBOTINFO (objP->info.nId).bossFlag)
					DoBossDyingFrame (objP);
		}
	}
}

//	-------------------------------------------------------------------------------------------------
