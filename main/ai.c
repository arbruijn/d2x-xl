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

/*
 *
 * Autonomous Individual movement.
 *
 * Old Log:
 * Revision 1.1  1995/12/05  14:15:37  allender
 * Initial revision
 *
 * Revision 1.10  1995/11/09  09:36:12  allender
 * cheats not active during demo playback
 *
 * Revision 1.9  1995/11/03  12:51:55  allender
 * shareware changes
 *
 * Revision 1.8  1995/10/31  10:25:07  allender
 * shareware stuff
 *
 * Revision 1.7  1995/10/26  14:01:38  allender
 * optimization for doing robot stuff only if anim angles done last frame
 *
 * Revision 1.6  1995/10/25  09:35:43  allender
 * prototype some functions causing mcc problems
 *
 * Revision 1.5  1995/10/17  13:11:40  allender
 * fix in ai code that makes bots only look for you every so often
 *
 * Revision 1.4  1995/10/10  11:48:10  allender
 * PC ai code
 *
 * Revision 2.11  1995/07/09  11:15:48  john
 * Put in Mike's code to fix bug where bosses don't gate in bots after
 * 32767 seconds of playing.
 *
 * Revision 2.10  1995/06/15  12:31:08  john
 * Fixed bug with cheats getting enabled when you type
 * the whole alphabet.
 *
 * Revision 2.9  1995/05/26  16:16:18  john
 * Split SATURN into define's for requiring cd, using cd, etc.
 * Also started adding all the Rockwell stuff.
 *
 * Revision 2.8  1995/04/06  15:12:27  john
 * Fixed bug with insane not working.
 *
 * Revision 2.7  1995/03/30  16:36:44  mike
 * text localization.
 *
 * Revision 2.6  1995/03/28  11:22:24  john
 * Added cheats to save file. Changed lunacy text.
 *
 * Revision 2.5  1995/03/27  16:45:07  john
 * Fixed some cheat bugs.  Added astral cheat.
 *
 * Revision 2.4  1995/03/24  15:29:17  mike
 * add new cheats.
 *
 * Revision 2.3  1995/03/21  14:39:45  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.2  1995/03/14  18:24:39  john
 * Force Destination Saturn to use CD-ROM drive.
 *
 * Revision 2.1  1995/03/06  16:47:14  mike
 * destination saturn
 *
 * Revision 2.0  1995/02/27  11:30:01  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.295  1995/02/22  13:23:04  allender
 * remove anonymous unions from object structure
 *
 * Revision 1.294  1995/02/13  11:00:43  rob
 * Make brain guys high enough to get an open slot.
 *
 * Revision 1.293  1995/02/13  10:31:55  mike
 * Make brains understand they can't open locked doors.
 *
 * Revision 1.292  1995/02/13  10:18:01  rob
 * Reduced brain guy's level of awareness to keep him from hogging slots.
 *
 * Revision 1.291  1995/02/11  12:27:12  mike
 * fix path-to-exit cheat.
 *
 * Revision 1.290  1995/02/11  01:56:30  mike
 * robots don't fire cheat.
 *
 * Revision 1.289  1995/02/10  17:15:09  rob
 * Fixed some stuff with 64 awareness stuff.
 *
 * Revision 1.288  1995/02/10  16:31:32  mike
 * oops.
 *
 * Revision 1.287  1995/02/10  16:24:45  mike
 * fix the network follow path fix.
 *
 * Revision 1.286  1995/02/10  16:11:40  mike
 * in serial or modem games, follow path guys don't move if far away and
 * can't see player.
 *
 * Revision 1.285  1995/02/09  13:11:35  mike
 * comment out a bunch of mprintfs.
 * add toaster (drops prox bombs, runs away) to boss gate list.
 *
 * Revision 1.284  1995/02/08  22:44:53  rob
 * Lowerd anger level for follow path of any sort.
 *
 * Revision 1.283  1995/02/08  22:30:43  mike
 * lower awareness on station guys if they are returning home (multiplayer).
 *
 * Revision 1.282  1995/02/08  17:01:06  rob
 * Fixed problem with toasters dropping of proximity bombs.
 *
 * Revision 1.281  1995/02/08  11:49:35  rob
 * Reduce Green-guy attack awareness level so we don't let him attack us too.
 *
 * Revision 1.280  1995/02/08  11:37:52  mike
 * Check for failures in call to CreateObject.
 *
 * Revision 1.279  1995/02/07  20:38:46  mike
 * fix toasters in multiplayer
 *
 *
 * Revision 1.278  1995/02/07  16:51:07  mike
 * fix sound time play bug.
 *
 * Revision 1.277  1995/02/06  22:33:04  mike
 * make robots follow path better in cooperative/roboarchy.
 *
 * Revision 1.276  1995/02/06  18:15:42  rob
 * Added forced sends for evasion movemnet.
 *
 * Revision 1.275  1995/02/06  16:41:22  rob
 * Change some positioning calls.
 *
 * Revision 1.274  1995/02/06  11:40:33  mike
 * replace some lint-related hacks with clean, proper code.
 *
 * Revision 1.273  1995/02/04  17:28:19  mike
 * make station guys return better.
 *
 * Revision 1.272  1995/02/03  17:40:55  mike
 * fix problem with robots falling asleep if you sit in game overnight, not in pause...bah.
 *
 * Revision 1.271  1995/02/02  21:11:25  rob
 * Tweaking stuff for multiplayer ai.
 *
 * Revision 1.270  1995/02/02  17:32:06  john
 * Added Hack for Assert that Mike put in after using Lint to find
 * uninitialized variables.
 *
 * Revision 1.269  1995/02/02  16:46:31  mike
 * fix boss gating.
 *
 * Revision 1.268  1995/02/02  16:27:29  mike
 * make boss not put out infinite robots.
 *
 * Revision 1.267  1995/02/01  21:10:02  mike
 * lint found bug! player_visibility not initialized!
 *
 * Revision 1.266  1995/02/01  20:51:27  john
 * Lintized
 *
 * Revision 1.265  1995/02/01  17:14:05  mike
 * fix robot sounds.
 *
 * Revision 1.264  1995/01/31  16:16:40  mike
 * Comment out "Darn you, John" Int3().
 *
 * Revision 1.263  1995/01/30  20:55:04  mike
 * fix nonsense in robot firing when a player is cloaked.
 *
 * Revision 1.262  1995/01/30  17:15:10  rob
 * Fixed problems with bigboss eclip messages.
 * Tweaked robot position sending for modem purposes.
 *
 * Revision 1.261  1995/01/30  15:30:31  rob
 * Prevent non-master players from gating in robots.
 *
 * Revision 1.260  1995/01/30  13:30:55  mike
 * new cases for firing at other players were bogus, could send position
 * without permission.
 *
 * Revision 1.259  1995/01/30  13:01:17  mike
 * Make robots fire at player other than one they are controlled by sometimes.
 *
 * Revision 1.258  1995/01/29  16:09:17  rob
 * Trying to get robots to shoot at non-controlling players.
 *
 * Revision 1.257  1995/01/29  13:47:05  mike
 * Make boss have more fireballs on death, have until end (though silent at end).
 * Fix bug which was preventing him from teleporting until hit, so he'd always
 * be in the same place when the player enters the room.
 *
 * Revision 1.256  1995/01/28  17:40:18  mike
 * make boss teleport & gate before you see him.
 *
 * Revision 1.255  1995/01/27  17:02:08  mike
 * move code around, was sending one frame (or worse!) old robot information.
 *
 * Revision 1.254  1995/01/26  17:02:43  mike
 * make fusion cannon have more chrome, make fusion, mega rock you!
 *
 * Revision 1.253  1995/01/26  15:11:17  rob
 * Shutup!  I fixed it!
 *
 * Revision 1.252  1995/01/26  15:08:55  rob
 * Changed robot gating to accomodate multiplayer.
 *
 * Revision 1.251  1995/01/26  14:49:04  rob
 * Increase awareness level for firing to 94.
 *
 * Revision 1.250  1995/01/26  12:41:20  mike
 * fix bogus multiplayer code, would send permission without getting permission.
 *
 * Revision 1.249  1995/01/26  12:23:23  rob
 * Removed defines that were moved to ai.h
 *
 * Revision 1.248  1995/01/25  23:38:48  mike
 * modify list of robots gated in by super boss.
 *
 * Revision 1.247  1995/01/25  21:21:13  rob
 * Trying to let robots fire at a player even if they're not in control.
 *
 * Revision 1.246  1995/01/25  13:50:37  mike
 * Robots make angry sounds.
 *
 * Revision 1.245  1995/01/25  10:53:47  mike
 * better handling of robots which poke out of mine and try to recover.
 *
 * Revision 1.244  1995/01/24  22:03:02  mike
 * Tricky code to move a robot to a legal position if he is poking out of
 * the mine, even if it means moving him to another segment.
 *
 * Revision 1.243  1995/01/24  20:12:06  rob
 * Changed robot fire awareness level from 74 to 94.
 *
 * Revision 1.242  1995/01/24  13:22:32  mike
 * make robots accelerate faster, and gameStates.app.nDifficultyLevel dependent.
 *
 * Revision 1.241  1995/01/24  12:09:39  mike
 * make robots animate in multiplayer.
 *
 * Revision 1.240  1995/01/21  21:21:10  mike
 * Make boss only gate robots into specified segments.
 *
 * Revision 1.239  1995/01/20  20:21:26  mike
 * prevent unnecessary boss cloaking.
 *
 */


#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

char ai_rcsid[] = "$Id: ai.c,v 1.7 2003/10/04 02:58:23 btb Exp $";

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
#ifdef NETWORK
#include "multi.h"
#include "network.h"
#endif
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

ubyte Boss_teleports[NUM_D2_BOSSES]           = {1,1,1,1,1,1, 1,1}; // Set byte if this boss can teleport
ubyte Boss_spew_more[NUM_D2_BOSSES]           = {0,1,0,0,0,0, 0,0}; // If set, 50% of time, spew two bots.
ubyte Boss_spews_bots_energy[NUM_D2_BOSSES]   = {1,1,0,1,0,1, 1,1}; // Set byte if boss spews bots when hit by energy weapon.
ubyte Boss_spews_bots_matter[NUM_D2_BOSSES]   = {0,0,1,1,1,1, 0,1}; // Set byte if boss spews bots when hit by matter weapon.
ubyte Boss_invulnerable_energy[NUM_D2_BOSSES] = {0,0,1,1,0,0, 0,0}; // Set byte if boss is invulnerable to energy weapons.
ubyte Boss_invulnerable_matter[NUM_D2_BOSSES] = {0,0,0,0,1,1, 1,0}; // Set byte if boss is invulnerable to matter weapons.
ubyte Boss_invulnerable_spot[NUM_D2_BOSSES]   = {0,0,0,0,0,1, 0,1}; // Set byte if boss is invulnerable in all but a certain spot.  (Dot product fvec|vec_to_collision < BOSS_INVULNERABLE_DOT)

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

