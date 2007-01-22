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

char ai_rcsid [] = "$Id: ai.c,v 1.7 2003/10/04 02:58:23 btb Exp $";

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
#include "cntrlcen.h"
#include "multibot.h"
#include "multi.h"
#include "network.h"
#include "gameseq.h"
#include "key.h"
#include "powerup.h"
#include "gauges.h"
#include "text.h"
#include "fuelcen.h"
#include "controls.h"
#include "kconfig.h"
#include "gameseg.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#include "string.h"

#ifndef NDEBUG
#include <time.h>
#endif

// ---------- John: These variables must be saved as part of gamesave. --------

#if 0

ubyte Boss_teleports [NUM_D2_BOSSES]           = {1,1,1,1,1,1, 1,1}; // Set byte if this boss can teleport
ubyte Boss_spew_more [NUM_D2_BOSSES]           = {0,1,0,0,0,0, 0,0}; // If set, 50% of time, spew two bots.
ubyte Boss_spews_bots_energy [NUM_D2_BOSSES]   = {1,1,0,1,0,1, 1,1}; // Set byte if boss spews bots when hit by energy weapon.
ubyte Boss_spews_bots_matter [NUM_D2_BOSSES]   = {0,0,1,1,1,1, 0,1}; // Set byte if boss spews bots when hit by matter weapon.
ubyte Boss_invulnerable_energy [NUM_D2_BOSSES] = {0,0,1,1,0,0, 0,0}; // Set byte if boss is invulnerable to energy weapons.
ubyte Boss_invulnerable_matter [NUM_D2_BOSSES] = {0,0,0,0,1,1, 1,0}; // Set byte if boss is invulnerable to matter weapons.
ubyte Boss_invulnerable_spot [NUM_D2_BOSSES]   = {0,0,0,0,0,1, 0,1}; // Set byte if boss is invulnerable in all but a certain spot.  (Dot product fVec|vec_to_collision < BOSS_INVULNERABLE_DOT)

#else

tBossProps bossProps [2][NUM_D2_BOSSES] = {
	{
	{1,0,1,0,0,0,0,0},
	{1,1,1,0,0,0,0,0},
	{1,0,1,1,0,1,0,0},
	{1,0,1,1,0,1,0,0},
	{1,0,0,1,0,0,1,0},
	{1,0,1,1,0,0,1,1},
	{1,0,1,0,0,0,1,0},
	{1,0,1,1,0,0,0,1}
	},
	{
	{1,0,1,0,0,0,0,0},
	{1,0,0,0,0,0,0,0},
	{1,1,1,1,1,1,0,0},
	{1,0,1,1,0,1,0,0},
	{1,0,0,1,0,0,1,0},
	{1,0,1,1,0,0,1,1},
	{1,0,1,0,0,0,1,0},
	{1,0,1,1,0,0,0,1}
	}
};

#endif

// -- sbyte Super_boss_gate_list [MAX_GATE_INDEX] = {0, 1, 8, 9, 10, 11, 12, 15, 16, 18, 19, 20, 22, 0, 8, 11, 19, 20, 8, 20, 8};

// These globals are set by a call to FindVectorIntersection, which is a slow routine,
// so we don't want to call it again (for this tObject) unless we have to.

#ifndef NDEBUG
// Index into this array with ailp->mode
char *mode_text [18] = {
	"STILL",
	"WANDER",
	"FOL_PATH",
	"CHASE_OBJ",
	"RUN_FROM",
	"BEHIND",
	"FOL_PATH2",
	"OPEN_DOOR",
	"GOTO_PLR",
	"GOTO_OBJ",
	"SN_ATT",
	"SN_FIRE",
	"SN_RETR",
	"SN_RTBK",
	"SN_WAIT",
	"TH_ATTACK",
	"TH_RETREAT",
	"TH_WAIT",
};

//	Index into this array with aip->behavior
char behavior_text [6][9] = {
	"STILL   ",
	"NORMAL  ",
	"HIDE    ",
	"RUN_FROM",
	"FOLPATH ",
	"STATION "
};

// Index into this array with aip->GOAL_STATE or aip->CURRENT_STATE
char state_text [8][5] = {
	"NONE",
	"REST",
	"SRCH",
	"LOCK",
	"FLIN",
	"FIRE",
	"RECO",
	"ERR_",
};


#endif

// Current state indicates where the robot current is, or has just done.
// Transition table between states for an AI tObject.
// First dimension is tTrigger event.
// Second dimension is current state.
// Third dimension is goal state.
// Result is new goal state.
// ERR_ means something impossible has happened.
sbyte Ai_transition_table [AI_MAX_EVENT][AI_MAX_STATE][AI_MAX_STATE] = {
	{
		// Event = AIE_FIRE, a nearby tObject fired
		// none     rest      srch      lock      flin      fire      reco        // CURRENT is rows, GOAL is columns
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO }, // none
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO }, // rest
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO }, // search
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO }, // lock
		{ AIS_ERR_, AIS_REST, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FIRE, AIS_RECO }, // flinch
		{ AIS_ERR_, AIS_FIRE, AIS_FIRE, AIS_FIRE, AIS_FLIN, AIS_FIRE, AIS_RECO }, // fire
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_FIRE }  // recoil
	},

	// Event = AIE_HITT, a nearby tObject was hit (or a wall was hit)
	{
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FLIN},
		{ AIS_ERR_, AIS_REST, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_FIRE}
	},

	// Event = AIE_COLL, tPlayer collided with robot
	{
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_LOCK, AIS_FLIN, AIS_FLIN},
		{ AIS_ERR_, AIS_REST, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_FIRE}
	},

	// Event = AIE_HURT, tPlayer hurt robot (by firing at and hitting it)
	// Note, this doesn't necessarily mean the robot JUST got hit, only that that is the most recent thing that happened.
	{
		{ AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN},
		{ AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN},
		{ AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN},
		{ AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN},
		{ AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN},
		{ AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN},
		{ AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN}
	}
};


// ----------------------------------------------------------------------------
void InitAIFrame (void)
{
	int ab_state;

gameData.ai.nDistToLastPlayerPosFiredAt = 
	VmVecDistQuick (&Last_fired_upon_player_pos, &gameData.ai.vBelievedPlayerPos);
ab_state = xAfterburnerCharge && Controls.afterburner_state && 
			  (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_AFTERBURNER);
if (! (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED) || 
	 (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_HEADLIGHT_ON) || ab_state)
	AIDoCloakStuff ();
}

// ----------------------------------------------------------------------------
// Return firing status.
// If ready to fire a weapon, return true, else return false.
// Ready to fire a weapon if nextPrimaryFire <= 0 or nextSecondaryFire <= 0.
int ReadyToFire (tRobotInfo *robotP, tAILocal *ailp)
{
return (ailp->nextPrimaryFire <= 0) || ((robotP->nSecWeaponType != -1) && (ailp->nextSecondaryFire <= 0));
}

// ----------------------------------------------------------------------------
// Make a robot near the tPlayer snipe.
#define	MNRS_SEG_MAX	70
void make_nearbyRobot_snipe (void)
{
	int bfs_length, i;
	short bfs_list [MNRS_SEG_MAX];

	CreateBfsList (gameData.objs.console->nSegment, bfs_list, &bfs_length, MNRS_SEG_MAX);

	for (i=0; i<bfs_length; i++) {
		int nObject = gameData.segs.segments [bfs_list [i]].objects;

		while (nObject != -1) {
			tObject *objP = gameData.objs.objects + nObject;
			tRobotInfo *robotP = gameData.bots.pInfo + objP->id;

			if ((objP->nType == OBJ_ROBOT) && (objP->id != ROBOT_BRAIN)) {
				if ((objP->cType.aiInfo.behavior != AIB_SNIPE) && 
					 (objP->cType.aiInfo.behavior != AIB_RUN_FROM) && 
					 !gameData.bots.pInfo [objP->id].bossFlag && 
					 !robotP->companion) {
					objP->cType.aiInfo.behavior = AIB_SNIPE;
					gameData.ai.localInfo [nObject].mode = AIM_SNIPE_ATTACK;
#if TRACE	
					con_printf (CON_DEBUG, "Making robot #%i go into snipe mode!\n", nObject);
#endif
					return;
				}
			}
			nObject = objP->next;
		}
	}
#if TRACE	
	con_printf (CON_DEBUG, "Couldn't find a robot to make snipe!\n");
#endif
}

int nAiLastMissileCamera;

//	-------------------------------------------------------------------------------------------------

