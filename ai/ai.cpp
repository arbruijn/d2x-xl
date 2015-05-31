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
ENTER (0);
	CObject*	pObj = TARGETOBJ ? TARGETOBJ : gameData.objData.pConsole;

for (int32_t i = 0; i < MAX_AI_CLOAK_INFO; i++) {
	gameData.aiData.cloakInfo [i].vLastPos = OBJPOS (pObj)->vPos;
	gameData.aiData.cloakInfo [i].nLastSeg = OBJSEG (pObj);
	gameData.aiData.cloakInfo [i].lastTime = gameData.timeData.xGame;
	}
// Make work for control centers.
gameData.aiData.target.vBelievedPos = gameData.aiData.cloakInfo [0].vLastPos;
gameData.aiData.target.nBelievedSeg = gameData.aiData.cloakInfo [0].nLastSeg;
LEAVE
}

// ----------------------------------------------------------------------------
// Returns false if awareness is considered too puny to add, else returns true.
int32_t AddAwarenessEvent (CObject *pObj, int32_t nType)
{
ENTER (0);
	// If player cloaked and hit a robot, then increase awareness
if (nType >= WEAPON_WALL_COLLISION)
	AIDoCloakStuff ();

if (gameData.aiData.nAwarenessEvents < MAX_AWARENESS_EVENTS) {
	if ((nType == WEAPON_WALL_COLLISION) || (nType == WEAPON_ROBOT_COLLISION))
		if (pObj->info.nId == VULCAN_ID)
			if (RandShort () > 3276)
				RETURN (0)       // For vulcan cannon, only about 1/10 actually cause awareness
	gameData.aiData.awarenessEvents [gameData.aiData.nAwarenessEvents].nSegment = pObj->info.nSegment;
	gameData.aiData.awarenessEvents [gameData.aiData.nAwarenessEvents].pos = pObj->info.position.vPos;
	gameData.aiData.awarenessEvents [gameData.aiData.nAwarenessEvents].nType = nType;
	gameData.aiData.nAwarenessEvents++;
	} 
RETURN (1)
}

// ----------------------------------------------------------------------------------
// Robots will become aware of the player based on something that occurred.
// The CObject (probably player or weapon) which created the awareness is pObj.
void CreateAwarenessEvent (CObject *pObj, int32_t nType)
{
	// If not in multiplayer, or in multiplayer with robots, do this, else unnecessary!
ENTER (0);
if (IsRobotGame) {
	if (AddAwarenessEvent (pObj, nType)) {
		if (((RandShort () * (nType + 4)) >> 15) > 4)
			gameData.aiData.nOverallAgitation++;
		if (gameData.aiData.nOverallAgitation > OVERALL_AGITATION_MAX)
			gameData.aiData.nOverallAgitation = OVERALL_AGITATION_MAX;
		}
	}
LEAVE
}

int8_t newAwareness [MAX_SEGMENTS_D2X];

// ----------------------------------------------------------------------------------

void pae_aux (int32_t nSegment, int32_t nType, int32_t level)
{
ENTER (0);
CSegment* pSeg = SEGMENT (nSegment);
if (pSeg) {
	if (newAwareness [nSegment] < nType)
		newAwareness [nSegment] = nType;
	for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++) {
		if (IS_CHILD (pSeg->m_children [i])) {
			if (level <= 3) {
				pae_aux (pSeg->m_children [i], (nType == 4) ? nType - 1 : nType, level + 1);
				}
			}
		}
	}
LEAVE
}

// ----------------------------------------------------------------------------------

void ProcessAwarenessEvents (void)
{
ENTER (0);
if (IsRobotGame) {
	memset (newAwareness, 0, sizeof (newAwareness [0]) * gameData.segData.nSegments);
	for (int32_t i = 0; i < gameData.aiData.nAwarenessEvents; i++)
		pae_aux (gameData.aiData.awarenessEvents [i].nSegment, gameData.aiData.awarenessEvents [i].nType, 1);
	}
gameData.aiData.nAwarenessEvents = 0;
LEAVE
}

// ----------------------------------------------------------------------------------

void SetPlayerAwarenessAll (void)
{
ENTER (0);
	int32_t	i;
	int16_t	nSegment;
	CObject	*pObj;

ProcessAwarenessEvents ();
FORALL_OBJS (pObj)
	if (pObj->info.controlType == CT_AI) {
		i = pObj->Index ();
		nSegment = OBJECT (i)->info.nSegment;
		if (newAwareness [nSegment] > gameData.aiData.localInfo [i].targetAwarenessType) {
			gameData.aiData.localInfo [i].targetAwarenessType = newAwareness [nSegment];
			gameData.aiData.localInfo [i].targetAwarenessTime = PLAYER_AWARENESS_INITIAL_TIME;
			}
		// Clear the bit that says this robot is only awake because a camera woke it up.
		if (newAwareness [nSegment] > gameData.aiData.localInfo [i].targetAwarenessType)
			pObj->cType.aiInfo.SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		}
LEAVE
}

// ----------------------------------------------------------------------------------
// Do things which need to get done for all AI OBJECTS each frame.
// This includes:
//  Setting player_awareness (a fix, time in seconds which CObject is aware of player)
void DoAIFrameAll (void)
{
ENTER (0);
SetPlayerAwarenessAll ();
if (USE_D1_AI)
	LEAVE
CObject *pObj = OBJECT (gameData.aiData.nLastMissileCamera);
if (pObj) {
	// Clear if supposed misisle camera is not a weapon, or just every so often, just in case.
	if (((gameData.appData.nFrameCount & 0x0f) == 0) || (pObj->Type () != OBJ_WEAPON)) {
		gameData.aiData.nLastMissileCamera = -1;
		FORALL_ROBOT_OBJS (pObj) {
			pObj->cType.aiInfo.SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
			}
		}
	}
for (int32_t h = gameData.bossData.ToS (), j = 0; j < h; j++)
	if (gameData.bossData [j].m_nDying) {
		if (gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses)
			DoBossDyingFrame (OBJECT (gameData.bossData [j].m_nDying));
		else {
			CObject *pObj = OBJECTS.Buffer ();
			FORALL_ROBOT_OBJS (pObj) {
				if (pObj->IsBoss ())
					DoBossDyingFrame (pObj);
				}
		}
	}
LEAVE
}

//	-------------------------------------------------------------------------------------------------
