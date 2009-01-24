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
#include "network.h"

#include "string.h"
//#define _DEBUG
#if DBG
#include <time.h>
#endif

// ----------------------------------------------------------------------------

void AIDoCloakStuff (void)
{
	int i;

for (i = 0; i < MAX_AI_CLOAK_INFO; i++) {
	gameData.ai.cloakInfo [i].vLastPos = OBJPOS (gameData.objs.consoleP)->vPos;
	gameData.ai.cloakInfo [i].nLastSeg = OBJSEG (gameData.objs.consoleP);
	gameData.ai.cloakInfo [i].lastTime = gameData.time.xGame;
	}
// Make work for control centers.
gameData.ai.vBelievedPlayerPos = gameData.ai.cloakInfo [0].vLastPos;
gameData.ai.nBelievedPlayerSeg = gameData.ai.cloakInfo [0].nLastSeg;
}

// ----------------------------------------------------------------------------
// Returns false if awareness is considered too puny to add, else returns true.
int AddAwarenessEvent (CObject *objP, int nType)
{
	// If CPlayerData cloaked and hit a robot, then increase awareness
if (nType >= WEAPON_WALL_COLLISION)
	AIDoCloakStuff ();

if (gameData.ai.nAwarenessEvents < MAX_AWARENESS_EVENTS) {
	if ((nType == WEAPON_WALL_COLLISION) || (nType == WEAPON_ROBOT_COLLISION))
		if (objP->info.nId == VULCAN_ID)
			if (d_rand () > 3276)
				return 0;       // For vulcan cannon, only about 1/10 actually cause awareness
	gameData.ai.awarenessEvents [gameData.ai.nAwarenessEvents].nSegment = objP->info.nSegment;
	gameData.ai.awarenessEvents [gameData.ai.nAwarenessEvents].pos = objP->info.position.vPos;
	gameData.ai.awarenessEvents [gameData.ai.nAwarenessEvents].nType = nType;
	gameData.ai.nAwarenessEvents++;
	} 
return 1;
}

// ----------------------------------------------------------------------------------
// Robots will become aware of the CPlayerData based on something that occurred.
// The CObject (probably CPlayerData or weapon) which created the awareness is objP.
void CreateAwarenessEvent (CObject *objP, int nType)
{
	// If not in multiplayer, or in multiplayer with robots, do this, else unnecessary!
if (IsRobotGame) {
	if (AddAwarenessEvent (objP, nType)) {
		if (((d_rand () * (nType+4)) >> 15) > 4)
			gameData.ai.nOverallAgitation++;
		if (gameData.ai.nOverallAgitation > OVERALL_AGITATION_MAX)
			gameData.ai.nOverallAgitation = OVERALL_AGITATION_MAX;
		}
	}
}

sbyte newAwareness [MAX_SEGMENTS_D2X];

// ----------------------------------------------------------------------------------

void pae_aux (int nSegment, int nType, int level)
{
	int j;

if (newAwareness [nSegment] < nType)
	newAwareness [nSegment] = nType;
// Process children.
for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++)
	if (IS_CHILD (SEGMENTS [nSegment].m_children [j])) {
		if (level <= 3) {
			if (nType == 4)
				pae_aux (SEGMENTS [nSegment].m_children [j], nType-1, level+1);
			else
				pae_aux (SEGMENTS [nSegment].m_children [j], nType, level+1);
			}
		}
}


// ----------------------------------------------------------------------------------

void ProcessAwarenessEvents (void)
{
	int i;

if (IsRobotGame) {
	memset (newAwareness, 0, sizeof (newAwareness [0]) * gameData.segs.nSegments);
	for (i = 0; i < gameData.ai.nAwarenessEvents; i++)
		pae_aux (gameData.ai.awarenessEvents [i].nSegment, gameData.ai.awarenessEvents [i].nType, 1);
	}
gameData.ai.nAwarenessEvents = 0;
}

// ----------------------------------------------------------------------------------

void SetPlayerAwarenessAll (void)
{
	int		i;
	short		nSegment;
	CObject	*objP;

ProcessAwarenessEvents ();
FORALL_OBJS (objP, i)
	if (objP->info.controlType == CT_AI) {
		i = objP->Index ();
		nSegment = OBJECTS [i].info.nSegment;
		if (newAwareness [nSegment] > gameData.ai.localInfo [i].playerAwarenessType) {
			gameData.ai.localInfo [i].playerAwarenessType = newAwareness [nSegment];
			gameData.ai.localInfo [i].playerAwarenessTime = PLAYER_AWARENESS_INITIAL_TIME;
			}
		// Clear the bit that says this robot is only awake because a camera woke it up.
		if (newAwareness [nSegment] > gameData.ai.localInfo [i].playerAwarenessType)
			objP->cType.aiInfo.SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		}
}

// ----------------------------------------------------------------------------------
// Do things which need to get done for all AI OBJECTS each frame.
// This includes:
//  Setting player_awareness (a fix, time in seconds which CObject is aware of CPlayerData)
void DoAIFrameAll (void)
{
	int		h, j;
	//int		i;
	CObject	*objP;

SetPlayerAwarenessAll ();
if (USE_D1_AI)
	return;
if (gameData.ai.nLastMissileCamera != -1) {
	// Clear if supposed misisle camera is not a weapon, or just every so often, just in case.
	if (((gameData.app.nFrameCount & 0x0f) == 0) || (OBJECTS [gameData.ai.nLastMissileCamera].info.nType != OBJ_WEAPON)) {
		gameData.ai.nLastMissileCamera = -1;
		FORALL_ROBOT_OBJS (objP, i) {
			objP->cType.aiInfo.SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
			}
		}
	}
for (h = BOSS_COUNT, j = 0; j < h; j++)
	if (gameData.bosses [j].m_nDying) {
		if (gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses)
			DoBossDyingFrame (OBJECTS + gameData.bosses [j].m_nDying);
		else {
			CObject *objP = OBJECTS.Buffer ();
			FORALL_ROBOT_OBJS (objP, i)
				if (ROBOTINFO (objP->info.nId).bossFlag)
					DoBossDyingFrame (objP);
		}
	}
}

//	-------------------------------------------------------------------------------------------------