void DoSnipeFrame (tObject *objP, fix xDistToPlayer, int player_visibility, vmsVector *vec_to_player)
{
	int			nObject = OBJ_IDX (objP);
	tAILocal		*ailp = &gameData.ai.localInfo [nObject];
	fix			connectedDistance;

	if (xDistToPlayer > F1_0*500)
		return;

	switch (ailp->mode) {
		case AIM_SNIPE_WAIT:
			if ((xDistToPlayer > F1_0*50) && (ailp->nextActionTime > 0))
				return;

			ailp->nextActionTime = SNIPE_WAIT_TIME;

			connectedDistance = FindConnectedDistance (&objP->position.vPos, objP->nSegment, &gameData.ai.vBelievedPlayerPos, 
																	 gameData.ai.nBelievedPlayerSeg, 30, WID_FLY_FLAG);
			if (connectedDistance < F1_0*500) {
				CreatePathToPlayer (objP, 30, 1);
				ailp->mode = AIM_SNIPE_ATTACK;
				ailp->nextActionTime = SNIPE_ATTACK_TIME;	//	have up to 10 seconds to find player.
			}
			break;

		case AIM_SNIPE_RETREAT:
		case AIM_SNIPE_RETREAT_BACKWARDS:
			if (ailp->nextActionTime < 0) {
				ailp->mode = AIM_SNIPE_WAIT;
				ailp->nextActionTime = SNIPE_WAIT_TIME;
			} else if ((player_visibility == 0) || (ailp->nextActionTime > SNIPE_ABORT_RETREAT_TIME)) {
				AIFollowPath (objP, player_visibility, player_visibility, vec_to_player);
				ailp->mode = AIM_SNIPE_RETREAT_BACKWARDS;
			} else {
				ailp->mode = AIM_SNIPE_FIRE;
				ailp->nextActionTime = SNIPE_FIRE_TIME/2;
			}
			break;

		case AIM_SNIPE_ATTACK:
			if (ailp->nextActionTime < 0) {
				ailp->mode = AIM_SNIPE_RETREAT;
				ailp->nextActionTime = SNIPE_WAIT_TIME;
			} else {
				AIFollowPath (objP, player_visibility, player_visibility, vec_to_player);
				if (player_visibility) {
					ailp->mode = AIM_SNIPE_FIRE;
					ailp->nextActionTime = SNIPE_FIRE_TIME;
				} else
					ailp->mode = AIM_SNIPE_ATTACK;
			}
			break;

		case AIM_SNIPE_FIRE:
			if (ailp->nextActionTime < 0) {
				tAIStatic	*aip = &objP->cType.aiInfo;
				CreateNSegmentPath (objP, 10 + d_rand ()/2048, gameData.objs.console->nSegment);
				aip->nPathLength = SmoothPath (objP, &gameData.ai.pointSegs [aip->nHideIndex], aip->nPathLength);
				if (d_rand () < 8192)
					ailp->mode = AIM_SNIPE_RETREAT_BACKWARDS;
				else
					ailp->mode = AIM_SNIPE_RETREAT;
				ailp->nextActionTime = SNIPE_RETREAT_TIME;
			} else {
			}
			break;

		default:
			Int3 ();	//	Oops, illegal mode for snipe behavior.
			ailp->mode = AIM_SNIPE_ATTACK;
			ailp->nextActionTime = F1_0;
			break;
	}
}

// --------------------------------------------------------------------------------------------------------------------