// -- sbyte Super_boss_gate_list[MAX_GATE_INDEX] = {0, 1, 8, 9, 10, 11, 12, 15, 16, 18, 19, 20, 22, 0, 8, 11, 19, 20, 8, 20, 8};

// These globals are set by a call to FindVectorIntersection, which is a slow routine,
// so we don't want to call it again (for this object) unless we have to.

#ifndef NDEBUG
// Index into this array with ailp->mode
char *mode_text[18] = {
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
char behavior_text[6][9] = {
	"STILL   ",
	"NORMAL  ",
	"HIDE    ",
	"RUN_FROM",
	"FOLPATH ",
	"STATION "
};

// Index into this array with aip->GOAL_STATE or aip->CURRENT_STATE
char state_text[8][5] = {
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
// Transition table between states for an AI object.
// First dimension is trigger event.
// Second dimension is current state.
// Third dimension is goal state.
// Result is new goal state.
// ERR_ means something impossible has happened.
sbyte Ai_transition_table[AI_MAX_EVENT][AI_MAX_STATE][AI_MAX_STATE] = {
	{
		// Event = AIE_FIRE, a nearby object fired
		// none     rest      srch      lock      flin      fire      reco        // CURRENT is rows, GOAL is columns
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO }, // none
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO }, // rest
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO }, // search
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO }, // lock
		{ AIS_ERR_, AIS_REST, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FIRE, AIS_RECO }, // flinch
		{ AIS_ERR_, AIS_FIRE, AIS_FIRE, AIS_FIRE, AIS_FLIN, AIS_FIRE, AIS_RECO }, // fire
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_FIRE }  // recoil
	},

	// Event = AIE_HITT, a nearby object was hit (or a wall was hit)
	{
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FLIN},
		{ AIS_ERR_, AIS_REST, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_FIRE}
	},

	// Event = AIE_COLL, player collided with robot
	{
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_LOCK, AIS_FLIN, AIS_FLIN},
		{ AIS_ERR_, AIS_REST, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_FIRE}
	},

	// Event = AIE_HURT, player hurt robot (by firing at and hitting it)
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
void InitAIFrame(void)
{
	int ab_state;

gameData.ai.nDistToLastPlayerPosFiredAt = 
	VmVecDistQuick (&Last_fired_upon_player_pos, &gameData.ai.vBelievedPlayerPos);
ab_state = xAfterburnerCharge && Controls.afterburner_state && 
			  (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_AFTERBURNER);
if (!(gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED) || 
	 (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_HEADLIGHT_ON) || ab_state)
	AIDoCloakStuff();
}

// ----------------------------------------------------------------------------
// Return firing status.
// If ready to fire a weapon, return true, else return false.
// Ready to fire a weapon if next_fire <= 0 or next_fire2 <= 0.
int ready_to_fire(robot_info *robptr, ai_local *ailp)
{
return (ailp->next_fire <= 0) || ((robptr->weapon_type2 != -1) && (ailp->next_fire2 <= 0));
}

// ----------------------------------------------------------------------------
// Make a robot near the player snipe.
#define	MNRS_SEG_MAX	70
void make_nearby_robot_snipe(void)
{
	int bfs_length, i;
	short bfs_list[MNRS_SEG_MAX];

	CreateBfsList(gameData.objs.console->segnum, bfs_list, &bfs_length, MNRS_SEG_MAX);

	for (i=0; i<bfs_length; i++) {
		int objnum = gameData.segs.segments[bfs_list[i]].objects;

		while (objnum != -1) {
			object *objP = gameData.objs.objects + objnum;
			robot_info *robptr = gameData.bots.pInfo + objP->id;

			if ((objP->type == OBJ_ROBOT) && (objP->id != ROBOT_BRAIN)) {
				if ((objP->ctype.ai_info.behavior != AIB_SNIPE) && 
					 (objP->ctype.ai_info.behavior != AIB_RUN_FROM) && 
					 !gameData.bots.pInfo[objP->id].boss_flag && 
					 !robptr->companion) {
					objP->ctype.ai_info.behavior = AIB_SNIPE;
					gameData.ai.localInfo[objnum].mode = AIM_SNIPE_ATTACK;
#if TRACE	
					con_printf (CON_DEBUG, "Making robot #%i go into snipe mode!\n", objnum);
#endif
					return;
				}
			}
			objnum = objP->next;
		}
	}
#if TRACE	
	con_printf (CON_DEBUG, "Couldn't find a robot to make snipe!\n");
#endif
}

int nAiLastMissileCamera;

//	-------------------------------------------------------------------------------------------------

void DoSnipeFrame(object *objP, fix dist_to_player, int player_visibility, vms_vector *vec_to_player)
{
	int			objnum = OBJ_IDX (objP);
	ai_local		*ailp = &gameData.ai.localInfo[objnum];
	fix			connected_distance;

	if (dist_to_player > F1_0*500)
		return;

	switch (ailp->mode) {
		case AIM_SNIPE_WAIT:
			if ((dist_to_player > F1_0*50) && (ailp->next_action_time > 0))
				return;

			ailp->next_action_time = SNIPE_WAIT_TIME;

			connected_distance = FindConnectedDistance(&objP->pos, objP->segnum, &gameData.ai.vBelievedPlayerPos, gameData.ai.nBelievedPlayerSeg, 30, WID_FLY_FLAG);
			if (connected_distance < F1_0*500) {
				CreatePathToPlayer(objP, 30, 1);
				ailp->mode = AIM_SNIPE_ATTACK;
				ailp->next_action_time = SNIPE_ATTACK_TIME;	//	have up to 10 seconds to find player.
			}
			break;

		case AIM_SNIPE_RETREAT:
		case AIM_SNIPE_RETREAT_BACKWARDS:
			if (ailp->next_action_time < 0) {
				ailp->mode = AIM_SNIPE_WAIT;
				ailp->next_action_time = SNIPE_WAIT_TIME;
			} else if ((player_visibility == 0) || (ailp->next_action_time > SNIPE_ABORT_RETREAT_TIME)) {
				ai_follow_path(objP, player_visibility, player_visibility, vec_to_player);
				ailp->mode = AIM_SNIPE_RETREAT_BACKWARDS;
			} else {
				ailp->mode = AIM_SNIPE_FIRE;
				ailp->next_action_time = SNIPE_FIRE_TIME/2;
			}
			break;

		case AIM_SNIPE_ATTACK:
			if (ailp->next_action_time < 0) {
				ailp->mode = AIM_SNIPE_RETREAT;
				ailp->next_action_time = SNIPE_WAIT_TIME;
			} else {
				ai_follow_path(objP, player_visibility, player_visibility, vec_to_player);
				if (player_visibility) {
					ailp->mode = AIM_SNIPE_FIRE;
					ailp->next_action_time = SNIPE_FIRE_TIME;
				} else
					ailp->mode = AIM_SNIPE_ATTACK;
			}
			break;

		case AIM_SNIPE_FIRE:
			if (ailp->next_action_time < 0) {
				ai_static	*aip = &objP->ctype.ai_info;
				create_n_segment_path(objP, 10 + d_rand()/2048, gameData.objs.console->segnum);
				aip->path_length = polish_path(objP, &gameData.ai.pointSegs[aip->hide_index], aip->path_length);
				if (d_rand() < 8192)
					ailp->mode = AIM_SNIPE_RETREAT_BACKWARDS;
				else
					ailp->mode = AIM_SNIPE_RETREAT;
				ailp->next_action_time = SNIPE_RETREAT_TIME;
			} else {
			}
			break;

		default:
			Int3();	//	Oops, illegal mode for snipe behavior.
			ailp->mode = AIM_SNIPE_ATTACK;
			ailp->next_action_time = F1_0;
			break;
	}
}

// --------------------------------------------------------------------------------------------------------------------

