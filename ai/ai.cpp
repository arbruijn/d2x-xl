/* $Id: ai.c,v 1.7 2003/10/04 02:58:23 btb Exp $ */
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
#include "game.h"
#include "mono.h"
#include "3d.h"

#include "object.h"
#include "render.h"
#include "error.h"
#include "ai.h"
#include "laser.h"
#include "fvi.h"
#include "polyobj.h"
#include "bm.h"
#include "weapon.h"
#include "physics.h"
#include "collide.h"
#include "player.h"
#include "wall.h"
#include "vclip.h"
#include "fireball.h"
#include "morph.h"
#include "effects.h"
#include "timer.h"
#include "sounds.h"
#include "reactor.h"
#include "multibot.h"
#include "multi.h"
#include "network.h"
#include "loadgame.h"
#include "key.h"
#include "powerup.h"
#include "gauges.h"
#include "text.h"
#include "fuelcen.h"
#include "controls.h"
#include "input.h"
#include "gameseg.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#include "string.h"
//#define _DEBUG
#ifdef _DEBUG
#include <time.h>
#endif

// ----------------------------------------------------------------------------

void AIDoCloakStuff (void)
{
	int i;

for (i = 0; i < MAX_AI_CLOAK_INFO; i++) {
	gameData.ai.cloakInfo [i].vLastPos = OBJPOS (gameData.objs.console)->vPos;
	gameData.ai.cloakInfo [i].nLastSeg = OBJSEG (gameData.objs.console);
	gameData.ai.cloakInfo [i].lastTime = gameData.time.xGame;
	}
// Make work for control centers.
gameData.ai.vBelievedPlayerPos = gameData.ai.cloakInfo [0].vLastPos;
gameData.ai.nBelievedPlayerSeg = gameData.ai.cloakInfo [0].nLastSeg;
}

// ----------------------------------------------------------------------------
// Returns false if awareness is considered too puny to add, else returns true.
int AddAwarenessEvent (tObject *objP, int nType)
{
	// If tPlayer cloaked and hit a robot, then increase awareness
	if (nType >= PA_WEAPON_WALL_COLLISION)
		AIDoCloakStuff ();

	if (gameData.ai.nAwarenessEvents < MAX_AWARENESS_EVENTS) {
		if ((nType == PA_WEAPON_WALL_COLLISION) || (nType == PA_WEAPON_ROBOT_COLLISION))
			if (objP->id == VULCAN_ID)
				if (d_rand () > 3276)
					return 0;       // For vulcan cannon, only about 1/10 actually cause awareness

		gameData.ai.awarenessEvents [gameData.ai.nAwarenessEvents].nSegment = objP->nSegment;
		gameData.ai.awarenessEvents [gameData.ai.nAwarenessEvents].pos = objP->position.vPos;
		gameData.ai.awarenessEvents [gameData.ai.nAwarenessEvents].nType = nType;
		gameData.ai.nAwarenessEvents++;
	} else {
		//Int3 ();   // Hey -- Overflowed gameData.ai.awarenessEvents, make more or something
		// This just gets ignored, so you can just
		// continue.
	}
	return 1;

}

// ----------------------------------------------------------------------------------
// Robots will become aware of the tPlayer based on something that occurred.
// The tObject (probably tPlayer or weapon) which created the awareness is objP.
void CreateAwarenessEvent (tObject *objP, int nType)
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
for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
	if (IS_CHILD (gameData.segs.segments [nSegment].children [j])) {
		if (level <= 3) {
			if (nType == 4)
				pae_aux (gameData.segs.segments [nSegment].children [j], nType-1, level+1);
			else
				pae_aux (gameData.segs.segments [nSegment].children [j], nType, level+1);
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
	int i;

ProcessAwarenessEvents ();
for (i = 0; i <= gameData.objs.nLastObject; i++)
	if (gameData.objs.objects [i].controlType == CT_AI) {
		if (newAwareness [gameData.objs.objects [i].nSegment] > gameData.ai.localInfo [i].playerAwarenessType) {
			gameData.ai.localInfo [i].playerAwarenessType = newAwareness [gameData.objs.objects [i].nSegment];
			gameData.ai.localInfo [i].playerAwarenessTime = PLAYER_AWARENESS_INITIAL_TIME;
			}
		// Clear the bit that says this robot is only awake because a camera woke it up.
		if (newAwareness [gameData.objs.objects [i].nSegment] > gameData.ai.localInfo [i].playerAwarenessType)
			gameData.objs.objects [i].cType.aiInfo.SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		}
}

// ----------------------------------------------------------------------------------
// Do things which need to get done for all AI gameData.objs.objects each frame.
// This includes:
//  Setting player_awareness (a fix, time in seconds which tObject is aware of tPlayer)
void DoAIFrameAll (void)
{
	int	h, i, j;

SetPlayerAwarenessAll ();
if (gameData.ai.nLastMissileCamera != -1) {
	// Clear if supposed misisle camera is not a weapon, or just every so often, just in case.
	if (((gameData.app.nFrameCount & 0x0f) == 0) || (gameData.objs.objects [gameData.ai.nLastMissileCamera].nType != OBJ_WEAPON)) {
		gameData.ai.nLastMissileCamera = -1;
		for (i = 0; i <= gameData.objs.nLastObject; i++)
			if (gameData.objs.objects [i].nType == OBJ_ROBOT)
				gameData.objs.objects [i].cType.aiInfo.SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		}
	}
for (h = BOSS_COUNT, j = 0; j < h; j++)
	if (gameData.boss [j].nDying) {
		if (gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses)
			DoBossDyingFrame (gameData.objs.objects + gameData.boss [j].nDying);
		else {
			tObject *objP = gameData.objs.objects;
			for (i = 0; i <= gameData.objs.nLastObject; i++, objP++)
				if ((objP->nType == OBJ_ROBOT) && ROBOTINFO (objP->id).bossFlag)
					DoBossDyingFrame (objP);
		}
	}
}

//	-------------------------------------------------------------------------------------------------