void AIIdleAnimation (tObject *objP)
{
if (gameOpts->gameplay.bIdleAnims) {
		int			h, i, j;
		tSegment		*segP = gameData.segs.segments + objP->nSegment;
		vmsVector	*vVertex, vVecToGoal, vGoal = gameData.objs.vRobotGoals [OBJ_IDX (objP)];

	for (i = 0; i < 8; i++) {
		vVertex = gameData.segs.vertices + segP->verts [i];
		if ((vGoal.p.x == vVertex->p.x) && (vGoal.p.y == vVertex->p.y) && (vGoal.p.z == vVertex->p.z))
			break;
		}
	VmVecNormalize (VmVecSub (&vVecToGoal, &vGoal, &objP->position.vPos));
	if (i == 8)
		h = 1;
	else if (AITurnTowardsVector (&vVecToGoal, objP, gameData.bots.pInfo [objP->id].turnTime [2]) < F1_0 - F1_0 / 5) {
		if (VmVecDot (&vVecToGoal, &objP->position.mOrient.fVec) > F1_0 - F1_0 / 5)
			h = rand () % 2 == 0;
		else
			h = 0;
		}
	else if (MoveTowardsPoint (objP, &vGoal, objP->size * 3 / 2))
		h = rand () % 8 == 0;
	else
		h = 1;
	if (h && (rand () % 25 == 0)) {
		j = rand () % 8;
		if ((j == i) || (rand () % 3 == 0))
			COMPUTE_SEGMENT_CENTER_I (&vGoal, objP->nSegment);
		else
			vGoal = gameData.segs.vertices [segP->verts [j]];
		gameData.objs.vRobotGoals [OBJ_IDX (objP)] = vGoal;
		DoSillyAnimation (objP);
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------

void DoAIFrame (tObject *objP)
{
	short       nObject = OBJ_IDX (objP);
	tAIStatic   *aip = &objP->cType.aiInfo;
	tAILocal    *ailp = &gameData.ai.localInfo [nObject];
	fix         xDistToPlayer;
	vmsVector	vec_to_player;
	fix         dot;
	tRobotInfo  *robotP;
	int         player_visibility=-1;
	int         nObjRef;
	int         bObjAnimates;
	int         nNewGoalState;
	int         nVisAndVecComputed = 0;
	int         nPrevVisibility;
	vmsVector	vGunPoint;
	vmsVector	vVisPos;

	ailp->nextActionTime -= gameData.time.xFrame;

	if (aip->SKIP_AI_COUNT) {
		aip->SKIP_AI_COUNT--;
		if (objP->mType.physInfo.flags & PF_USES_THRUST) {
			objP->mType.physInfo.rotThrust.p.x = (objP->mType.physInfo.rotThrust.p.x * 15)/16;
			objP->mType.physInfo.rotThrust.p.y = (objP->mType.physInfo.rotThrust.p.y * 15)/16;
			objP->mType.physInfo.rotThrust.p.z = (objP->mType.physInfo.rotThrust.p.z * 15)/16;
			if (!aip->SKIP_AI_COUNT)
				objP->mType.physInfo.flags &= ~PF_USES_THRUST;
		}
		return;
	}

	robotP = gameData.bots.pInfo + objP->id;
	Assert (robotP->always_0xabcd == 0xabcd);

	if (DoAnyRobotDyingFrame (objP))
		return;

	// Kind of a hack.  If a robot is flinching, but it is time for it to fire, unflinch it.
	// Else, you can turn a big nasty robot into a wimp by firing flares at it.
	// This also allows the tPlayer to see the cool flinch effect for mechs without unbalancing the game.
	if ((aip->GOAL_STATE == AIS_FLIN) && ReadyToFire (robotP, ailp)) {
		aip->GOAL_STATE = AIS_FIRE;
	}

#ifndef NDEBUG
	if ((aip->behavior == AIB_RUN_FROM) && (ailp->mode != AIM_RUN_FROM_OBJECT))
		Int3 (); // This is peculiar.  Behavior is run from, but mode is not.  Contact Mike.

	mprintf_animation_info (objP);

	if (!bDoAIFlag)
		return;

	if (nBreakOnObject != -1)
		if ((OBJ_IDX (objP)) == nBreakOnObject)
			Int3 (); // Contact Mike: This is a debug break
#endif
#if TRACE	
	//con_printf (CON_DEBUG, "Object %i: behavior = %02x, mode = %i, awareness = %i, time = %7.3f\n", OBJ_IDX (objP), aip->behavior, ailp->mode, ailp->playerAwarenessType, f2fl (ailp->playerAwarenessTime));
	//con_printf (CON_DEBUG, "Object %i: behavior = %02x, mode = %i, awareness = %i, cur=%i, goal=%i\n", OBJ_IDX (objP), aip->behavior, ailp->mode, ailp->playerAwarenessType, aip->CURRENT_STATE, aip->GOAL_STATE);
#endif
	//Assert ((aip->behavior >= MIN_BEHAVIOR) && (aip->behavior <= MAX_BEHAVIOR);
	if (! ((aip->behavior >= MIN_BEHAVIOR) && (aip->behavior <= MAX_BEHAVIOR))) {
#if TRACE	
		//con_printf (CON_DEBUG, "Object %i behavior is %i, setting to AIB_NORMAL, fix in editor!\n", nObject, aip->behavior);
#endif
		aip->behavior = AIB_NORMAL;
	}

	Assert (objP->nSegment != -1);
	//Assert (objP->id < gameData.bots.nTypes [gameStates.app.bD1Data]);

	nObjRef = nObject ^ gameData.app.nFrameCount;

	if (ailp->nextPrimaryFire > -F1_0*8)
		ailp->nextPrimaryFire -= gameData.time.xFrame;

	if (robotP->nSecWeaponType != -1) {
		if (ailp->nextSecondaryFire > -F1_0*8)
			ailp->nextSecondaryFire -= gameData.time.xFrame;
	} else
		ailp->nextSecondaryFire = F1_0*8;

	if (ailp->timeSinceProcessed < F1_0*256)
		ailp->timeSinceProcessed += gameData.time.xFrame;

	nPrevVisibility = ailp->nPrevVisibility;    //  Must get this before we toast the master copy!

	// -- (No robots have this behavior...)
	// -- // Deal with cloaking for robots which are cloaked except just before firing.
	// -- if (robotP->cloakType == RI_CLOAKED_EXCEPT_FIRING)
	// -- 	if (ailp->nextPrimaryFire < F1_0/2)
	// -- 		aip->CLOAKED = 1;
	// -- 	else
	// -- 		aip->CLOAKED = 0;

	// If only awake because of a camera, make that the believed tPlayer position.
	if ((aip->SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE) && (nAiLastMissileCamera != -1))
		gameData.ai.vBelievedPlayerPos = gameData.objs.objects [nAiLastMissileCamera].position.vPos;
	else {
		if (gameStates.app.cheats.bRobotsKillRobots) {
			vVisPos = objP->position.vPos;
			ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);
			if (player_visibility) {
				int ii, min_obj = -1;
				fix minDist = F1_0*200, curDist;

				for (ii=0; ii<=gameData.objs.nLastObject; ii++)
					if ((gameData.objs.objects [ii].nType == OBJ_ROBOT) && (ii != nObject)) {
						curDist = VmVecDistQuick (&objP->position.vPos, &gameData.objs.objects [ii].position.vPos);

						if (curDist < F1_0*100)
							if (ObjectToObjectVisibility (objP, &gameData.objs.objects [ii], FQ_TRANSWALL))
								if (curDist < minDist) {
									min_obj = ii;
									minDist = curDist;
								}
					}
				if (min_obj != -1) {
					gameData.ai.vBelievedPlayerPos = gameData.objs.objects [min_obj].position.vPos;
					gameData.ai.nBelievedPlayerSeg = gameData.objs.objects [min_obj].nSegment;
					VmVecNormalizedDirQuick (&vec_to_player, &gameData.ai.vBelievedPlayerPos, &objP->position.vPos);
				} else
					goto _exit_cheat;
			} else
				goto _exit_cheat;
		} else {
_exit_cheat:
			nVisAndVecComputed = 0;
			if (! (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED))
				gameData.ai.vBelievedPlayerPos = gameData.objs.console->position.vPos;
			else
				gameData.ai.vBelievedPlayerPos = gameData.ai.cloakInfo [nObject & (MAX_AI_CLOAK_INFO-1)].last_position;
		}
	}
	xDistToPlayer = VmVecDistQuick (&gameData.ai.vBelievedPlayerPos, &objP->position.vPos);
	//if (robotP->companion) {
#if TRACE	
	//	con_printf (CON_DEBUG, "%3i: %3i %8.3f %8s %8s [%3i %4i]\n", nObject, objP->nSegment, f2fl (xDistToPlayer), mode_text [ailp->mode], behavior_text [aip->behavior-0x80], aip->nHideIndex, aip->nPathLength);
#endif
	//}
	// If this robot can fire, compute visibility from gun position.
	// Don't want to compute visibility twice, as it is expensive.  (So is call to CalcGunPoint).
	if ((nPrevVisibility || ! (nObjRef & 3)) && ReadyToFire (robotP, ailp) && (xDistToPlayer < F1_0*200) && (robotP->nGuns) && ! (robotP->attackType)) {
		// Since we passed ReadyToFire (), either nextPrimaryFire or nextSecondaryFire <= 0.  CalcGunPoint from relevant one.
		// If both are <= 0, we will deal with the mess in ai_do_actual_firing_stuff
		if (ailp->nextPrimaryFire <= 0)
			CalcGunPoint (&vGunPoint, objP, aip->CURRENT_GUN);
		else
			CalcGunPoint (&vGunPoint, objP, 0);
		vVisPos = vGunPoint;
	} else {
		vVisPos = objP->position.vPos;
		VmVecZero (&vGunPoint);
#if TRACE	
		//con_printf (CON_DEBUG, "Visibility = %i, computed from center.\n", player_visibility);
#endif
	}

// MK: Debugging, July 26, 1995!
// if (nObject == 1)
// {
// 	ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);
#if TRACE	
// 	con_printf (CON_DEBUG, "Frame %i: dist=%7.3f, vecdot = %7.3f, mode=%i\n", gameData.app.nFrameCount, f2fl (xDistToPlayer), f2fl (VmVecDot (&vec_to_player, &objP->position.mOrient.fVec)), ailp->mode);
#endif
// }
	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - 
	// Occasionally make non-still robots make a path to the player.  Based on agitation and distance from player.
	if ((aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_RUN_FROM) && (aip->behavior != AIB_IDLING) && 
		 ! (gameData.app.nGameMode & GM_MULTI) && 
		 (robotP->companion != 1) && (robotP->thief != 1))
		if (gameData.ai.nOverallAgitation > 70) {
			if ((xDistToPlayer < F1_0*200) && (d_rand () < gameData.time.xFrame/4)) {
				if (d_rand () * (gameData.ai.nOverallAgitation - 40) > F1_0*5) {
					CreatePathToPlayer (objP, 4 + gameData.ai.nOverallAgitation/8 + gameStates.app.nDifficultyLevel, 1);
					return;
				}
			}
		}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// If retry count not 0, then add it into nConsecutiveRetries.
	// If it is 0, cut down nConsecutiveRetries.
	// This is largely a hack to speed up physics and deal with stupid
	// AI.  This is low level communication between systems of a sort
	// that should not be done.
	if ((ailp->nRetryCount) && ! (gameData.app.nGameMode & GM_MULTI)) {
		ailp->nConsecutiveRetries += ailp->nRetryCount;
		ailp->nRetryCount = 0;
		if (ailp->nConsecutiveRetries > 3) {
			switch (ailp->mode) {
				case AIM_GOTO_PLAYER:
					// -- Buddy_got_stuck = 1;
					MoveTowardsSegmentCenter (objP);
					CreatePathToPlayer (objP, 100, 1);
					// -- Buddy_got_stuck = 0;
					break;
				case AIM_GOTO_OBJECT:
					gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
					//if (objP->nSegment == gameData.objs.console->nSegment) {
					//	if (gameData.ai.pointSegs [aip->nHideIndex + aip->nCurPathIndex].nSegment == objP->nSegment)
					//		if ((aip->nCurPathIndex + aip->PATH_DIR >= 0) && (aip->nCurPathIndex + aip->PATH_DIR < aip->nPathLength-1))
					//			aip->nCurPathIndex += aip->PATH_DIR;
					//}
					break;
				case AIM_CHASE_OBJECT:
					CreatePathToPlayer (objP, 4 + gameData.ai.nOverallAgitation/8 + gameStates.app.nDifficultyLevel, 1);
					break;
				case AIM_IDLING:
					if (robotP->attackType)
						MoveTowardsSegmentCenter (objP);
					else if (! ((aip->behavior == AIB_IDLING) || (aip->behavior == AIB_STATION) || (aip->behavior == AIB_FOLLOW)))    // Behavior is still, so don't follow path.
						AttemptToResumePath (objP);
					break;
				case AIM_FOLLOW_PATH:
					if (gameData.app.nGameMode & GM_MULTI) {
						ailp->mode = AIM_IDLING;
					} else
						AttemptToResumePath (objP);
					break;
				case AIM_RUN_FROM_OBJECT:
					MoveTowardsSegmentCenter (objP);
					objP->mType.physInfo.velocity.p.x = 0;
					objP->mType.physInfo.velocity.p.y = 0;
					objP->mType.physInfo.velocity.p.z = 0;
					CreateNSegmentPath (objP, 5, -1);
					ailp->mode = AIM_RUN_FROM_OBJECT;
					break;
				case AIM_BEHIND:
#if TRACE	
					con_printf (CON_DEBUG, "Hiding robot (%i) collided much.\n", OBJ_IDX (objP));
#endif
					MoveTowardsSegmentCenter (objP);
					objP->mType.physInfo.velocity.p.x = 0;
					objP->mType.physInfo.velocity.p.y = 0;
					objP->mType.physInfo.velocity.p.z = 0;
					break;
				case AIM_OPEN_DOOR:
					CreateNSegmentPathToDoor (objP, 5, -1);
					break;
				#ifndef NDEBUG
				case AIM_FOLLOW_PATH_2:
					Int3 (); // Should never happen!
					break;
				#endif
			}
			ailp->nConsecutiveRetries = 0;
		}
	} else
		ailp->nConsecutiveRetries /= 2;

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// If in materialization center, exit
	if (! (gameData.app.nGameMode & GM_MULTI) && (gameData.segs.segment2s [objP->nSegment].special == SEGMENT_IS_ROBOTMAKER)) {
		if (gameData.matCens.fuelCenters [gameData.segs.segment2s [objP->nSegment].value].bEnabled) {
			AIFollowPath (objP, 1, 1, NULL);    // 1 = tPlayer is visible, which might be a lie, but it works.
			return;
		}
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// Decrease tPlayer awareness due to the passage of time.
	if (ailp->playerAwarenessType) {
		if (ailp->playerAwarenessTime > 0) {
			ailp->playerAwarenessTime -= gameData.time.xFrame;
			if (ailp->playerAwarenessTime <= 0) {
				ailp->playerAwarenessTime = F1_0*2;   //new: 11/05/94
				ailp->playerAwarenessType--;          //new: 11/05/94
			}
		} else {
			ailp->playerAwarenessType--;
			ailp->playerAwarenessTime = F1_0*2;
			//aip->GOAL_STATE = AIS_REST;
		}
	} else
		aip->GOAL_STATE = AIS_REST;                     //new: 12/13/94


	if (gameStates.app.bPlayerIsDead && (ailp->playerAwarenessType == 0))
		if ((xDistToPlayer < F1_0*200) && (d_rand () < gameData.time.xFrame/8)) {
			if ((aip->behavior != AIB_IDLING) && (aip->behavior != AIB_RUN_FROM)) {
				if (!AIMultiplayerAwareness (objP, 30))
					return;
				ai_multi_sendRobot_position (nObject, -1);
				if (! ((ailp->mode == AIM_FOLLOW_PATH) && (aip->nCurPathIndex < aip->nPathLength-1)))
					if ((aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_RUN_FROM)) {
						if (xDistToPlayer < F1_0*30)
							CreateNSegmentPath (objP, 5, 1);
						else
							CreatePathToPlayer (objP, 20, 1);
					}
			}
		}
	// Make sure that if this guy got hit or bumped, then he's chasing player.
	if ((ailp->playerAwarenessType == PA_WEAPON_ROBOT_COLLISION) || 
		 (ailp->playerAwarenessType >= PA_PLAYER_COLLISION)) {
		ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);
		if (player_visibility == 1) // Only increase visibility if unobstructed, else claw guys attack through doors.
			player_visibility = 2;
	} else if (((nObjRef&3) == 0) && !nPrevVisibility && (xDistToPlayer < F1_0*100)) {
		fix sval, rval;

		rval = d_rand ();
		sval = (xDistToPlayer * (gameStates.app.nDifficultyLevel+1))/64;

		if ((FixMul (rval, sval) < gameData.time.xFrame) || (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_HEADLIGHT_ON)) {
			ailp->playerAwarenessType = PA_PLAYER_COLLISION;
			ailp->playerAwarenessTime = F1_0*3;
			ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);
			if (player_visibility == 1) {
				player_visibility = 2;
			}
		}
	}


	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	if ((aip->GOAL_STATE == AIS_FLIN) && (aip->CURRENT_STATE == AIS_FLIN))
		aip->GOAL_STATE = AIS_LOCK;

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// Note: Should only do these two function calls for gameData.objs.objects which animate
	if (gameData.ai.bEnableAnimation && (xDistToPlayer < F1_0*100)) { // && ! (gameData.app.nGameMode & GM_MULTI)) {
		bObjAnimates = DoSillyAnimation (objP);
		if (bObjAnimates)
			AIFrameAnimation (objP);
	} else {
		// If Object is supposed to animate, but we don't let it animate due to distance, then
		// we must change its state, else it will never update.
		aip->CURRENT_STATE = aip->GOAL_STATE;
		bObjAnimates = 0;        // If we're not doing the animation, then should pretend it doesn't animate.
	}

	switch (gameData.bots.pInfo [objP->id].bossFlag) {
	case 0:
		break;

	case 1:
	case 2:
//		break;

	default:
		{
			int	pv;
			fix	dtp = xDistToPlayer/4;

			if (aip->GOAL_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_FIRE;
			if (aip->CURRENT_STATE == AIS_FLIN)
				aip->CURRENT_STATE = AIS_FIRE;

			ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);

			pv = player_visibility;

			// If tPlayer cloaked, visibility is screwed up and superboss will gate in robots when not supposed to.
			if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED) {
				pv = 0;
				dtp = VmVecDistQuick (&gameData.objs.console->position.vPos, &objP->position.vPos)/4;
			}

			DoBossStuff (objP, pv);
		}
		break;
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// Time-slice, don't process all the time, purely an efficiency hack.
	// Guys whose behavior is station and are not at their hide tSegment get processed anyway.
	if (! ((aip->behavior == AIB_SNIPE) && (ailp->mode != AIM_SNIPE_WAIT)) && !robotP->companion && !robotP->thief && (ailp->playerAwarenessType < PA_WEAPON_ROBOT_COLLISION-1)) { // If robot got hit, he gets to attack tPlayer always!
#ifndef NDEBUG
		if (nBreakOnObject != nObject) {    // don't time slice if we're interested in this tObject.
#endif
			if ((aip->behavior == AIB_STATION) && (ailp->mode == AIM_FOLLOW_PATH) && (aip->nHideSegment != objP->nSegment)) {
				if (xDistToPlayer > F1_0*250) {  // station guys not at home always processed until 250 units away.
					AIIdleAnimation (objP);
					return;
					}
				}
			else if ((!ailp->nPrevVisibility) && ((xDistToPlayer >> 7) > ailp->timeSinceProcessed)) {  // 128 units away (6.4 segments) processed after 1 second.
				AIIdleAnimation (objP);
				return;
			}
#ifndef NDEBUG
		}
#endif
	}

	// Reset time since processed, but skew gameData.objs.objects so not everything
	// processed synchronously, else we get fast frames with the
	// occasional very slow frame.
	// AI_procTime = ailp->timeSinceProcessed;
	ailp->timeSinceProcessed = - ((nObject & 0x03) * gameData.time.xFrame ) / 2;

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	//	Perform special ability
	switch (objP->id) {
		case ROBOT_BRAIN:
			// Robots function nicely if behavior is gameData.matCens.fuelCenters.  This
			// means they won't move until they can see the tPlayer, at
			// which time they will start wandering about opening doors.
			if (gameData.objs.console->nSegment == objP->nSegment) {
				if (!AIMultiplayerAwareness (objP, 97))
					return;
				ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);
				move_away_from_player (objP, &vec_to_player, 0);
				ai_multi_sendRobot_position (nObject, -1);
			} else if (ailp->mode != AIM_IDLING) {
				int r;

				r = OpenableDoorsInSegment (objP->nSegment);
				if (r != -1) {
					ailp->mode = AIM_OPEN_DOOR;
					aip->GOALSIDE = r;
				} else if (ailp->mode != AIM_FOLLOW_PATH) {
					if (!AIMultiplayerAwareness (objP, 50))
						return;
					CreateNSegmentPathToDoor (objP, 8+gameStates.app.nDifficultyLevel, -1);     // third parameter is avoid_seg, -1 means avoid nothing.
					ai_multi_sendRobot_position (nObject, -1);
				}

				if (ailp->nextActionTime < 0) {
					ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);
					if (player_visibility) {
						make_nearbyRobot_snipe ();
						ailp->nextActionTime = (NDL - gameStates.app.nDifficultyLevel) * 2*F1_0;
					}
				}
			} else {
				ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);
				if (player_visibility) {
					if (!AIMultiplayerAwareness (objP, 50))
						return;
					CreateNSegmentPathToDoor (objP, 8+gameStates.app.nDifficultyLevel, -1);     // third parameter is avoid_seg, -1 means avoid nothing.
					ai_multi_sendRobot_position (nObject, -1);
				}
			}
			break;
		default:
			break;
	}

	if (aip->behavior == AIB_SNIPE) {
		if ((gameData.app.nGameMode & GM_MULTI) && !robotP->thief) {
			aip->behavior = AIB_NORMAL;
			ailp->mode = AIM_CHASE_OBJECT;
			return;
		}

		if (! (nObjRef & 3) || nPrevVisibility) {
			ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);

			// If this sniper is in still mode, if he was hit or can see tPlayer, switch to snipe mode.
			if (ailp->mode == AIM_IDLING)
				if (player_visibility || (ailp->playerAwarenessType == PA_WEAPON_ROBOT_COLLISION))
					ailp->mode = AIM_SNIPE_ATTACK;

			if (!robotP->thief && (ailp->mode != AIM_IDLING))
				DoSnipeFrame (objP, xDistToPlayer, player_visibility, &vec_to_player);
		} else if (!robotP->thief && !robotP->companion)
			return;
	}

	// More special ability stuff, but based on a property of a robot, not its ID.
	if (robotP->companion) {
		ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);
		DoEscortFrame (objP, xDistToPlayer, player_visibility);

		if (objP->cType.aiInfo.nDangerLaser != -1) {
			tObject *dObjP = &gameData.objs.objects [objP->cType.aiInfo.nDangerLaser];

			if ((dObjP->nType == OBJ_WEAPON) && (dObjP->nSignature == objP->cType.aiInfo.nDangerLaserSig)) {
				fix circleDistance;
				circleDistance = robotP->circleDistance [gameStates.app.nDifficultyLevel] + gameData.objs.console->size;
				AIMoveRelativeToPlayer (objP, ailp, xDistToPlayer, &vec_to_player, circleDistance, 1, player_visibility);
			}
		}

		if (ReadyToFire (robotP, ailp)) {
			int do_stuff = 0;
			
			if (OpenableDoorsInSegment (objP->nSegment) != -1)
				do_stuff = 1;
			else if (OpenableDoorsInSegment ((short) gameData.ai.pointSegs [aip->nHideIndex + aip->nCurPathIndex + aip->PATH_DIR].nSegment) != -1)
				do_stuff = 1;
			else if (OpenableDoorsInSegment ((short) gameData.ai.pointSegs [aip->nHideIndex + aip->nCurPathIndex + 2*aip->PATH_DIR].nSegment) != -1)
				do_stuff = 1;
			else if ((ailp->mode == AIM_GOTO_PLAYER) && (xDistToPlayer < 3*MIN_ESCORT_DISTANCE/2) ) {
				do_stuff = 1;
			} else
				; 

			if (do_stuff && (VmVecDot (&gameData.objs.console->position.mOrient.fVec, &vec_to_player) < F1_0/3)) {
				CreateNewLaserEasy ( &objP->position.mOrient.fVec, &objP->position.vPos, OBJ_IDX (objP), FLARE_ID, 1);
				ailp->nextPrimaryFire = F1_0/2;
				if (!gameData.escort.bMayTalk) // If buddy not talking, make him fire flares less often.
					ailp->nextPrimaryFire += d_rand ()*4;
			}

		}
	}

	if (robotP->thief) {
		ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);
		DoThiefFrame (objP, xDistToPlayer, player_visibility, &vec_to_player);

		if (ReadyToFire (robotP, ailp)) {
			int do_stuff = 0;
			if (OpenableDoorsInSegment (objP->nSegment) != -1)
				do_stuff = 1;
			else if (OpenableDoorsInSegment ((short) gameData.ai.pointSegs [aip->nHideIndex + aip->nCurPathIndex + aip->PATH_DIR].nSegment) != -1)
				do_stuff = 1;
			else if (OpenableDoorsInSegment ((short) gameData.ai.pointSegs [aip->nHideIndex + aip->nCurPathIndex + 2*aip->PATH_DIR].nSegment) != -1)
				do_stuff = 1;

			if (do_stuff) {
				// @mk, 05/08/95: Firing flare from center of tObject, this is dumb...
				CreateNewLaserEasy ( &objP->position.mOrient.fVec, &objP->position.vPos, OBJ_IDX (objP), FLARE_ID, 1);
				ailp->nextPrimaryFire = F1_0/2;
				if (gameData.thief.nStolenItem == 0)     // If never stolen an item, fire flares less often (bad: gameData.thief.nStolenItem wraps, but big deal)
					ailp->nextPrimaryFire += d_rand ()*4;
			}
		}
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	switch (ailp->mode) {
		case AIM_CHASE_OBJECT: {        // chasing tPlayer, sort of, chase if far, back off if close, circle in between
			fix circleDistance;

			circleDistance = robotP->circleDistance [gameStates.app.nDifficultyLevel] + gameData.objs.console->size;
			// Green guy doesn't get his circle distance boosted, else he might never attack.
			if (robotP->attackType != 1)
				circleDistance += (nObject&0xf) * F1_0/2;

			ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);

			// @mk, 12/27/94, structure here was strange.  Would do both clauses of what are now this if/then/else.  Used to be if/then, if/then.
			if ((player_visibility < 2) && (nPrevVisibility == 2)) { // this is redundant: mk, 01/15/95: && (ailp->mode == AIM_CHASE_OBJECT)) {
				if (!AIMultiplayerAwareness (objP, 53)) {
					if (maybe_ai_do_actual_firing_stuff (objP, aip))
						ai_do_actual_firing_stuff (objP, aip, ailp, robotP, &vec_to_player, xDistToPlayer, &vGunPoint, player_visibility, bObjAnimates, aip->CURRENT_GUN);
					return;
				}
				CreatePathToPlayer (objP, 8, 1);
				ai_multi_sendRobot_position (nObject, -1);
			} else if ((player_visibility == 0) && (xDistToPlayer > F1_0*80) && (! (gameData.app.nGameMode & GM_MULTI))) {
				// If pretty far from the tPlayer, tPlayer cannot be seen
				// (obstructed) and in chase mode, switch to follow path mode.
				// This has one desirable benefit of avoiding physics retries.
				if (aip->behavior == AIB_STATION) {
					ailp->nGoalSegment = aip->nHideSegment;
					CreatePathToStation (objP, 15);
				} // -- this looks like a dumb thing to do...robots following paths far away from you! else CreateNSegmentPath (objP, 5, -1);
				break;
			}

			if ((aip->CURRENT_STATE == AIS_REST) && (aip->GOAL_STATE == AIS_REST)) {
				if (player_visibility &&
					 (d_rand () < gameData.time.xFrame*player_visibility) &&
					 (xDistToPlayer/256 < d_rand ()*player_visibility)) {
					aip->GOAL_STATE = AIS_SRCH;
					aip->CURRENT_STATE = AIS_SRCH;
					}
				else
					AIIdleAnimation (objP);
				}

			if (gameData.time.xGame - ailp->timePlayerSeen > CHASE_TIME_LENGTH) {

				if (gameData.app.nGameMode & GM_MULTI)
					if (!player_visibility && (xDistToPlayer > F1_0*70)) {
						ailp->mode = AIM_IDLING;
						return;
					}

				if (!AIMultiplayerAwareness (objP, 64)) {
					if (maybe_ai_do_actual_firing_stuff (objP, aip))
						ai_do_actual_firing_stuff (objP, aip, ailp, robotP, &vec_to_player, xDistToPlayer, &vGunPoint, player_visibility, bObjAnimates, aip->CURRENT_GUN);
					return;
				}
				// -- bad idea, robots charge tPlayer they've never seen! -- CreatePathToPlayer (objP, 10, 1);
				// -- bad idea, robots charge tPlayer they've never seen! -- ai_multi_sendRobot_position (nObject, -1);
			} else if ((aip->CURRENT_STATE != AIS_REST) && (aip->GOAL_STATE != AIS_REST)) {
				if (!AIMultiplayerAwareness (objP, 70)) {
					if (maybe_ai_do_actual_firing_stuff (objP, aip))
						ai_do_actual_firing_stuff (objP, aip, ailp, robotP, &vec_to_player, xDistToPlayer, &vGunPoint, player_visibility, bObjAnimates, aip->CURRENT_GUN);
					return;
				}
				AIMoveRelativeToPlayer (objP, ailp, xDistToPlayer, &vec_to_player, circleDistance, 0, player_visibility);

				if ((nObjRef & 1) && ((aip->GOAL_STATE == AIS_SRCH) || (aip->GOAL_STATE == AIS_LOCK))) {
					if (player_visibility) // == 2)
						AITurnTowardsVector (&vec_to_player, objP, robotP->turnTime [gameStates.app.nDifficultyLevel]);
				}

				if (gameData.ai.bEvaded) {
					ai_multi_sendRobot_position (nObject, 1);
					gameData.ai.bEvaded = 0;
				} else
					ai_multi_sendRobot_position (nObject, -1);

				do_firing_stuff (objP, player_visibility, &vec_to_player);
			}
			break;
		}

		case AIM_RUN_FROM_OBJECT:
			ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);

			if (player_visibility) {
				if (ailp->playerAwarenessType == 0)
					ailp->playerAwarenessType = PA_WEAPON_ROBOT_COLLISION;

			}

			// If in multiplayer, only do if tPlayer visible.  If not multiplayer, do always.
			if (! (gameData.app.nGameMode & GM_MULTI) || player_visibility)
				if (AIMultiplayerAwareness (objP, 75)) {
					AIFollowPath (objP, player_visibility, nPrevVisibility, &vec_to_player);
					ai_multi_sendRobot_position (nObject, -1);
				}

			if (aip->GOAL_STATE != AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;
			else if (aip->CURRENT_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;

			// Bad to let run_from robot fire at tPlayer because it
			// will cause a war in which it turns towards the tPlayer
			// to fire and then towards its goal to move.
			// do_firing_stuff (objP, player_visibility, &vec_to_player);
			// Instead, do this:
			// (Note, only drop if tPlayer is visible.  This prevents
			// the bombs from being a giveaway, and also ensures that
			// the robot is moving while it is dropping.  Also means
			// fewer will be dropped.)
			if ((ailp->nextPrimaryFire <= 0) && (player_visibility)) {
				vmsVector fire_vec, fire_pos;

				if (!AIMultiplayerAwareness (objP, 75))
					return;

				fire_vec = objP->position.mOrient.fVec;
				VmVecNegate (&fire_vec);
				VmVecAdd (&fire_pos, &objP->position.vPos, &fire_vec);

				if (aip->SUB_FLAGS & SUB_FLAGS_SPROX)
					CreateNewLaserEasy ( &fire_vec, &fire_pos, OBJ_IDX (objP), ROBOT_SMARTMINE_ID, 1);
				else
					CreateNewLaserEasy ( &fire_vec, &fire_pos, OBJ_IDX (objP), PROXMINE_ID, 1);

				ailp->nextPrimaryFire = (F1_0/2)* (NDL+5 - gameStates.app.nDifficultyLevel);      // Drop a proximity bomb every 5 seconds.

				if (gameData.app.nGameMode & GM_MULTI) {
					ai_multi_sendRobot_position (OBJ_IDX (objP), -1);
					if (aip->SUB_FLAGS & SUB_FLAGS_SPROX)
						MultiSendRobotFire (OBJ_IDX (objP), -2, &fire_vec);
					else
						MultiSendRobotFire (OBJ_IDX (objP), -1, &fire_vec);
				}
 			}
			break;

		case AIM_GOTO_PLAYER:
		case AIM_GOTO_OBJECT:
			AIFollowPath (objP, 2, nPrevVisibility, &vec_to_player);    // Follows path as if tPlayer can see robot.
			ai_multi_sendRobot_position (nObject, -1);
			break;

		case AIM_FOLLOW_PATH: {
			int angerLevel = 65;

			if (aip->behavior == AIB_STATION)
				if (gameData.ai.pointSegs [aip->nHideIndex + aip->nPathLength - 1].nSegment == aip->nHideSegment) {
					angerLevel = 64;
				}

			ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);

			if (gameData.app.nGameMode & (GM_MODEM | GM_SERIAL))
				if (!player_visibility && (xDistToPlayer > F1_0*70)) {
					ailp->mode = AIM_IDLING;
					return;
				}

			if (!AIMultiplayerAwareness (objP, angerLevel)) {
				if (maybe_ai_do_actual_firing_stuff (objP, aip)) {
					ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);
					ai_do_actual_firing_stuff (objP, aip, ailp, robotP, &vec_to_player, xDistToPlayer, &vGunPoint, player_visibility, bObjAnimates, aip->CURRENT_GUN);
				}
				return;
			}

			AIFollowPath (objP, player_visibility, nPrevVisibility, &vec_to_player);

			if (aip->GOAL_STATE != AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;
			else if (aip->CURRENT_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;

			if (aip->behavior != AIB_RUN_FROM)
				do_firing_stuff (objP, player_visibility, &vec_to_player);

			if ((player_visibility == 2) && (aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_FOLLOW) && (aip->behavior != AIB_RUN_FROM) && (objP->id != ROBOT_BRAIN) && (robotP->companion != 1) && (robotP->thief != 1)) {
				if (robotP->attackType == 0)
					ailp->mode = AIM_CHASE_OBJECT;
				// This should not just be distance based, but also time-since-tPlayer-seen based.
			} else if ((xDistToPlayer > F1_0* (20* (2*gameStates.app.nDifficultyLevel + robotP->pursuit)))
						&& (gameData.time.xGame - ailp->timePlayerSeen > (F1_0/2* (gameStates.app.nDifficultyLevel+robotP->pursuit)))
						&& (player_visibility == 0)
						&& (aip->behavior == AIB_NORMAL)
						&& (ailp->mode == AIM_FOLLOW_PATH)) {
				ailp->mode = AIM_IDLING;
				aip->nHideIndex = -1;
				aip->nPathLength = 0;
			}

			ai_multi_sendRobot_position (nObject, -1);

			break;
		}

		case AIM_BEHIND:
			if (!AIMultiplayerAwareness (objP, 71)) {
				if (maybe_ai_do_actual_firing_stuff (objP, aip)) {
					ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);
					ai_do_actual_firing_stuff (objP, aip, ailp, robotP, &vec_to_player, xDistToPlayer, &vGunPoint, player_visibility, bObjAnimates, aip->CURRENT_GUN);
				}
				return;
			}

			ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);

			if (player_visibility == 2) {
				// Get behind the player.
				// Method:
				// If vec_to_player dot player_rear_vector > 0, behind is goal.
				// Else choose goal with larger dot from left, right.
				vmsVector  goal_point, goal_vector, vec_to_goal, rand_vec;
				fix         dot;

				dot = VmVecDot (&gameData.objs.console->position.mOrient.fVec, &vec_to_player);
				if (dot > 0) {          // Remember, we're interested in the rear vector dot being < 0.
					goal_vector = gameData.objs.console->position.mOrient.fVec;
					VmVecNegate (&goal_vector);
				} else {
					fix dot;
					dot = VmVecDot (&gameData.objs.console->position.mOrient.rVec, &vec_to_player);
					goal_vector = gameData.objs.console->position.mOrient.rVec;
					if (dot > 0) {
						VmVecNegate (&goal_vector);
					} else
						;
				}

				VmVecScale (&goal_vector, 2* (gameData.objs.console->size + objP->size + (((nObject*4 + gameData.app.nFrameCount) & 63) << 12)));
				VmVecAdd (&goal_point, &gameData.objs.console->position.vPos, &goal_vector);
				MakeRandomVector (&rand_vec);
				VmVecScaleInc (&goal_point, &rand_vec, F1_0*8);
				VmVecSub (&vec_to_goal, &goal_point, &objP->position.vPos);
				VmVecNormalizeQuick (&vec_to_goal);
				move_towards_vector (objP, &vec_to_goal, 0);
				AITurnTowardsVector (&vec_to_player, objP, robotP->turnTime [gameStates.app.nDifficultyLevel]);
				ai_do_actual_firing_stuff (objP, aip, ailp, robotP, &vec_to_player, xDistToPlayer, &vGunPoint, player_visibility, bObjAnimates, aip->CURRENT_GUN);
			}

			if (aip->GOAL_STATE != AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;
			else if (aip->CURRENT_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;

			ai_multi_sendRobot_position (nObject, -1);
			break;

		case AIM_IDLING:
			if ((xDistToPlayer < F1_0*120 + gameStates.app.nDifficultyLevel*F1_0*20) ||
				 (ailp->playerAwarenessType >= PA_WEAPON_ROBOT_COLLISION-1)) {
				ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);

				// turn towards vector if visible this time or last time, or rand
				// new!
				if ((player_visibility == 2) || (nPrevVisibility == 2)) { // -- MK, 06/09/95:  || ((d_rand () > 0x4000) && ! (gameData.app.nGameMode & GM_MULTI))) {
					if (!AIMultiplayerAwareness (objP, 71)) {
						if (maybe_ai_do_actual_firing_stuff (objP, aip))
							ai_do_actual_firing_stuff (objP, aip, ailp, robotP, &vec_to_player, xDistToPlayer, &vGunPoint, player_visibility, bObjAnimates, aip->CURRENT_GUN);
						return;
					}
					AITurnTowardsVector (&vec_to_player, objP, robotP->turnTime [gameStates.app.nDifficultyLevel]);
					ai_multi_sendRobot_position (nObject, -1);
				}

				do_firing_stuff (objP, player_visibility, &vec_to_player);
				if (player_visibility == 2) {  // Changed @mk, 09/21/95: Require that they be looking to evade.  Change, MK, 01/03/95 for Multiplayer reasons.  If robots can't see you (even with eyes on back of head), then don't do evasion.
					if (robotP->attackType == 1) {
						aip->behavior = AIB_NORMAL;
						if (!AIMultiplayerAwareness (objP, 80)) {
							if (maybe_ai_do_actual_firing_stuff (objP, aip))
								ai_do_actual_firing_stuff (objP, aip, ailp, robotP, &vec_to_player, xDistToPlayer, &vGunPoint, player_visibility, bObjAnimates, aip->CURRENT_GUN);
							return;
						}
						AIMoveRelativeToPlayer (objP, ailp, xDistToPlayer, &vec_to_player, 0, 0, player_visibility);
						if (gameData.ai.bEvaded) {
							ai_multi_sendRobot_position (nObject, 1);
							gameData.ai.bEvaded = 0;
						}
						else
							ai_multi_sendRobot_position (nObject, -1);
					} else {
						// Robots in hover mode are allowed to evade at half normal speed.
						if (!AIMultiplayerAwareness (objP, 81)) {
							if (maybe_ai_do_actual_firing_stuff (objP, aip))
								ai_do_actual_firing_stuff (objP, aip, ailp, robotP, &vec_to_player, xDistToPlayer, &vGunPoint, player_visibility, bObjAnimates, aip->CURRENT_GUN);
							return;
						}
						AIMoveRelativeToPlayer (objP, ailp, xDistToPlayer, &vec_to_player, 0, 1, player_visibility);
						if (gameData.ai.bEvaded) {
							ai_multi_sendRobot_position (nObject, -1);
							gameData.ai.bEvaded = 0;
						}
						else
							ai_multi_sendRobot_position (nObject, -1);
					}
				} else if ((objP->nSegment != aip->nHideSegment) && (xDistToPlayer > F1_0*80) && (! (gameData.app.nGameMode & GM_MULTI))) {
					// If pretty far from the tPlayer, tPlayer cannot be
					// seen (obstructed) and in chase mode, switch to
					// follow path mode.
					// This has one desirable benefit of avoiding physics retries.
					if (aip->behavior == AIB_STATION) {
						ailp->nGoalSegment = aip->nHideSegment;
						CreatePathToStation (objP, 15);
					}
					break;
				}
			}
			break;

		case AIM_OPEN_DOOR: {       // trying to open a door.
			vmsVector center_point, goal_vector;
			Assert (objP->id == ROBOT_BRAIN);     // Make sure this guy is allowed to be in this mode.

			if (!AIMultiplayerAwareness (objP, 62))
				return;
			COMPUTE_SIDE_CENTER (&center_point, &gameData.segs.segments [objP->nSegment], aip->GOALSIDE);
			VmVecSub (&goal_vector, &center_point, &objP->position.vPos);
			VmVecNormalizeQuick (&goal_vector);
			AITurnTowardsVector (&goal_vector, objP, robotP->turnTime [gameStates.app.nDifficultyLevel]);
			move_towards_vector (objP, &goal_vector, 0);
			ai_multi_sendRobot_position (nObject, -1);

			break;
		}

		case AIM_SNIPE_WAIT:
			break;
		case AIM_SNIPE_RETREAT:
			// -- if (AIMultiplayerAwareness (objP, 53))
			// -- 	if (ailp->nextPrimaryFire < -F1_0)
			// -- 		ai_do_actual_firing_stuff (objP, aip, ailp, robotP, &vec_to_player, xDistToPlayer, &vGunPoint, player_visibility, bObjAnimates, aip->CURRENT_GUN);
			break;
		case AIM_SNIPE_RETREAT_BACKWARDS:
		case AIM_SNIPE_ATTACK:
		case AIM_SNIPE_FIRE:
			if (AIMultiplayerAwareness (objP, 53)) {
				ai_do_actual_firing_stuff (objP, aip, ailp, robotP, &vec_to_player, xDistToPlayer, &vGunPoint, player_visibility, bObjAnimates, aip->CURRENT_GUN);
				if (robotP->thief)
					AIMoveRelativeToPlayer (objP, ailp, xDistToPlayer, &vec_to_player, 0, 0, player_visibility);
				break;
			}
			break;

		case AIM_THIEF_WAIT:
		case AIM_THIEF_ATTACK:
		case AIM_THIEF_RETREAT:
		case AIM_WANDER:    // Used for Buddy Bot
			break;

		default:
#if TRACE	
			con_printf (CON_DEBUG, "Unknown mode = %i in robot %i, behavior = %i\n", ailp->mode, OBJ_IDX (objP), aip->behavior);
#endif
			ailp->mode = AIM_CHASE_OBJECT;
			break;
	}       // end: switch (ailp->mode) {

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// If the robot can see you, increase his awareness of you.
	// This prevents the problem of a robot looking right at you but doing nothing.
	// Assert (player_visibility != -1); // Means it didn't get initialized!
	ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);
	if ((player_visibility == 2) && (aip->behavior != AIB_FOLLOW) && (!robotP->thief)) {
		if ((ailp->playerAwarenessType == 0) && (aip->SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE))
			aip->SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		else if (ailp->playerAwarenessType == 0)
			ailp->playerAwarenessType = PA_PLAYER_COLLISION;
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	if (!bObjAnimates) {
		aip->CURRENT_STATE = aip->GOAL_STATE;
	}

	Assert (ailp->playerAwarenessType <= AIE_MAX);
	Assert (aip->CURRENT_STATE < AIS_MAX);
	Assert (aip->GOAL_STATE < AIS_MAX);

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	if (ailp->playerAwarenessType) {
		nNewGoalState = Ai_transition_table [ailp->playerAwarenessType-1][aip->CURRENT_STATE][aip->GOAL_STATE];
		if (nNewGoalState == 6)
			nNewGoalState = 6;
		if (ailp->playerAwarenessType == PA_WEAPON_ROBOT_COLLISION) {
			// Decrease awareness, else this robot will flinch every frame.
			ailp->playerAwarenessType--;
			ailp->playerAwarenessTime = F1_0*3;
		}

		if (nNewGoalState == AIS_ERR_)
			nNewGoalState = AIS_REST;

		if (aip->CURRENT_STATE == AIS_NONE)
			aip->CURRENT_STATE = AIS_REST;

		aip->GOAL_STATE = nNewGoalState;

	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// If new state = fire, then set all gun states to fire.
	if ((aip->GOAL_STATE == AIS_FIRE) ) {
		int i,num_guns;
		num_guns = gameData.bots.pInfo [objP->id].nGuns;
		for (i=0; i<num_guns; i++)
			ailp->goalState [i] = AIS_FIRE;
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// Hack by mk on 01/04/94, if a guy hasn't animated to the firing state, but his nextPrimaryFire says ok to fire, bash him there
	if (ReadyToFire (robotP, ailp) && (aip->GOAL_STATE == AIS_FIRE))
		aip->CURRENT_STATE = AIS_FIRE;

	if ((aip->GOAL_STATE != AIS_FLIN)  && (objP->id != ROBOT_BRAIN)) {
		switch (aip->CURRENT_STATE) {
		case AIS_NONE:
			ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);

			dot = VmVecDot (&objP->position.mOrient.fVec, &vec_to_player);
			if (dot >= F1_0/2)
				if (aip->GOAL_STATE == AIS_REST)
					aip->GOAL_STATE = AIS_SRCH;
			break;
		case AIS_REST:
			if (aip->GOAL_STATE == AIS_REST) {
				ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);
				if (ReadyToFire (robotP, ailp) && (player_visibility)) {
					aip->GOAL_STATE = AIS_FIRE;
				}
			}
			break;
		case AIS_SRCH:
			if (!AIMultiplayerAwareness (objP, 60))
				return;

			ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);

			if (player_visibility == 2) {
				AITurnTowardsVector (&vec_to_player, objP, robotP->turnTime [gameStates.app.nDifficultyLevel]);
				ai_multi_sendRobot_position (nObject, -1);
			}
			break;
		case AIS_LOCK:
			ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);

			if (! (gameData.app.nGameMode & GM_MULTI) || (player_visibility)) {
				if (!AIMultiplayerAwareness (objP, 68))
					return;

				if (player_visibility == 2) {   // @mk, 09/21/95, require that they be looking towards you to turn towards you.
					AITurnTowardsVector (&vec_to_player, objP, robotP->turnTime [gameStates.app.nDifficultyLevel]);
					ai_multi_sendRobot_position (nObject, -1);
				}
			}
			break;
		case AIS_FIRE:
			ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);

			if (player_visibility == 2) {
				if (!AIMultiplayerAwareness (objP, (ROBOT_FIRE_AGITATION-1))) {
					if (gameData.app.nGameMode & GM_MULTI) {
						ai_do_actual_firing_stuff (objP, aip, ailp, robotP, &vec_to_player, xDistToPlayer, &vGunPoint, player_visibility, bObjAnimates, aip->CURRENT_GUN);
						return;
					}
				}
				AITurnTowardsVector (&vec_to_player, objP, robotP->turnTime [gameStates.app.nDifficultyLevel]);
				ai_multi_sendRobot_position (nObject, -1);
			}

			// Fire at tPlayer, if appropriate.
			ai_do_actual_firing_stuff (objP, aip, ailp, robotP, &vec_to_player, xDistToPlayer, &vGunPoint, player_visibility, bObjAnimates, aip->CURRENT_GUN);

			break;
		case AIS_RECO:
			if (! (nObjRef & 3)) {
				ComputeVisAndVec (objP, &vVisPos, ailp, &vec_to_player, &player_visibility, robotP, &nVisAndVecComputed);
				if (player_visibility == 2) {
					if (!AIMultiplayerAwareness (objP, 69))
						return;
					AITurnTowardsVector (&vec_to_player, objP, robotP->turnTime [gameStates.app.nDifficultyLevel]);
					ai_multi_sendRobot_position (nObject, -1);
				} // -- MK, 06/09/95: else if (! (gameData.app.nGameMode & GM_MULTI)) {
			}
			break;
		case AIS_FLIN:
			break;
		default:
#if TRACE	
			con_printf (1, "Unknown mode for AI tObject #%i\n", nObject);
#endif
			aip->GOAL_STATE = AIS_REST;
			aip->CURRENT_STATE = AIS_REST;
			break;
		}
	} // end of: if (aip->GOAL_STATE != AIS_FLIN) {

	// Switch to next gun for next fire.
	if (player_visibility == 0) {
		aip->CURRENT_GUN++;
		if (aip->CURRENT_GUN >= gameData.bots.pInfo [objP->id].nGuns)
		{
			if ((robotP->nGuns == 1) || (robotP->nSecWeaponType == -1))  // Two weapon types hack.
				aip->CURRENT_GUN = 0;
			else
				aip->CURRENT_GUN = 1;
		}
	}
//HUDInitMessage ("%d %d %d", aip->flags [1], aip->flags [2], xDistToPlayer / F1_0);
}

// ----------------------------------------------------------------------------
void AIDoCloakStuff (void)
{
	int i;

	for (i=0; i<MAX_AI_CLOAK_INFO; i++) {
		gameData.ai.cloakInfo [i].last_position = gameData.objs.console->position.vPos;
		gameData.ai.cloakInfo [i].last_segment = gameData.objs.console->nSegment;
		gameData.ai.cloakInfo [i].lastTime = gameData.time.xGame;
	}

	// Make work for control centers.
	gameData.ai.vBelievedPlayerPos = gameData.ai.cloakInfo [0].last_position;
	gameData.ai.nBelievedPlayerSeg = gameData.ai.cloakInfo [0].last_segment;

}