void DoAIFrame(object *objP)
{
	short         objnum = OBJ_IDX (objP);
	ai_static   *aip = &objP->ctype.ai_info;
	ai_local    *ailp = &gameData.ai.localInfo[objnum];
	fix         dist_to_player;
	vms_vector  vec_to_player;
	fix         dot;
	robot_info  *robptr;
	int         player_visibility=-1;
	int         obj_ref;
	int         object_animates;
	int         new_goal_state;
	int         visibility_and_vec_computed = 0;
	int         previous_visibility;
	vms_vector  gun_point;
	vms_vector  vis_vec_pos;

	ailp->next_action_time -= gameData.time.xFrame;

	if (aip->SKIP_AI_COUNT) {
		aip->SKIP_AI_COUNT--;
		if (objP->mtype.phys_info.flags & PF_USES_THRUST) {
			objP->mtype.phys_info.rotthrust.x = (objP->mtype.phys_info.rotthrust.x * 15)/16;
			objP->mtype.phys_info.rotthrust.y = (objP->mtype.phys_info.rotthrust.y * 15)/16;
			objP->mtype.phys_info.rotthrust.z = (objP->mtype.phys_info.rotthrust.z * 15)/16;
			if (!aip->SKIP_AI_COUNT)
				objP->mtype.phys_info.flags &= ~PF_USES_THRUST;
		}
		return;
	}

	robptr = gameData.bots.pInfo + objP->id;
	Assert(robptr->always_0xabcd == 0xabcd);

	if (do_any_robot_dying_frame(objP))
		return;

	// Kind of a hack.  If a robot is flinching, but it is time for it to fire, unflinch it.
	// Else, you can turn a big nasty robot into a wimp by firing flares at it.
	// This also allows the player to see the cool flinch effect for mechs without unbalancing the game.
	if ((aip->GOAL_STATE == AIS_FLIN) && ready_to_fire(robptr, ailp)) {
		aip->GOAL_STATE = AIS_FIRE;
	}

#ifndef NDEBUG
	if ((aip->behavior == AIB_RUN_FROM) && (ailp->mode != AIM_RUN_FROM_OBJECT))
		Int3(); // This is peculiar.  Behavior is run from, but mode is not.  Contact Mike.

	mprintf_animation_info (objP);

	if (!Do_ai_flag)
		return;

	if (Break_on_object != -1)
		if ((OBJ_IDX (objP)) == Break_on_object)
			Int3(); // Contact Mike: This is a debug break
#endif
#if TRACE	
	//con_printf (CON_DEBUG, "Object %i: behavior = %02x, mode = %i, awareness = %i, time = %7.3f\n", OBJ_IDX (objP), aip->behavior, ailp->mode, ailp->player_awareness_type, f2fl(ailp->player_awareness_time));
	//con_printf (CON_DEBUG, "Object %i: behavior = %02x, mode = %i, awareness = %i, cur=%i, goal=%i\n", OBJ_IDX (objP), aip->behavior, ailp->mode, ailp->player_awareness_type, aip->CURRENT_STATE, aip->GOAL_STATE);
#endif
	//Assert((aip->behavior >= MIN_BEHAVIOR) && (aip->behavior <= MAX_BEHAVIOR);
	if (!((aip->behavior >= MIN_BEHAVIOR) && (aip->behavior <= MAX_BEHAVIOR))) {
#if TRACE	
		//con_printf (CON_DEBUG, "Object %i behavior is %i, setting to AIB_NORMAL, fix in editor!\n", objnum, aip->behavior);
#endif
		aip->behavior = AIB_NORMAL;
	}

	Assert(objP->segnum != -1);
	//Assert(objP->id < gameData.bots.nTypes [gameStates.app.bD1Data]);

	obj_ref = objnum ^ gameData.app.nFrameCount;

	if (ailp->next_fire > -F1_0*8)
		ailp->next_fire -= gameData.time.xFrame;

	if (robptr->weapon_type2 != -1) {
		if (ailp->next_fire2 > -F1_0*8)
			ailp->next_fire2 -= gameData.time.xFrame;
	} else
		ailp->next_fire2 = F1_0*8;

	if (ailp->time_since_processed < F1_0*256)
		ailp->time_since_processed += gameData.time.xFrame;

	previous_visibility = ailp->previous_visibility;    //  Must get this before we toast the master copy!

	// -- (No robots have this behavior...)
	// -- // Deal with cloaking for robots which are cloaked except just before firing.
	// -- if (robptr->cloak_type == RI_CLOAKED_EXCEPT_FIRING)
	// -- 	if (ailp->next_fire < F1_0/2)
	// -- 		aip->CLOAKED = 1;
	// -- 	else
	// -- 		aip->CLOAKED = 0;

	// If only awake because of a camera, make that the believed player position.
	if ((aip->SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE) && (nAiLastMissileCamera != -1))
		gameData.ai.vBelievedPlayerPos = gameData.objs.objects[nAiLastMissileCamera].pos;
	else {
		if (gameStates.app.cheats.bRobotsKillRobots) {
			vis_vec_pos = objP->pos;
			ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
			if (player_visibility) {
				int ii, min_obj = -1;
				fix min_dist = F1_0*200, cur_dist;

				for (ii=0; ii<=gameData.objs.nLastObject; ii++)
					if ((gameData.objs.objects[ii].type == OBJ_ROBOT) && (ii != objnum)) {
						cur_dist = VmVecDistQuick(&objP->pos, &gameData.objs.objects[ii].pos);

						if (cur_dist < F1_0*100)
							if (ObjectToObjectVisibility(objP, &gameData.objs.objects[ii], FQ_TRANSWALL))
								if (cur_dist < min_dist) {
									min_obj = ii;
									min_dist = cur_dist;
								}
					}
				if (min_obj != -1) {
					gameData.ai.vBelievedPlayerPos = gameData.objs.objects[min_obj].pos;
					gameData.ai.nBelievedPlayerSeg = gameData.objs.objects[min_obj].segnum;
					VmVecNormalizedDirQuick(&vec_to_player, &gameData.ai.vBelievedPlayerPos, &objP->pos);
				} else
					goto _exit_cheat;
			} else
				goto _exit_cheat;
		} else {
_exit_cheat:
			visibility_and_vec_computed = 0;
			if (!(gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED))
				gameData.ai.vBelievedPlayerPos = gameData.objs.console->pos;
			else
				gameData.ai.vBelievedPlayerPos = gameData.ai.cloakInfo[objnum & (MAX_AI_CLOAK_INFO-1)].last_position;
		}
	}
	dist_to_player = VmVecDistQuick(&gameData.ai.vBelievedPlayerPos, &objP->pos);
	//if (robptr->companion) {
#if TRACE	
	//	con_printf (CON_DEBUG, "%3i: %3i %8.3f %8s %8s [%3i %4i]\n", objnum, objP->segnum, f2fl(dist_to_player), mode_text[ailp->mode], behavior_text[aip->behavior-0x80], aip->hide_index, aip->path_length);
#endif
	//}
	// If this robot can fire, compute visibility from gun position.
	// Don't want to compute visibility twice, as it is expensive.  (So is call to calc_gun_point).
	if ((previous_visibility || !(obj_ref & 3)) && ready_to_fire(robptr, ailp) && (dist_to_player < F1_0*200) && (robptr->n_guns) && !(robptr->attack_type)) {
		// Since we passed ready_to_fire(), either next_fire or next_fire2 <= 0.  calc_gun_point from relevant one.
		// If both are <= 0, we will deal with the mess in ai_do_actual_firing_stuff
		if (ailp->next_fire <= 0)
			calc_gun_point(&gun_point, objP, aip->CURRENT_GUN);
		else
			calc_gun_point(&gun_point, objP, 0);
		vis_vec_pos = gun_point;
	} else {
		vis_vec_pos = objP->pos;
		VmVecZero(&gun_point);
#if TRACE	
		//con_printf (CON_DEBUG, "Visibility = %i, computed from center.\n", player_visibility);
#endif
	}

// MK: Debugging, July 26, 1995!
// if (objnum == 1)
// {
// 	ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
#if TRACE	
// 	con_printf (CON_DEBUG, "Frame %i: dist=%7.3f, vecdot = %7.3f, mode=%i\n", gameData.app.nFrameCount, f2fl(dist_to_player), f2fl(VmVecDot(&vec_to_player, &objP->orient.fvec)), ailp->mode);
#endif
// }
	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - 
	// Occasionally make non-still robots make a path to the player.  Based on agitation and distance from player.
	if ((aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_RUN_FROM) && (aip->behavior != AIB_STILL) && !(gameData.app.nGameMode & GM_MULTI) && (robptr->companion != 1) && (robptr->thief != 1))
		if (gameData.ai.nOverallAgitation > 70) {
			if ((dist_to_player < F1_0*200) && (d_rand() < gameData.time.xFrame/4)) {
				if (d_rand() * (gameData.ai.nOverallAgitation - 40) > F1_0*5) {
					CreatePathToPlayer(objP, 4 + gameData.ai.nOverallAgitation/8 + gameStates.app.nDifficultyLevel, 1);
					return;
				}
			}
		}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// If retry count not 0, then add it into consecutive_retries.
	// If it is 0, cut down consecutive_retries.
	// This is largely a hack to speed up physics and deal with stupid
	// AI.  This is low level communication between systems of a sort
	// that should not be done.
	if ((ailp->retry_count) && !(gameData.app.nGameMode & GM_MULTI)) {
		ailp->consecutive_retries += ailp->retry_count;
		ailp->retry_count = 0;
		if (ailp->consecutive_retries > 3) {
			switch (ailp->mode) {
				case AIM_GOTO_PLAYER:
					// -- Buddy_got_stuck = 1;
					move_towards_segment_center(objP);
					CreatePathToPlayer(objP, 100, 1);
					// -- Buddy_got_stuck = 0;
					break;
				case AIM_GOTO_OBJECT:
					gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
					//if (objP->segnum == gameData.objs.console->segnum) {
					//	if (gameData.ai.pointSegs[aip->hide_index + aip->cur_path_index].segnum == objP->segnum)
					//		if ((aip->cur_path_index + aip->PATH_DIR >= 0) && (aip->cur_path_index + aip->PATH_DIR < aip->path_length-1))
					//			aip->cur_path_index += aip->PATH_DIR;
					//}
					break;
				case AIM_CHASE_OBJECT:
					CreatePathToPlayer(objP, 4 + gameData.ai.nOverallAgitation/8 + gameStates.app.nDifficultyLevel, 1);
					break;
				case AIM_STILL:
					if (robptr->attack_type)
						move_towards_segment_center(objP);
					else if (!((aip->behavior == AIB_STILL) || (aip->behavior == AIB_STATION) || (aip->behavior == AIB_FOLLOW)))    // Behavior is still, so don't follow path.
						attempt_to_resume_path(objP);
					break;
				case AIM_FOLLOW_PATH:
					if (gameData.app.nGameMode & GM_MULTI) {
						ailp->mode = AIM_STILL;
					} else
						attempt_to_resume_path(objP);
					break;
				case AIM_RUN_FROM_OBJECT:
					move_towards_segment_center(objP);
					objP->mtype.phys_info.velocity.x = 0;
					objP->mtype.phys_info.velocity.y = 0;
					objP->mtype.phys_info.velocity.z = 0;
					create_n_segment_path(objP, 5, -1);
					ailp->mode = AIM_RUN_FROM_OBJECT;
					break;
				case AIM_BEHIND:
#if TRACE	
					con_printf (CON_DEBUG, "Hiding robot (%i) collided much.\n", OBJ_IDX (objP));
#endif
					move_towards_segment_center(objP);
					objP->mtype.phys_info.velocity.x = 0;
					objP->mtype.phys_info.velocity.y = 0;
					objP->mtype.phys_info.velocity.z = 0;
					break;
				case AIM_OPEN_DOOR:
					create_n_segment_path_to_door(objP, 5, -1);
					break;
				#ifndef NDEBUG
				case AIM_FOLLOW_PATH_2:
					Int3(); // Should never happen!
					break;
				#endif
			}
			ailp->consecutive_retries = 0;
		}
	} else
		ailp->consecutive_retries /= 2;

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// If in materialization center, exit
	if (!(gameData.app.nGameMode & GM_MULTI) && (gameData.segs.segment2s[objP->segnum].special == SEGMENT_IS_ROBOTMAKER)) {
		if (gameData.matCens.fuelCenters[gameData.segs.segment2s[objP->segnum].value].Enabled) {
			ai_follow_path(objP, 1, 1, NULL);    // 1 = player is visible, which might be a lie, but it works.
			return;
		}
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// Decrease player awareness due to the passage of time.
	if (ailp->player_awareness_type) {
		if (ailp->player_awareness_time > 0) {
			ailp->player_awareness_time -= gameData.time.xFrame;
			if (ailp->player_awareness_time <= 0) {
				ailp->player_awareness_time = F1_0*2;   //new: 11/05/94
				ailp->player_awareness_type--;          //new: 11/05/94
			}
		} else {
			ailp->player_awareness_type--;
			ailp->player_awareness_time = F1_0*2;
			//aip->GOAL_STATE = AIS_REST;
		}
	} else
		aip->GOAL_STATE = AIS_REST;                     //new: 12/13/94


	if (gameStates.app.bPlayerIsDead && (ailp->player_awareness_type == 0))
		if ((dist_to_player < F1_0*200) && (d_rand() < gameData.time.xFrame/8)) {
			if ((aip->behavior != AIB_STILL) && (aip->behavior != AIB_RUN_FROM)) {
				if (!ai_multiplayer_awareness(objP, 30))
					return;
				#ifndef SHAREWARE
				ai_multi_send_robot_position(objnum, -1);
				#endif

				if (!((ailp->mode == AIM_FOLLOW_PATH) && (aip->cur_path_index < aip->path_length-1)))
					if ((aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_RUN_FROM)) {
						if (dist_to_player < F1_0*30)
							create_n_segment_path(objP, 5, 1);
						else
							CreatePathToPlayer(objP, 20, 1);
					}
			}
		}

	// -- // Make sure that if this guy got hit or bumped, then he's chasing player.
	// -- if ((ailp->player_awareness_type == PA_WEAPON_ROBOT_COLLISION) || (ailp->player_awareness_type >= PA_PLAYER_COLLISION)) {
	// -- 	if ((ailp->mode != AIM_BEHIND) && (aip->behavior != AIB_STILL) && (aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_RUN_FROM) && (!robptr->companion) && (!robptr->thief) && (objP->id != ROBOT_BRAIN)) {
	// -- 		ailp->mode = AIM_CHASE_OBJECT;
	// -- 		ailp->player_awareness_type = 0;
	// -- 		ailp->player_awareness_time = 0;
	// -- 	}
	// -- }

	// Make sure that if this guy got hit or bumped, then he's chasing player.
	if ((ailp->player_awareness_type == PA_WEAPON_ROBOT_COLLISION) || 
		 (ailp->player_awareness_type >= PA_PLAYER_COLLISION)) {
		ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
		if (player_visibility == 1) // Only increase visibility if unobstructed, else claw guys attack through doors.
			player_visibility = 2;
	} else if (((obj_ref&3) == 0) && !previous_visibility && (dist_to_player < F1_0*100)) {
		fix sval, rval;

		rval = d_rand();
		sval = (dist_to_player * (gameStates.app.nDifficultyLevel+1))/64;

		if ((FixMul(rval, sval) < gameData.time.xFrame) || (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_HEADLIGHT_ON)) {
			ailp->player_awareness_type = PA_PLAYER_COLLISION;
			ailp->player_awareness_time = F1_0*3;
			ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
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
	if (gameData.ai.bEnableAnimation && (dist_to_player < F1_0*100)) { // && !(gameData.app.nGameMode & GM_MULTI)) {
		object_animates = do_silly_animation(objP);
		if (object_animates)
			ai_frame_animation(objP);
	} else {
		// If Object is supposed to animate, but we don't let it animate due to distance, then
		// we must change its state, else it will never update.
		aip->CURRENT_STATE = aip->GOAL_STATE;
		object_animates = 0;        // If we're not doing the animation, then should pretend it doesn't animate.
	}

	switch (gameData.bots.pInfo[objP->id].boss_flag) {
	case 0:
		break;

	case 1:
	case 2:
//		break;

	default:
		{
			int	pv;
			fix	dtp = dist_to_player/4;

			if (aip->GOAL_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_FIRE;
			if (aip->CURRENT_STATE == AIS_FLIN)
				aip->CURRENT_STATE = AIS_FIRE;

			ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			pv = player_visibility;

			// If player cloaked, visibility is screwed up and superboss will gate in robots when not supposed to.
			if (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED) {
				pv = 0;
				dtp = VmVecDistQuick(&gameData.objs.console->pos, &objP->pos)/4;
			}

			do_boss_stuff(objP, pv);
		}
		break;
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// Time-slice, don't process all the time, purely an efficiency hack.
	// Guys whose behavior is station and are not at their hide segment get processed anyway.
	if (!((aip->behavior == AIB_SNIPE) && (ailp->mode != AIM_SNIPE_WAIT)) && !robptr->companion && !robptr->thief && (ailp->player_awareness_type < PA_WEAPON_ROBOT_COLLISION-1)) { // If robot got hit, he gets to attack player always!
#ifndef NDEBUG
		if (Break_on_object != objnum) {    // don't time slice if we're interested in this object.
#endif
			if ((aip->behavior == AIB_STATION) && (ailp->mode == AIM_FOLLOW_PATH) && (aip->hide_segment != objP->segnum)) {
				if (dist_to_player > F1_0*250)  // station guys not at home always processed until 250 units away.
					return;
			} else if ((!ailp->previous_visibility) && ((dist_to_player >> 7) > ailp->time_since_processed)) {  // 128 units away (6.4 segments) processed after 1 second.
				return;
			}
#ifndef NDEBUG
		}
#endif
	}

	// Reset time since processed, but skew gameData.objs.objects so not everything
	// processed synchronously, else we get fast frames with the
	// occasional very slow frame.
	// AI_proc_time = ailp->time_since_processed;
	ailp->time_since_processed = - ((objnum & 0x03) * gameData.time.xFrame ) / 2;

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	//	Perform special ability
	switch (objP->id) {
		case ROBOT_BRAIN:
			// Robots function nicely if behavior is gameData.matCens.fuelCenters.  This
			// means they won't move until they can see the player, at
			// which time they will start wandering about opening doors.
			if (gameData.objs.console->segnum == objP->segnum) {
				if (!ai_multiplayer_awareness(objP, 97))
					return;
				ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
				move_away_from_player(objP, &vec_to_player, 0);
				ai_multi_send_robot_position(objnum, -1);
			} else if (ailp->mode != AIM_STILL) {
				int r;

				r = openable_doors_in_segment(objP->segnum);
				if (r != -1) {
					ailp->mode = AIM_OPEN_DOOR;
					aip->GOALSIDE = r;
				} else if (ailp->mode != AIM_FOLLOW_PATH) {
					if (!ai_multiplayer_awareness(objP, 50))
						return;
					create_n_segment_path_to_door(objP, 8+gameStates.app.nDifficultyLevel, -1);     // third parameter is avoid_seg, -1 means avoid nothing.
					ai_multi_send_robot_position(objnum, -1);
				}

				if (ailp->next_action_time < 0) {
					ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
					if (player_visibility) {
						make_nearby_robot_snipe();
						ailp->next_action_time = (NDL - gameStates.app.nDifficultyLevel) * 2*F1_0;
					}
				}
			} else {
				ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
				if (player_visibility) {
					if (!ai_multiplayer_awareness(objP, 50))
						return;
					create_n_segment_path_to_door(objP, 8+gameStates.app.nDifficultyLevel, -1);     // third parameter is avoid_seg, -1 means avoid nothing.
					ai_multi_send_robot_position(objnum, -1);
				}
			}
			break;
		default:
			break;
	}

	if (aip->behavior == AIB_SNIPE) {
		if ((gameData.app.nGameMode & GM_MULTI) && !robptr->thief) {
			aip->behavior = AIB_NORMAL;
			ailp->mode = AIM_CHASE_OBJECT;
			return;
		}

		if (!(obj_ref & 3) || previous_visibility) {
			ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			// If this sniper is in still mode, if he was hit or can see player, switch to snipe mode.
			if (ailp->mode == AIM_STILL)
				if (player_visibility || (ailp->player_awareness_type == PA_WEAPON_ROBOT_COLLISION))
					ailp->mode = AIM_SNIPE_ATTACK;

			if (!robptr->thief && (ailp->mode != AIM_STILL))
				DoSnipeFrame(objP, dist_to_player, player_visibility, &vec_to_player);
		} else if (!robptr->thief && !robptr->companion)
			return;
	}

	// More special ability stuff, but based on a property of a robot, not its ID.
	if (robptr->companion) {

		ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
		DoEscortFrame(objP, dist_to_player, player_visibility);

		if (objP->ctype.ai_info.danger_laser_num != -1) {
			object *dObjP = &gameData.objs.objects[objP->ctype.ai_info.danger_laser_num];

			if ((dObjP->type == OBJ_WEAPON) && (dObjP->signature == objP->ctype.ai_info.danger_laser_signature)) {
				fix circle_distance;
				circle_distance = robptr->circle_distance[gameStates.app.nDifficultyLevel] + gameData.objs.console->size;
				ai_move_relative_to_player(objP, ailp, dist_to_player, &vec_to_player, circle_distance, 1, player_visibility);
			}
		}

		if (ready_to_fire(robptr, ailp)) {
			int do_stuff = 0;
			if (openable_doors_in_segment(objP->segnum) != -1)
				do_stuff = 1;
			else if (openable_doors_in_segment((short) gameData.ai.pointSegs[aip->hide_index + aip->cur_path_index + aip->PATH_DIR].segnum) != -1)
				do_stuff = 1;
			else if (openable_doors_in_segment((short) gameData.ai.pointSegs[aip->hide_index + aip->cur_path_index + 2*aip->PATH_DIR].segnum) != -1)
				do_stuff = 1;
			else if ((ailp->mode == AIM_GOTO_PLAYER) && (dist_to_player < 3*MIN_ESCORT_DISTANCE/2) ) {
				do_stuff = 1;
			} else
				; 

			if (do_stuff && (VmVecDot(&gameData.objs.console->orient.fvec, &vec_to_player) > -F1_0/4)) {
				CreateNewLaserEasy( &objP->orient.fvec, &objP->pos, OBJ_IDX (objP), FLARE_ID, 1);
				ailp->next_fire = F1_0/2;
				if (!gameData.escort.bMayTalk) // If buddy not talking, make him fire flares less often.
					ailp->next_fire += d_rand()*4;
			}

		}
	}

	if (robptr->thief) {

		ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
		DoThiefFrame(objP, dist_to_player, player_visibility, &vec_to_player);

		if (ready_to_fire(robptr, ailp)) {
			int do_stuff = 0;
			if (openable_doors_in_segment(objP->segnum) != -1)
				do_stuff = 1;
			else if (openable_doors_in_segment((short) gameData.ai.pointSegs[aip->hide_index + aip->cur_path_index + aip->PATH_DIR].segnum) != -1)
				do_stuff = 1;
			else if (openable_doors_in_segment((short) gameData.ai.pointSegs[aip->hide_index + aip->cur_path_index + 2*aip->PATH_DIR].segnum) != -1)
				do_stuff = 1;

			if (do_stuff) {
				// @mk, 05/08/95: Firing flare from center of object, this is dumb...
				CreateNewLaserEasy( &objP->orient.fvec, &objP->pos, OBJ_IDX (objP), FLARE_ID, 1);
				ailp->next_fire = F1_0/2;
				if (gameData.thief.nStolenItem == 0)     // If never stolen an item, fire flares less often (bad: gameData.thief.nStolenItem wraps, but big deal)
					ailp->next_fire += d_rand()*4;
			}
		}
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	switch (ailp->mode) {
		case AIM_CHASE_OBJECT: {        // chasing player, sort of, chase if far, back off if close, circle in between
			fix circle_distance;

			circle_distance = robptr->circle_distance[gameStates.app.nDifficultyLevel] + gameData.objs.console->size;
			// Green guy doesn't get his circle distance boosted, else he might never attack.
			if (robptr->attack_type != 1)
				circle_distance += (objnum&0xf) * F1_0/2;

			ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			// @mk, 12/27/94, structure here was strange.  Would do both clauses of what are now this if/then/else.  Used to be if/then, if/then.
			if ((player_visibility < 2) && (previous_visibility == 2)) { // this is redundant: mk, 01/15/95: && (ailp->mode == AIM_CHASE_OBJECT)) {
				if (!ai_multiplayer_awareness(objP, 53)) {
					if (maybe_ai_do_actual_firing_stuff(objP, aip))
						ai_do_actual_firing_stuff(objP, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
					return;
				}
				CreatePathToPlayer(objP, 8, 1);
				ai_multi_send_robot_position(objnum, -1);
			} else if ((player_visibility == 0) && (dist_to_player > F1_0*80) && (!(gameData.app.nGameMode & GM_MULTI))) {
				// If pretty far from the player, player cannot be seen
				// (obstructed) and in chase mode, switch to follow path mode.
				// This has one desirable benefit of avoiding physics retries.
				if (aip->behavior == AIB_STATION) {
					ailp->goal_segment = aip->hide_segment;
					create_path_to_station(objP, 15);
				} // -- this looks like a dumb thing to do...robots following paths far away from you! else create_n_segment_path(objP, 5, -1);
				break;
			}

			if ((aip->CURRENT_STATE == AIS_REST) && (aip->GOAL_STATE == AIS_REST)) {
				if (player_visibility) {
					if (d_rand() < gameData.time.xFrame*player_visibility) {
						if (dist_to_player/256 < d_rand()*player_visibility) {
							aip->GOAL_STATE = AIS_SRCH;
							aip->CURRENT_STATE = AIS_SRCH;
						}
					}
				}
			}

			if (gameData.time.xGame - ailp->time_player_seen > CHASE_TIME_LENGTH) {

				if (gameData.app.nGameMode & GM_MULTI)
					if (!player_visibility && (dist_to_player > F1_0*70)) {
						ailp->mode = AIM_STILL;
						return;
					}

				if (!ai_multiplayer_awareness(objP, 64)) {
					if (maybe_ai_do_actual_firing_stuff(objP, aip))
						ai_do_actual_firing_stuff(objP, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
					return;
				}
				// -- bad idea, robots charge player they've never seen! -- CreatePathToPlayer(objP, 10, 1);
				// -- bad idea, robots charge player they've never seen! -- ai_multi_send_robot_position(objnum, -1);
			} else if ((aip->CURRENT_STATE != AIS_REST) && (aip->GOAL_STATE != AIS_REST)) {
				if (!ai_multiplayer_awareness(objP, 70)) {
					if (maybe_ai_do_actual_firing_stuff(objP, aip))
						ai_do_actual_firing_stuff(objP, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
					return;
				}
				ai_move_relative_to_player(objP, ailp, dist_to_player, &vec_to_player, circle_distance, 0, player_visibility);

				if ((obj_ref & 1) && ((aip->GOAL_STATE == AIS_SRCH) || (aip->GOAL_STATE == AIS_LOCK))) {
					if (player_visibility) // == 2)
						ai_turn_towards_vector(&vec_to_player, objP, robptr->turn_time[gameStates.app.nDifficultyLevel]);
				}

				if (gameData.ai.bEvaded) {
					ai_multi_send_robot_position(objnum, 1);
					gameData.ai.bEvaded = 0;
				} else
					ai_multi_send_robot_position(objnum, -1);

				do_firing_stuff(objP, player_visibility, &vec_to_player);
			}
			break;
		}

		case AIM_RUN_FROM_OBJECT:
			ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			if (player_visibility) {
				if (ailp->player_awareness_type == 0)
					ailp->player_awareness_type = PA_WEAPON_ROBOT_COLLISION;

			}

			// If in multiplayer, only do if player visible.  If not multiplayer, do always.
			if (!(gameData.app.nGameMode & GM_MULTI) || player_visibility)
				if (ai_multiplayer_awareness(objP, 75)) {
					ai_follow_path(objP, player_visibility, previous_visibility, &vec_to_player);
					ai_multi_send_robot_position(objnum, -1);
				}

			if (aip->GOAL_STATE != AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;
			else if (aip->CURRENT_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;

			// Bad to let run_from robot fire at player because it
			// will cause a war in which it turns towards the player
			// to fire and then towards its goal to move.
			// do_firing_stuff(objP, player_visibility, &vec_to_player);
			// Instead, do this:
			// (Note, only drop if player is visible.  This prevents
			// the bombs from being a giveaway, and also ensures that
			// the robot is moving while it is dropping.  Also means
			// fewer will be dropped.)
			if ((ailp->next_fire <= 0) && (player_visibility)) {
				vms_vector fire_vec, fire_pos;

				if (!ai_multiplayer_awareness(objP, 75))
					return;

				fire_vec = objP->orient.fvec;
				VmVecNegate(&fire_vec);
				VmVecAdd(&fire_pos, &objP->pos, &fire_vec);

				if (aip->SUB_FLAGS & SUB_FLAGS_SPROX)
					CreateNewLaserEasy( &fire_vec, &fire_pos, OBJ_IDX (objP), ROBOT_SUPERPROX_ID, 1);
				else
					CreateNewLaserEasy( &fire_vec, &fire_pos, OBJ_IDX (objP), PROXIMITY_ID, 1);

				ailp->next_fire = (F1_0/2)*(NDL+5 - gameStates.app.nDifficultyLevel);      // Drop a proximity bomb every 5 seconds.

#ifdef NETWORK
#ifndef SHAREWARE
				if (gameData.app.nGameMode & GM_MULTI)
				{
					ai_multi_send_robot_position(OBJ_IDX (objP), -1);
					if (aip->SUB_FLAGS & SUB_FLAGS_SPROX)
						MultiSendRobotFire(OBJ_IDX (objP), -2, &fire_vec);
					else
						MultiSendRobotFire(OBJ_IDX (objP), -1, &fire_vec);
				}
#endif
#endif
			}
			break;

		case AIM_GOTO_PLAYER:
		case AIM_GOTO_OBJECT:
			ai_follow_path(objP, 2, previous_visibility, &vec_to_player);    // Follows path as if player can see robot.
			ai_multi_send_robot_position(objnum, -1);
			break;

		case AIM_FOLLOW_PATH: {
			int anger_level = 65;

			if (aip->behavior == AIB_STATION)
				if (gameData.ai.pointSegs[aip->hide_index + aip->path_length - 1].segnum == aip->hide_segment) {
					anger_level = 64;
				}

			ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			if (gameData.app.nGameMode & (GM_MODEM | GM_SERIAL))
				if (!player_visibility && (dist_to_player > F1_0*70)) {
					ailp->mode = AIM_STILL;
					return;
				}

			if (!ai_multiplayer_awareness(objP, anger_level)) {
				if (maybe_ai_do_actual_firing_stuff(objP, aip)) {
					ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
					ai_do_actual_firing_stuff(objP, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
				}
				return;
			}

			ai_follow_path(objP, player_visibility, previous_visibility, &vec_to_player);

			if (aip->GOAL_STATE != AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;
			else if (aip->CURRENT_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;

			if (aip->behavior != AIB_RUN_FROM)
				do_firing_stuff(objP, player_visibility, &vec_to_player);

			if ((player_visibility == 2) && (aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_FOLLOW) && (aip->behavior != AIB_RUN_FROM) && (objP->id != ROBOT_BRAIN) && (robptr->companion != 1) && (robptr->thief != 1)) {
				if (robptr->attack_type == 0)
					ailp->mode = AIM_CHASE_OBJECT;
				// This should not just be distance based, but also time-since-player-seen based.
			} else if ((dist_to_player > F1_0*(20*(2*gameStates.app.nDifficultyLevel + robptr->pursuit)))
						&& (gameData.time.xGame - ailp->time_player_seen > (F1_0/2*(gameStates.app.nDifficultyLevel+robptr->pursuit)))
						&& (player_visibility == 0)
						&& (aip->behavior == AIB_NORMAL)
						&& (ailp->mode == AIM_FOLLOW_PATH)) {
				ailp->mode = AIM_STILL;
				aip->hide_index = -1;
				aip->path_length = 0;
			}

			ai_multi_send_robot_position(objnum, -1);

			break;
		}

		case AIM_BEHIND:
			if (!ai_multiplayer_awareness(objP, 71)) {
				if (maybe_ai_do_actual_firing_stuff(objP, aip)) {
					ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
					ai_do_actual_firing_stuff(objP, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
				}
				return;
			}

			ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			if (player_visibility == 2) {
				// Get behind the player.
				// Method:
				// If vec_to_player dot player_rear_vector > 0, behind is goal.
				// Else choose goal with larger dot from left, right.
				vms_vector  goal_point, goal_vector, vec_to_goal, rand_vec;
				fix         dot;

				dot = VmVecDot(&gameData.objs.console->orient.fvec, &vec_to_player);
				if (dot > 0) {          // Remember, we're interested in the rear vector dot being < 0.
					goal_vector = gameData.objs.console->orient.fvec;
					VmVecNegate(&goal_vector);
				} else {
					fix dot;
					dot = VmVecDot(&gameData.objs.console->orient.rvec, &vec_to_player);
					goal_vector = gameData.objs.console->orient.rvec;
					if (dot > 0) {
						VmVecNegate(&goal_vector);
					} else
						;
				}

				VmVecScale(&goal_vector, 2*(gameData.objs.console->size + objP->size + (((objnum*4 + gameData.app.nFrameCount) & 63) << 12)));
				VmVecAdd(&goal_point, &gameData.objs.console->pos, &goal_vector);
				MakeRandomVector(&rand_vec);
				VmVecScaleInc(&goal_point, &rand_vec, F1_0*8);
				VmVecSub(&vec_to_goal, &goal_point, &objP->pos);
				VmVecNormalizeQuick(&vec_to_goal);
				move_towards_vector(objP, &vec_to_goal, 0);
				ai_turn_towards_vector(&vec_to_player, objP, robptr->turn_time[gameStates.app.nDifficultyLevel]);
				ai_do_actual_firing_stuff(objP, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
			}

			if (aip->GOAL_STATE != AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;
			else if (aip->CURRENT_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;

			ai_multi_send_robot_position(objnum, -1);
			break;

		case AIM_STILL:
			if ((dist_to_player < F1_0*120+gameStates.app.nDifficultyLevel*F1_0*20) || (ailp->player_awareness_type >= PA_WEAPON_ROBOT_COLLISION-1)) {
				ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

				// turn towards vector if visible this time or last time, or rand
				// new!
				if ((player_visibility == 2) || (previous_visibility == 2)) { // -- MK, 06/09/95:  || ((d_rand() > 0x4000) && !(gameData.app.nGameMode & GM_MULTI))) {
					if (!ai_multiplayer_awareness(objP, 71)) {
						if (maybe_ai_do_actual_firing_stuff(objP, aip))
							ai_do_actual_firing_stuff(objP, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
						return;
					}
					ai_turn_towards_vector(&vec_to_player, objP, robptr->turn_time[gameStates.app.nDifficultyLevel]);
					ai_multi_send_robot_position(objnum, -1);
				}

				do_firing_stuff(objP, player_visibility, &vec_to_player);
				if (player_visibility == 2) {  // Changed @mk, 09/21/95: Require that they be looking to evade.  Change, MK, 01/03/95 for Multiplayer reasons.  If robots can't see you (even with eyes on back of head), then don't do evasion.
					if (robptr->attack_type == 1) {
						aip->behavior = AIB_NORMAL;
						if (!ai_multiplayer_awareness(objP, 80)) {
							if (maybe_ai_do_actual_firing_stuff(objP, aip))
								ai_do_actual_firing_stuff(objP, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
							return;
						}
						ai_move_relative_to_player(objP, ailp, dist_to_player, &vec_to_player, 0, 0, player_visibility);
						if (gameData.ai.bEvaded) {
							ai_multi_send_robot_position(objnum, 1);
							gameData.ai.bEvaded = 0;
						}
						else
							ai_multi_send_robot_position(objnum, -1);
					} else {
						// Robots in hover mode are allowed to evade at half normal speed.
						if (!ai_multiplayer_awareness(objP, 81)) {
							if (maybe_ai_do_actual_firing_stuff(objP, aip))
								ai_do_actual_firing_stuff(objP, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
							return;
						}
						ai_move_relative_to_player(objP, ailp, dist_to_player, &vec_to_player, 0, 1, player_visibility);
						if (gameData.ai.bEvaded) {
							ai_multi_send_robot_position(objnum, -1);
							gameData.ai.bEvaded = 0;
						}
						else
							ai_multi_send_robot_position(objnum, -1);
					}
				} else if ((objP->segnum != aip->hide_segment) && (dist_to_player > F1_0*80) && (!(gameData.app.nGameMode & GM_MULTI))) {
					// If pretty far from the player, player cannot be
					// seen (obstructed) and in chase mode, switch to
					// follow path mode.
					// This has one desirable benefit of avoiding physics retries.
					if (aip->behavior == AIB_STATION) {
						ailp->goal_segment = aip->hide_segment;
						create_path_to_station(objP, 15);
					}
					break;
				}
			}

			break;
		case AIM_OPEN_DOOR: {       // trying to open a door.
			vms_vector center_point, goal_vector;
			Assert(objP->id == ROBOT_BRAIN);     // Make sure this guy is allowed to be in this mode.

			if (!ai_multiplayer_awareness(objP, 62))
				return;
			COMPUTE_SIDE_CENTER(&center_point, &gameData.segs.segments[objP->segnum], aip->GOALSIDE);
			VmVecSub(&goal_vector, &center_point, &objP->pos);
			VmVecNormalizeQuick(&goal_vector);
			ai_turn_towards_vector(&goal_vector, objP, robptr->turn_time[gameStates.app.nDifficultyLevel]);
			move_towards_vector(objP, &goal_vector, 0);
			ai_multi_send_robot_position(objnum, -1);

			break;
		}

		case AIM_SNIPE_WAIT:
			break;
		case AIM_SNIPE_RETREAT:
			// -- if (ai_multiplayer_awareness(objP, 53))
			// -- 	if (ailp->next_fire < -F1_0)
			// -- 		ai_do_actual_firing_stuff(objP, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
			break;
		case AIM_SNIPE_RETREAT_BACKWARDS:
		case AIM_SNIPE_ATTACK:
		case AIM_SNIPE_FIRE:
			if (ai_multiplayer_awareness(objP, 53)) {
				ai_do_actual_firing_stuff(objP, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
				if (robptr->thief)
					ai_move_relative_to_player(objP, ailp, dist_to_player, &vec_to_player, 0, 0, player_visibility);
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
	// Assert(player_visibility != -1); // Means it didn't get initialized!
	ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
	if ((player_visibility == 2) && (aip->behavior != AIB_FOLLOW) && (!robptr->thief)) {
		if ((ailp->player_awareness_type == 0) && (aip->SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE))
			aip->SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		else if (ailp->player_awareness_type == 0)
			ailp->player_awareness_type = PA_PLAYER_COLLISION;
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	if (!object_animates) {
		aip->CURRENT_STATE = aip->GOAL_STATE;
	}

	Assert(ailp->player_awareness_type <= AIE_MAX);
	Assert(aip->CURRENT_STATE < AIS_MAX);
	Assert(aip->GOAL_STATE < AIS_MAX);

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	if (ailp->player_awareness_type) {
		new_goal_state = Ai_transition_table[ailp->player_awareness_type-1][aip->CURRENT_STATE][aip->GOAL_STATE];
		if (new_goal_state == 6)
			new_goal_state = 6;
		if (ailp->player_awareness_type == PA_WEAPON_ROBOT_COLLISION) {
			// Decrease awareness, else this robot will flinch every frame.
			ailp->player_awareness_type--;
			ailp->player_awareness_time = F1_0*3;
		}

		if (new_goal_state == AIS_ERR_)
			new_goal_state = AIS_REST;

		if (aip->CURRENT_STATE == AIS_NONE)
			aip->CURRENT_STATE = AIS_REST;

		aip->GOAL_STATE = new_goal_state;

	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// If new state = fire, then set all gun states to fire.
	if ((aip->GOAL_STATE == AIS_FIRE) ) {
		int i,num_guns;
		num_guns = gameData.bots.pInfo[objP->id].n_guns;
		for (i=0; i<num_guns; i++)
			ailp->goal_state[i] = AIS_FIRE;
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// Hack by mk on 01/04/94, if a guy hasn't animated to the firing state, but his next_fire says ok to fire, bash him there
	if (ready_to_fire(robptr, ailp) && (aip->GOAL_STATE == AIS_FIRE))
		aip->CURRENT_STATE = AIS_FIRE;

	if ((aip->GOAL_STATE != AIS_FLIN)  && (objP->id != ROBOT_BRAIN)) {
		switch (aip->CURRENT_STATE) {
		case AIS_NONE:
			ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			dot = VmVecDot(&objP->orient.fvec, &vec_to_player);
			if (dot >= F1_0/2)
				if (aip->GOAL_STATE == AIS_REST)
					aip->GOAL_STATE = AIS_SRCH;
			break;
		case AIS_REST:
			if (aip->GOAL_STATE == AIS_REST) {
				ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
				if (ready_to_fire(robptr, ailp) && (player_visibility)) {
					aip->GOAL_STATE = AIS_FIRE;
				}
			}
			break;
		case AIS_SRCH:
			if (!ai_multiplayer_awareness(objP, 60))
				return;

			ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			if (player_visibility == 2) {
				ai_turn_towards_vector(&vec_to_player, objP, robptr->turn_time[gameStates.app.nDifficultyLevel]);
				ai_multi_send_robot_position(objnum, -1);
			}
			break;
		case AIS_LOCK:
			ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			if (!(gameData.app.nGameMode & GM_MULTI) || (player_visibility)) {
				if (!ai_multiplayer_awareness(objP, 68))
					return;

				if (player_visibility == 2) {   // @mk, 09/21/95, require that they be looking towards you to turn towards you.
					ai_turn_towards_vector(&vec_to_player, objP, robptr->turn_time[gameStates.app.nDifficultyLevel]);
					ai_multi_send_robot_position(objnum, -1);
				}
			}
			break;
		case AIS_FIRE:
			ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			if (player_visibility == 2) {
				if (!ai_multiplayer_awareness(objP, (ROBOT_FIRE_AGITATION-1))) {
					if (gameData.app.nGameMode & GM_MULTI) {
						ai_do_actual_firing_stuff(objP, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
						return;
					}
				}
				ai_turn_towards_vector(&vec_to_player, objP, robptr->turn_time[gameStates.app.nDifficultyLevel]);
				ai_multi_send_robot_position(objnum, -1);
			}

			// Fire at player, if appropriate.
			ai_do_actual_firing_stuff(objP, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);

			break;
		case AIS_RECO:
			if (!(obj_ref & 3)) {
				ComputeVisAndVec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
				if (player_visibility == 2) {
					if (!ai_multiplayer_awareness(objP, 69))
						return;
					ai_turn_towards_vector(&vec_to_player, objP, robptr->turn_time[gameStates.app.nDifficultyLevel]);
					ai_multi_send_robot_position(objnum, -1);
				} // -- MK, 06/09/95: else if (!(gameData.app.nGameMode & GM_MULTI)) {
			}
			break;
		case AIS_FLIN:
			break;
		default:
#if TRACE	
			con_printf (1, "Unknown mode for AI object #%i\n", objnum);
#endif
			aip->GOAL_STATE = AIS_REST;
			aip->CURRENT_STATE = AIS_REST;
			break;
		}
	} // end of: if (aip->GOAL_STATE != AIS_FLIN) {

	// Switch to next gun for next fire.
	if (player_visibility == 0) {
		aip->CURRENT_GUN++;
		if (aip->CURRENT_GUN >= gameData.bots.pInfo[objP->id].n_guns)
		{
			if ((robptr->n_guns == 1) || (robptr->weapon_type2 == -1))  // Two weapon types hack.
				aip->CURRENT_GUN = 0;
			else
				aip->CURRENT_GUN = 1;
		}
	}
//HUDInitMessage ("%d %d %d", aip->flags [1], aip->flags [2], dist_to_player / F1_0);
}

// ----------------------------------------------------------------------------
void AIDoCloakStuff(void)
{
	int i;

	for (i=0; i<MAX_AI_CLOAK_INFO; i++) {
		gameData.ai.cloakInfo[i].last_position = gameData.objs.console->pos;
		gameData.ai.cloakInfo[i].last_segment = gameData.objs.console->segnum;
		gameData.ai.cloakInfo[i].last_time = gameData.time.xGame;
	}

	// Make work for control centers.
	gameData.ai.vBelievedPlayerPos = gameData.ai.cloakInfo[0].last_position;
	gameData.ai.nBelievedPlayerSeg = gameData.ai.cloakInfo[0].last_segment;

}

// ----------------------------------------------------------------------------
// Returns false if awareness is considered too puny to add, else returns true.
int add_awareness_event(object *objP, int type)
{
	// If player cloaked and hit a robot, then increase awareness
	if ((type == PA_WEAPON_ROBOT_COLLISION) || (type == PA_WEAPON_WALL_COLLISION) || (type == PA_PLAYER_COLLISION))
		AIDoCloakStuff();

	if (gameData.ai.nAwarenessEvents < MAX_AWARENESS_EVENTS) {
		if ((type == PA_WEAPON_WALL_COLLISION) || (type == PA_WEAPON_ROBOT_COLLISION))
			if (objP->id == VULCAN_ID)
				if (d_rand() > 3276)
					return 0;       // For vulcan cannon, only about 1/10 actually cause awareness

		gameData.ai.awarenessEvents[gameData.ai.nAwarenessEvents].segnum = objP->segnum;
		gameData.ai.awarenessEvents[gameData.ai.nAwarenessEvents].pos = objP->pos;
		gameData.ai.awarenessEvents[gameData.ai.nAwarenessEvents].type = type;
		gameData.ai.nAwarenessEvents++;
	} else {
		//Int3();   // Hey -- Overflowed gameData.ai.awarenessEvents, make more or something
		// This just gets ignored, so you can just
		// continue.
	}
	return 1;

}

// ----------------------------------------------------------------------------------
// Robots will become aware of the player based on something that occurred.
// The object (probably player or weapon) which created the awareness is objP.
void CreateAwarenessEvent(object *objP, int type)
{
	// If not in multiplayer, or in multiplayer with robots, do this, else unnecessary!
	if (!(gameData.app.nGameMode & GM_MULTI) || (gameData.app.nGameMode & GM_MULTI_ROBOTS)) {
		if (add_awareness_event(objP, type)) {
			if (((d_rand() * (type+4)) >> 15) > 4)
				gameData.ai.nOverallAgitation++;
			if (gameData.ai.nOverallAgitation > OVERALL_AGITATION_MAX)
				gameData.ai.nOverallAgitation = OVERALL_AGITATION_MAX;
		}
	}
}

sbyte New_awareness[MAX_SEGMENTS];

// ----------------------------------------------------------------------------------
void pae_aux(int segnum, int type, int level)
{
	int j;

	if (New_awareness[segnum] < type)
		New_awareness[segnum] = type;

	// Process children.
	for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
		if (IS_CHILD(gameData.segs.segments[segnum].children[j]))
		{
			if (level <= 3)
			{
				if (type == 4)
					pae_aux(gameData.segs.segments[segnum].children[j], type-1, level+1);
				else
					pae_aux(gameData.segs.segments[segnum].children[j], type, level+1);
			}
		}
}


// ----------------------------------------------------------------------------------
void process_awareness_events(void)
{
	int i;

	if (!(gameData.app.nGameMode & GM_MULTI) || (gameData.app.nGameMode & GM_MULTI_ROBOTS)) {
		memset(New_awareness, 0, sizeof(New_awareness[0]) * (gameData.segs.nLastSegment+1));

		for (i=0; i<gameData.ai.nAwarenessEvents; i++)
			pae_aux(gameData.ai.awarenessEvents[i].segnum, gameData.ai.awarenessEvents[i].type, 1);

	}

	gameData.ai.nAwarenessEvents = 0;
}

// ----------------------------------------------------------------------------------
void set_player_awareness_all(void)
{
	int i;

	process_awareness_events();

	for (i=0; i<=gameData.objs.nLastObject; i++)
		if (gameData.objs.objects[i].control_type == CT_AI) {
			if (New_awareness[gameData.objs.objects[i].segnum] > gameData.ai.localInfo[i].player_awareness_type) {
				gameData.ai.localInfo[i].player_awareness_type = New_awareness[gameData.objs.objects[i].segnum];
				gameData.ai.localInfo[i].player_awareness_time = PLAYER_AWARENESS_INITIAL_TIME;
			}

			// Clear the bit that says this robot is only awake because a camera woke it up.
			if (New_awareness[gameData.objs.objects[i].segnum] > gameData.ai.localInfo[i].player_awareness_type)
				gameData.objs.objects[i].ctype.ai_info.SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		}
}

#ifndef NDEBUG
int Ai_dump_enable = 0;

FILE *Ai_dump_file = NULL;

char Ai_error_message[128] = "";

// ----------------------------------------------------------------------------------
void force_dump_ai_objects_all(char *msg)
{
	int tsave;

	tsave = Ai_dump_enable;

	Ai_dump_enable = 1;

	sprintf(Ai_error_message, "%s\n", msg);
	//dump_ai_objects_all();
	Ai_error_message[0] = 0;

	Ai_dump_enable = tsave;
}

// ----------------------------------------------------------------------------------
void turn_off_ai_dump(void)
{
	if (Ai_dump_file != NULL)
		fclose(Ai_dump_file);

	Ai_dump_file = NULL;
}

#endif

extern void do_boss_dying_frame(object *objP);

// ----------------------------------------------------------------------------------
// Do things which need to get done for all AI gameData.objs.objects each frame.
// This includes:
//  Setting player_awareness (a fix, time in seconds which object is aware of player)
void DoAIFrameAll(void)
{
#ifndef NDEBUG
	//dump_ai_objects_all();
#endif

	set_player_awareness_all();

	if (nAiLastMissileCamera != -1) {
		// Clear if supposed misisle camera is not a weapon, or just every so often, just in case.
		if (((gameData.app.nFrameCount & 0x0f) == 0) || (gameData.objs.objects[nAiLastMissileCamera].type != OBJ_WEAPON)) {
			int i;

			nAiLastMissileCamera = -1;
			for (i=0; i<=gameData.objs.nLastObject; i++)
				if (gameData.objs.objects[i].type == OBJ_ROBOT)
					gameData.objs.objects[i].ctype.ai_info.SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		}
	}

	// (Moved here from do_boss_stuff() because that only gets called if robot aware of player.)
	if (gameData.boss.nDying) {
		if (gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses)
			do_boss_dying_frame(gameData.objs.objects + gameData.boss.nDying);
		else {
			int i;
			object *objP = gameData.objs.objects;

			for (i=0; i<=gameData.objs.nLastObject; i++, objP++)
				if ((objP->type == OBJ_ROBOT) && (gameData.bots.pInfo[objP->id].boss_flag))
					do_boss_dying_frame(objP);
		}
	}
}


//	-------------------------------------------------------------------------------------------------
extern fix Boss_invulnerable_dot;

// Initializations to be performed for all robots for a new level.
void InitRobotsForLevel(void)
{
	gameData.ai.nOverallAgitation = 0;
	gameStates.gameplay.bFinalBossIsDead=0;

	gameData.escort.nObjNum = 0;
	gameData.escort.bMayTalk = 0;

	Boss_invulnerable_dot = F1_0/4 - i2f(gameStates.app.nDifficultyLevel)/8;
	gameData.boss.nDyingStartTime = 0;
}

//	-------------------------------------------------------------------------------------------------

int AISaveBinState (CFILE *fp)
{
	int	i;

	CFWrite(&gameData.ai.bInitialized, sizeof(int), 1, fp);
	CFWrite(&gameData.ai.nOverallAgitation, sizeof(int), 1, fp);
	CFWrite(gameData.ai.localInfo, sizeof(ai_local), MAX_OBJECTS, fp);
	CFWrite(gameData.ai.pointSegs, sizeof(point_seg), MAX_POINT_SEGS, fp);
	CFWrite(gameData.ai.cloakInfo, sizeof(ai_cloak_info), MAX_AI_CLOAK_INFO, fp);
	CFWrite(&gameData.boss.nCloakStartTime, sizeof(fix), 1, fp);
	CFWrite(&gameData.boss.nCloakEndTime , sizeof(fix), 1, fp);
	CFWrite(&gameData.boss.nLastTeleportTime , sizeof(fix), 1, fp);
	CFWrite(&gameData.boss.nTeleportInterval, sizeof(fix), 1, fp);
	CFWrite(&gameData.boss.nCloakInterval, sizeof(fix), 1, fp);
	CFWrite(&gameData.boss.nCloakDuration, sizeof(fix), 1, fp);
	CFWrite(&gameData.boss.nLastGateTime, sizeof(fix), 1, fp);
	CFWrite(&gameData.boss.nGateInterval, sizeof(fix), 1, fp);
	CFWrite(&gameData.boss.nDyingStartTime, sizeof(fix), 1, fp);
	CFWrite(&gameData.boss.nDying, sizeof(int), 1, fp);
	CFWrite(&gameData.boss.bDyingSoundPlaying, sizeof(int), 1, fp);
	CFWrite(&gameData.boss.nHitTime, sizeof(fix), 1, fp);
	// -- MK, 10/21/95, unused! -- CFWrite( &Boss_been_hit, sizeof(int), 1, fp);

	CFWrite(&gameData.escort.nKillObject, sizeof(gameData.escort.nKillObject), 1, fp);
	CFWrite(&gameData.escort.xLastPathCreated, sizeof(gameData.escort.xLastPathCreated), 1, fp);
	CFWrite(&gameData.escort.nGoalObject, sizeof(gameData.escort.nGoalObject), 1, fp);
	CFWrite(&gameData.escort.nSpecialGoal, sizeof(gameData.escort.nSpecialGoal), 1, fp);
	CFWrite(&gameData.escort.nGoalIndex, sizeof(gameData.escort.nGoalIndex), 1, fp);
	CFWrite(&gameData.thief.stolenItems, sizeof(gameData.thief.stolenItems[0]), MAX_STOLEN_ITEMS, fp);

	{ 
	int temp;
	temp = (int) (gameData.ai.freePointSegs - gameData.ai.pointSegs);
	CFWrite(&temp, sizeof(int), 1, fp);
	}

	CFWrite(gameData.boss.nTeleportSegs, sizeof(gameData.boss.nTeleportSegs), 1, fp);
	CFWrite(gameData.boss.nGateSegs, sizeof(gameData.boss.nGateSegs), 1, fp);
	for (i = 0; i < MAX_BOSS_COUNT; i++) {
		if (gameData.boss.nGateSegs [i])
			CFWrite(gameData.boss.gateSegs [i], sizeof(gameData.boss.gateSegs[i][0]), gameData.boss.nGateSegs [i], fp);
		if (gameData.boss.nTeleportSegs [i])
			CFWrite(gameData.boss.teleportSegs [i], sizeof(gameData.boss.teleportSegs[i][0]), gameData.boss.nTeleportSegs [i], fp);
		}

	return 1;
}

//	-------------------------------------------------------------------------------------------------

int AIRestoreBinState (CFILE *fp, int version)
{
	int	i;

	memset(gameData.ai.localInfo, 0, sizeof(ai_local));
	memset(gameData.ai.pointSegs, 0, sizeof(point_seg));
	CFRead(&gameData.ai.bInitialized, sizeof(int), 1, fp);
	CFRead(&gameData.ai.nOverallAgitation, sizeof(int), 1, fp);
	CFRead(gameData.ai.localInfo, sizeof(ai_local), (version > 22) ? MAX_OBJECTS : MAX_OBJECTS_D2, fp);
	CFRead(gameData.ai.pointSegs, sizeof(point_seg), (version > 22) ? MAX_POINT_SEGS : MAX_POINT_SEGS_D2, fp);
	CFRead(gameData.ai.cloakInfo, sizeof(ai_cloak_info), MAX_AI_CLOAK_INFO, fp);
	CFRead(&gameData.boss.nCloakStartTime, sizeof(fix), 1, fp);
	CFRead(&gameData.boss.nCloakEndTime , sizeof(fix), 1, fp);
	CFRead(&gameData.boss.nLastTeleportTime , sizeof(fix), 1, fp);
	CFRead(&gameData.boss.nTeleportInterval, sizeof(fix), 1, fp);
	CFRead(&gameData.boss.nCloakInterval, sizeof(fix), 1, fp);
	CFRead(&gameData.boss.nCloakDuration, sizeof(fix), 1, fp);
	CFRead(&gameData.boss.nLastGateTime, sizeof(fix), 1, fp);
	CFRead(&gameData.boss.nGateInterval, sizeof(fix), 1, fp);
	CFRead(&gameData.boss.nDyingStartTime, sizeof(fix), 1, fp);
	CFRead(&gameData.boss.nDying, sizeof(int), 1, fp);
	CFRead(&gameData.boss.bDyingSoundPlaying, sizeof(int), 1, fp);
	CFRead(&gameData.boss.nHitTime, sizeof(fix), 1, fp);
	// -- MK, 10/21/95, unused! -- CFRead(&Boss_been_hit, sizeof(int), 1, fp);

	if (version >= 8) {
		CFRead(&gameData.escort.nKillObject, sizeof(gameData.escort.nKillObject), 1, fp);
		CFRead(&gameData.escort.xLastPathCreated, sizeof(gameData.escort.xLastPathCreated), 1, fp);
		CFRead(&gameData.escort.nGoalObject, sizeof(gameData.escort.nGoalObject), 1, fp);
		CFRead(&gameData.escort.nSpecialGoal, sizeof(gameData.escort.nSpecialGoal), 1, fp);
		CFRead(&gameData.escort.nGoalIndex, sizeof(gameData.escort.nGoalIndex), 1, fp);
		CFRead(&gameData.thief.stolenItems, sizeof(gameData.thief.stolenItems[0]), MAX_STOLEN_ITEMS, fp);
	} else {
		int i;

		gameData.escort.nKillObject = -1;
		gameData.escort.xLastPathCreated = 0;
		gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
		gameData.escort.nSpecialGoal = -1;
		gameData.escort.nGoalIndex = -1;

		for (i=0; i<MAX_STOLEN_ITEMS; i++) {
			gameData.thief.stolenItems[i] = 255;
		}

	}

	if (version >= 15) {
		int temp;
		CFRead(&temp, sizeof(int), 1, fp);
		gameData.ai.freePointSegs = gameData.ai.pointSegs + temp;
	} else
		AIResetAllPaths();

	if (version >= 24) {
		CFRead(gameData.boss.nTeleportSegs, sizeof(gameData.boss.nTeleportSegs), 1, fp);
		CFRead(gameData.boss.nGateSegs, sizeof(gameData.boss.nGateSegs), 1, fp);
		for (i = 0; i < MAX_BOSS_COUNT; i++) {
			if (gameData.boss.nGateSegs [i])
				CFRead(&gameData.boss.gateSegs [i][0], sizeof(gameData.boss.gateSegs [i][0]), gameData.boss.nGateSegs [i], fp);
			if (gameData.boss.nTeleportSegs [i])
				CFRead(&gameData.boss.teleportSegs [i][0], sizeof(gameData.boss.teleportSegs [i][0]), gameData.boss.nTeleportSegs [i], fp);
			}
		}
	else if (version >= 21) {
		CFRead(&gameData.boss.nTeleportSegs [0], sizeof(gameData.boss.nTeleportSegs [0]), 1, fp);
		CFRead(&gameData.boss.nGateSegs [0], sizeof(gameData.boss.nGateSegs [0]), 1, fp);

		if (gameData.boss.nGateSegs [0])
			CFRead(gameData.boss.gateSegs[0], sizeof(gameData.boss.gateSegs[0][0]), gameData.boss.nGateSegs [0], fp);

		if (gameData.boss.nTeleportSegs [0])
			CFRead(gameData.boss.teleportSegs[0], sizeof(gameData.boss.teleportSegs[0][0]), gameData.boss.nTeleportSegs [0], fp);
	} else {
		// -- gameData.boss.nTeleportSegs = 1;
		// -- gameData.boss.nGateSegs = 1;
		// -- gameData.boss.teleportSegs[0] = 0;
		// -- gameData.boss.gateSegs[0] = 0;
		// Note: Maybe better to leave alone...will probably be ok.
#if TRACE	
		con_printf (1, "Warning: If you fight the boss, he might teleport to segment #0!\n");
#endif
	}

	return 1;
}
//	-------------------------------------------------------------------------------------------------

void AISaveLocalInfo (ai_local *ailP, CFILE *fp)
{
	int	i;

CFWriteInt (ailP->player_awareness_type, fp); 
CFWriteInt (ailP->retry_count, fp);           
CFWriteInt (ailP->consecutive_retries, fp);   
CFWriteInt (ailP->mode, fp);                  
CFWriteInt (ailP->previous_visibility, fp);   
CFWriteInt (ailP->rapidfire_count, fp);       
CFWriteInt (ailP->goal_segment, fp);          
CFWriteFix (ailP->next_action_time, fp); 
CFWriteFix (ailP->next_fire, fp);        
CFWriteFix (ailP->next_fire2, fp);       
CFWriteFix (ailP->player_awareness_time, fp);
CFWriteFix (ailP->time_player_seen, fp);     
CFWriteFix (ailP->time_player_sound_attacked, fp);
CFWriteFix (ailP->next_misc_sound_time, fp);      
CFWriteFix (ailP->time_since_processed, fp);      
for (i = 0; i < MAX_SUBMODELS; i++) {
	CFWriteAngVec (ailP->goal_angles + i, fp);  
	CFWriteAngVec (ailP->delta_angles + i, fp);
	}
CFWrite (ailP->goal_state, sizeof (ailP->goal_state [0]), 1, fp);
CFWrite (ailP->achieved_state, sizeof (ailP->achieved_state[0]), 1, fp);
}

//	-------------------------------------------------------------------------------------------------

void AISavePointSeg (point_seg *psegP, CFILE *fp)
{
CFWriteInt (psegP->segnum, fp);
CFWriteVector (&psegP->point, fp);
}

//	-------------------------------------------------------------------------------------------------

void AISaveCloakInfo (ai_cloak_info *ciP, CFILE *fp)
{
CFWriteFix (ciP->last_time, fp);
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
CFWriteFix (gameData.boss.nCloakStartTime, fp);
CFWriteFix (gameData.boss.nCloakEndTime , fp);
CFWriteFix (gameData.boss.nLastTeleportTime , fp);
CFWriteFix (gameData.boss.nTeleportInterval, fp);
CFWriteFix (gameData.boss.nCloakInterval, fp);
CFWriteFix (gameData.boss.nCloakDuration, fp);
CFWriteFix (gameData.boss.nLastGateTime, fp);
CFWriteFix (gameData.boss.nGateInterval, fp);
CFWriteFix (gameData.boss.nDyingStartTime, fp);
CFWriteInt (gameData.boss.nDying, fp);
CFWriteInt (gameData.boss.bDyingSoundPlaying, fp);
CFWriteFix (gameData.boss.nHitTime, fp);
CFWriteInt (gameData.escort.nKillObject, fp);
CFWriteFix (gameData.escort.xLastPathCreated, fp);
CFWriteInt (gameData.escort.nGoalObject, fp);
CFWriteInt (gameData.escort.nSpecialGoal, fp);
CFWriteInt (gameData.escort.nGoalIndex, fp);
CFWrite (gameData.thief.stolenItems, sizeof (gameData.thief.stolenItems [0]), MAX_STOLEN_ITEMS, fp);
CFWriteInt ((int) (gameData.ai.freePointSegs - gameData.ai.pointSegs), fp);
for (i = 0; i < MAX_BOSS_COUNT; i++) {
	CFWriteShort (gameData.boss.nTeleportSegs [i], fp);
	CFWriteShort (gameData.boss.nGateSegs [i], fp);
	}
for (i = 0; i < MAX_BOSS_COUNT; i++) {
	if (h = gameData.boss.nGateSegs [i])
		for (j = 0; j < h; j++)
			CFWriteShort (gameData.boss.gateSegs [i][j], fp);
	if (h = gameData.boss.nTeleportSegs [i])
		for (j = 0; j < h; j++)
			CFWriteShort (gameData.boss.teleportSegs [i][j], fp);
	}
return 1;
}

//	-------------------------------------------------------------------------------------------------

void AIRestoreLocalInfo (ai_local *ailP, CFILE *fp)
{
	int	i;

ailP->player_awareness_type = CFReadInt (fp); 
ailP->retry_count = CFReadInt (fp);           
ailP->consecutive_retries = CFReadInt (fp);   
ailP->mode = CFReadInt (fp);                  
ailP->previous_visibility = CFReadInt (fp);   
ailP->rapidfire_count = CFReadInt (fp);       
ailP->goal_segment = CFReadInt (fp);          
ailP->next_action_time = CFReadFix (fp); 
ailP->next_fire = CFReadFix (fp);        
ailP->next_fire2 = CFReadFix (fp);       
ailP->player_awareness_time = CFReadFix (fp);
ailP->time_player_seen = CFReadFix (fp);     
ailP->time_player_sound_attacked = CFReadFix (fp);
ailP->next_misc_sound_time = CFReadFix (fp);      
ailP->time_since_processed = CFReadFix (fp);      
for (i = 0; i < MAX_SUBMODELS; i++) {
	CFReadAngVec (ailP->goal_angles + i, fp);  
	CFReadAngVec (ailP->delta_angles + i, fp);
	}
CFRead (ailP->goal_state, sizeof (ailP->goal_state [0]), 1, fp);
CFRead (ailP->achieved_state, sizeof (ailP->achieved_state [0]), 1, fp);
}

//	-------------------------------------------------------------------------------------------------

void AIRestorePointSeg (point_seg *psegP, CFILE *fp)
{
psegP->segnum = CFReadInt (fp);
CFReadVector (&psegP->point, fp);
}

//	-------------------------------------------------------------------------------------------------

void AIRestoreCloakInfo (ai_cloak_info *ciP, CFILE *fp)
{
ciP->last_time = CFReadFix (fp);
ciP->last_segment = CFReadInt (fp);
CFReadVector (&ciP->last_position, fp);
}

//	-------------------------------------------------------------------------------------------------

int AIRestoreUniState (CFILE *fp, int version)
{
	int	h, i, j, nMaxBossCount;

memset(gameData.ai.localInfo, 0, sizeof (gameData.ai.localInfo));
memset(gameData.ai.pointSegs, 0, sizeof (gameData.ai.pointSegs));
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
gameData.boss.nCloakStartTime = CFReadFix (fp);
gameData.boss.nCloakEndTime = CFReadFix (fp);
gameData.boss.nLastTeleportTime = CFReadFix (fp);
gameData.boss.nTeleportInterval = CFReadFix (fp);
gameData.boss.nCloakInterval = CFReadFix (fp);
gameData.boss.nCloakDuration = CFReadFix (fp);
gameData.boss.nLastGateTime = CFReadFix (fp);
gameData.boss.nGateInterval = CFReadFix (fp);
gameData.boss.nDyingStartTime = CFReadFix (fp);
gameData.boss.nDying = CFReadInt (fp);
gameData.boss.bDyingSoundPlaying = CFReadInt (fp);
gameData.boss.nHitTime = CFReadFix (fp);
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
	AIResetAllPaths();

if (version < 21) {
	#if TRACE	
	con_printf (1, "Warning: If you fight the boss, he might teleport to segment #0!\n");
	#endif
	}
else {
	nMaxBossCount = (version >= 24) ? MAX_BOSS_COUNT : 1;
	for (i = 0; i < MAX_BOSS_COUNT; i++) {
		gameData.boss.nTeleportSegs [i] = CFReadShort (fp);
		gameData.boss.nGateSegs [i] = CFReadShort (fp);
		}
	for (i = 0; i < MAX_BOSS_COUNT; i++) {
		if (h = gameData.boss.nGateSegs [i])
			for (j = 0; j < h; j++)
				gameData.boss.gateSegs [i][j] = CFReadShort (fp);
		if (h = gameData.boss.nTeleportSegs [i])
			for (j = 0; j < h; j++)
				gameData.boss.teleportSegs [i][j] = CFReadShort (fp);
		}
	}
return 1;
}
//	-------------------------------------------------------------------------------------------------