// ----------------------------------------------------------------------------
// Returns false if awareness is considered too puny to add, else returns true.
int add_tAwarenessEvent (tObject *objP, int nType)
{
	// If tPlayer cloaked and hit a robot, then increase awareness
	if ((nType == PA_WEAPON_ROBOT_COLLISION) || (nType == PA_WEAPON_WALL_COLLISION) || (nType == PA_PLAYER_COLLISION))
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
	if (! (gameData.app.nGameMode & GM_MULTI) || (gameData.app.nGameMode & GM_MULTI_ROBOTS)) {
		if (add_tAwarenessEvent (objP, nType)) {
			if (((d_rand () * (nType+4)) >> 15) > 4)
				gameData.ai.nOverallAgitation++;
			if (gameData.ai.nOverallAgitation > OVERALL_AGITATION_MAX)
				gameData.ai.nOverallAgitation = OVERALL_AGITATION_MAX;
		}
	}
}

sbyte New_awareness [MAX_SEGMENTS];

// ----------------------------------------------------------------------------------
void pae_aux (int nSegment, int nType, int level)
{
	int j;

	if (New_awareness [nSegment] < nType)
		New_awareness [nSegment] = nType;

	// Process children.
	for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
		if (IS_CHILD (gameData.segs.segments [nSegment].children [j]))
		{
			if (level <= 3)
			{
				if (nType == 4)
					pae_aux (gameData.segs.segments [nSegment].children [j], nType-1, level+1);
				else
					pae_aux (gameData.segs.segments [nSegment].children [j], nType, level+1);
			}
		}
}


// ----------------------------------------------------------------------------------
void process_tAwarenessEvents (void)
{
	int i;

	if (! (gameData.app.nGameMode & GM_MULTI) || (gameData.app.nGameMode & GM_MULTI_ROBOTS)) {
		memset (New_awareness, 0, sizeof (New_awareness [0]) * (gameData.segs.nLastSegment+1));

		for (i=0; i<gameData.ai.nAwarenessEvents; i++)
			pae_aux (gameData.ai.awarenessEvents [i].nSegment, gameData.ai.awarenessEvents [i].nType, 1);

	}

	gameData.ai.nAwarenessEvents = 0;
}

// ----------------------------------------------------------------------------------
void SetPlayerAwarenessAll (void)
{
	int i;

	process_tAwarenessEvents ();

	for (i=0; i<=gameData.objs.nLastObject; i++)
		if (gameData.objs.objects [i].controlType == CT_AI) {
			if (New_awareness [gameData.objs.objects [i].nSegment] > gameData.ai.localInfo [i].playerAwarenessType) {
				gameData.ai.localInfo [i].playerAwarenessType = New_awareness [gameData.objs.objects [i].nSegment];
				gameData.ai.localInfo [i].playerAwarenessTime = PLAYER_AWARENESS_INITIAL_TIME;
			}

			// Clear the bit that says this robot is only awake because a camera woke it up.
			if (New_awareness [gameData.objs.objects [i].nSegment] > gameData.ai.localInfo [i].playerAwarenessType)
				gameData.objs.objects [i].cType.aiInfo.SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		}
}

#ifndef NDEBUG
int Ai_dump_enable = 0;

FILE *Ai_dump_file = NULL;

char Ai_error_message [128] = "";

// ----------------------------------------------------------------------------------
void force_dump_aiObjects_all (char *msg)
{
	int tsave;

	tsave = Ai_dump_enable;

	Ai_dump_enable = 1;

	sprintf (Ai_error_message, "%s\n", msg);
	//dump_aiObjects_all ();
	Ai_error_message [0] = 0;

	Ai_dump_enable = tsave;
}

// ----------------------------------------------------------------------------------
void turn_off_ai_dump (void)
{
	if (Ai_dump_file != NULL)
		fclose (Ai_dump_file);

	Ai_dump_file = NULL;
}

#endif

extern void DoBossDyingFrame (tObject *objP);

// ----------------------------------------------------------------------------------
// Do things which need to get done for all AI gameData.objs.objects each frame.
// This includes:
//  Setting player_awareness (a fix, time in seconds which tObject is aware of tPlayer)
void DoAIFrameAll (void)
{
	int	h, i, j;

SetPlayerAwarenessAll ();
if (nAiLastMissileCamera != -1) {
	// Clear if supposed misisle camera is not a weapon, or just every so often, just in case.
	if (((gameData.app.nFrameCount & 0x0f) == 0) || (gameData.objs.objects [nAiLastMissileCamera].nType != OBJ_WEAPON)) {
		nAiLastMissileCamera = -1;
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
				if ((objP->nType == OBJ_ROBOT) && (gameData.bots.pInfo [objP->id].bossFlag))
					DoBossDyingFrame (objP);
		}
	}
}


//	-------------------------------------------------------------------------------------------------
extern fix Boss_invulnerable_dot;

// Initializations to be performed for all robots for a new level.
void InitRobotsForLevel (void)
{
	int	i;

gameData.ai.nOverallAgitation = 0;
gameStates.gameplay.bFinalBossIsDead=0;
gameData.escort.nObjNum = 0;
gameData.escort.bMayTalk = 0;
Boss_invulnerable_dot = F1_0/4 - i2f (gameStates.app.nDifficultyLevel)/8;
for (i = 0; i < MAX_BOSS_COUNT; i++)
	gameData.boss [i].nDyingStartTime = 0;
}

//	-------------------------------------------------------------------------------------------------

int AISaveBinState (CFILE *fp)
{
	int	i;

CFWrite (&gameData.ai.bInitialized, sizeof (int), 1, fp);
CFWrite (&gameData.ai.nOverallAgitation, sizeof (int), 1, fp);
CFWrite (gameData.ai.localInfo, sizeof (tAILocal), MAX_OBJECTS, fp);
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

{ 
int temp;
temp = (int) (gameData.ai.freePointSegs - gameData.ai.pointSegs);
CFWrite (&temp, sizeof (int), 1, fp);
}

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

	memset (gameData.ai.localInfo, 0, sizeof (tAILocal));
	memset (gameData.ai.pointSegs, 0, sizeof (tPointSeg));
	CFRead (&gameData.ai.bInitialized, sizeof (int), 1, fp);
	CFRead (&gameData.ai.nOverallAgitation, sizeof (int), 1, fp);
	CFRead (gameData.ai.localInfo, sizeof (tAILocal), (version > 22) ? MAX_OBJECTS : MAX_OBJECTS_D2, fp);
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
	// -- MK, 10/21/95, unused! -- CFRead (&Boss_been_hit, sizeof (int), 1, fp);

	if (version >= 8) {
		CFRead (&gameData.escort.nKillObject, sizeof (gameData.escort.nKillObject), 1, fp);
		CFRead (&gameData.escort.xLastPathCreated, sizeof (gameData.escort.xLastPathCreated), 1, fp);
		CFRead (&gameData.escort.nGoalObject, sizeof (gameData.escort.nGoalObject), 1, fp);
		CFRead (&gameData.escort.nSpecialGoal, sizeof (gameData.escort.nSpecialGoal), 1, fp);
		CFRead (&gameData.escort.nGoalIndex, sizeof (gameData.escort.nGoalIndex), 1, fp);
		CFRead (&gameData.thief.stolenItems, sizeof (gameData.thief.stolenItems [0]), MAX_STOLEN_ITEMS, fp);
	} else {
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
	} else
		AIResetAllPaths ();

	if (version >= 24) {
		for (i = 0; i < MAX_BOSS_COUNT; i++)
			CFRead (&gameData.boss [i].nTeleportSegs, sizeof (gameData.boss [i].nTeleportSegs), 1, fp);
		for (i = 0; i < MAX_BOSS_COUNT; i++)
			CFRead (&gameData.boss [i].nGateSegs, sizeof (gameData.boss [i].nGateSegs), 1, fp);
		for (i = 0; i < MAX_BOSS_COUNT; i++) {
			if (gameData.boss [i].nGateSegs)
				CFRead (gameData.boss [i].gateSegs, sizeof (gameData.boss [i].gateSegs), gameData.boss [i].nGateSegs, fp);
			if (gameData.boss [i].nTeleportSegs)
				CFRead (gameData.boss [i].teleportSegs, sizeof (gameData.boss [i].teleportSegs), gameData.boss [i].nTeleportSegs, fp);
			}
		}
	else if (version >= 21) {
		CFRead (&gameData.boss [0].nTeleportSegs, sizeof (gameData.boss [0].nTeleportSegs), 1, fp);
		CFRead (&gameData.boss [0].nGateSegs, sizeof (gameData.boss [0].nGateSegs), 1, fp);

		if (gameData.boss [0].nGateSegs)
			CFRead (gameData.boss [0].gateSegs, sizeof (gameData.boss [0].gateSegs), gameData.boss [0].nGateSegs, fp);

		if (gameData.boss [0].nTeleportSegs)
			CFRead (gameData.boss [0].teleportSegs, sizeof (gameData.boss [0].teleportSegs), gameData.boss [0].nTeleportSegs, fp);
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

void AISaveLocalInfo (tAILocal *ailP, CFILE *fp)
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
	CFWriteAngVec (ailP->goalAngles + i, fp);  
	CFWriteAngVec (ailP->deltaAngles + i, fp);
	}
CFWrite (ailP->goalState, sizeof (ailP->goalState [0]), 1, fp);
CFWrite (ailP->achievedState, sizeof (ailP->achievedState [0]), 1, fp);
}

//	-------------------------------------------------------------------------------------------------

void AISavePointSeg (tPointSeg *psegP, CFILE *fp)
{
CFWriteInt (psegP->nSegment, fp);
CFWriteVector (&psegP->point, fp);
}

//	-------------------------------------------------------------------------------------------------

void AISaveCloakInfo (tAICloakInfo *ciP, CFILE *fp)
{
CFWriteFix (ciP->lastTime, fp);
CFWriteInt (ciP->last_segment, fp);
CFWriteVector (&ciP->last_position, fp);
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
	AISavePointSeg (gameData.ai.pointSegs+  i, fp);
for (i = 0; i < MAX_AI_CLOAK_INFO; i++)
	AISaveCloakInfo (gameData.ai.cloakInfo + i, fp);
for (i = 0; i < MAX_BOSS_COUNT; i++) {
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
CFWriteInt ((int) (gameData.ai.freePointSegs - gameData.ai.pointSegs), fp);
i =CFTell (fp);
for (i = 0; i < MAX_BOSS_COUNT; i++) {
	CFWriteShort (gameData.boss [i].nTeleportSegs, fp);
	CFWriteShort (gameData.boss [i].nGateSegs, fp);
	}
for (i = 0; i < MAX_BOSS_COUNT; i++) {
	if (h = gameData.boss [i].nGateSegs)
		for (j = 0; j < h; j++)
			CFWriteShort (gameData.boss [i].gateSegs [j], fp);
	if (h = gameData.boss [i].nTeleportSegs)
		for (j = 0; j < h; j++)
			CFWriteShort (gameData.boss [i].teleportSegs [j], fp);
	}
return 1;
}

//	-------------------------------------------------------------------------------------------------

void AIRestoreLocalInfo (tAILocal *ailP, CFILE *fp)
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
	CFReadAngVec (ailP->goalAngles + i, fp);  
	CFReadAngVec (ailP->deltaAngles + i, fp);
	}
CFRead (ailP->goalState, sizeof (ailP->goalState [0]), 1, fp);
CFRead (ailP->achievedState, sizeof (ailP->achievedState [0]), 1, fp);
}

//	-------------------------------------------------------------------------------------------------

void AIRestorePointSeg (tPointSeg *psegP, CFILE *fp)
{
psegP->nSegment = CFReadInt (fp);
CFReadVector (&psegP->point, fp);
}

//	-------------------------------------------------------------------------------------------------

void AIRestoreCloakInfo (tAICloakInfo *ciP, CFILE *fp)
{
ciP->lastTime = CFReadFix (fp);
ciP->last_segment = CFReadInt (fp);
CFReadVector (&ciP->last_position, fp);
}

//	-------------------------------------------------------------------------------------------------

int AIRestoreUniState (CFILE *fp, int version)
{
	int	h, i, j, nMaxBossCount;

memset (gameData.ai.localInfo, 0, sizeof (gameData.ai.localInfo));
memset (gameData.ai.pointSegs, 0, sizeof (gameData.ai.pointSegs));
gameData.ai.bInitialized = CFReadInt (fp);
gameData.ai.nOverallAgitation = CFReadInt (fp);
h = (version > 22) ? MAX_OBJECTS : MAX_OBJECTS_D2;
for (i = 0; i < h; i++)
	AIRestoreLocalInfo (gameData.ai.localInfo + i, fp);
h = (version > 22) ? MAX_POINT_SEGS : MAX_POINT_SEGS_D2;
for (i = 0; i < h; i++)
	AIRestorePointSeg (gameData.ai.pointSegs + i, fp);
for (i = 0; i < MAX_AI_CLOAK_INFO; i++)
	AIRestoreCloakInfo (gameData.ai.cloakInfo + i, fp);
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

if (version >= 15)
	gameData.ai.freePointSegs = gameData.ai.pointSegs + CFReadInt (fp);
else
	AIResetAllPaths ();

if (version < 21) {
	#if TRACE	
	con_printf (1, "Warning: If you fight the boss, he might teleport to tSegment #0!\n");
	#endif
	}
else {
	nMaxBossCount = (version >= 24) ? MAX_BOSS_COUNT : 1;
	i =CFTell (fp);
	for (i = 0; i < MAX_BOSS_COUNT; i++) {
		gameData.boss [i].nTeleportSegs = CFReadShort (fp);
		gameData.boss [i].nGateSegs = CFReadShort (fp);
		}
	for (i = 0; i < MAX_BOSS_COUNT; i++) {
		if (h = gameData.boss [i].nGateSegs)
			for (j = 0; j < h; j++)
				gameData.boss [i].gateSegs [j] = CFReadShort (fp);
		if (h = gameData.boss [i].nTeleportSegs)
			for (j = 0; j < h; j++)
				gameData.boss [i].teleportSegs [j] = CFReadShort (fp);
		}
	}
return 1;
}
//	-------------------------------------------------------------------------------------------------
