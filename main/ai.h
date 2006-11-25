/* $Id: ai.h,v 1.7 2003/10/04 02:58:23 btb Exp $ */
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
 * Header file for AI system.
 *
 * Old Log:
 * Revision 1.3  1995/10/15  16:28:07  allender
 * added flag to player_is_visible function
 *
 * Revision 1.2  1995/10/10  11:48:32  allender
 * PC ai header
 *
 * Revision 1.1  1995/05/16  15:54:00  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:33:07  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.57  1995/02/04  17:28:31  mike
 * make station guys return better.
 *
 * Revision 1.56  1995/02/04  10:03:23  mike
 * Fly to exit cheat.
 *
 * Revision 1.55  1995/02/01  19:23:52  rob
 * Externed a boss var.
 *
 * Revision 1.54  1995/01/30  13:00:58  mike
 * Make robots fire at tPlayer other than one they are controlled by sometimes.
 *
 * Revision 1.53  1995/01/26  15:09:16  rob
 * Changed robot gating to accomodate multiplayer.
 *
 * Revision 1.52  1995/01/26  12:23:12  rob
 * Added new externs needed for multiplayer.
 *
 * Revision 1.51  1995/01/21  21:22:14  mike
 * Kill prototype of InitBossSegments, which didn't need to be public
 * and had changed.
 *
 * Revision 1.50  1995/01/16  19:24:29  mike
 * Publicize BOSS_GATE_MATCEN_NUM and Boss_been_hit.
 *
 * Revision 1.49  1995/01/02  16:17:35  mike
 * prototype some super boss function for gameseq.
 *
 * Revision 1.48  1994/12/19  17:08:06  mike
 * deal with new AIMultiplayerAwareness which returns a value saying whether this tObject can be moved by this tPlayer.
 *
 * Revision 1.47  1994/12/12  17:18:04  mike
 * make boss cloak/teleport when get hit, make quad laser 3/4 as powerful.
 *
 * Revision 1.46  1994/12/08  15:46:16  mike
 * better robot behavior.
 *
 * Revision 1.45  1994/11/27  23:16:08  matt
 * Made debug code go away when debugging turned off
 *
 * Revision 1.44  1994/11/16  23:38:41  mike
 * new improved boss teleportation behavior.
 *
 * Revision 1.43  1994/11/10  17:45:11  mike
 * debugging.
 *
 * Revision 1.42  1994/11/07  10:37:42  mike
 * hooks for rob's network code.
 *
 * Revision 1.41  1994/11/06  15:10:50  mike
 * prototype a debug function for dumping ai info.
 *
 * Revision 1.40  1994/11/02  17:57:30  rob
 * Added extern of Believe_player_pos needed to get control centers
 * locating people.
 *
 * Revision 1.39  1994/10/28  19:43:39  mike
 * Prototype Boss_cloak_startTime, Boss_cloak_endTime.
 *
 * Revision 1.38  1994/10/22  14:14:42  mike
 * Prototype AIResetAllPaths.
 *
 * Revision 1.37  1994/10/21  20:42:01  mike
 * Define MAX_PATH_LENGTH: maximum allowed length of a path.
 *
 * Revision 1.36  1994/10/20  09:49:18  mike
 * Prototype something.
 *
 *
 * Revision 1.35  1994/10/18  15:37:52  mike
 * Define ROBOT_BOSS1.
 *
 * Revision 1.34  1994/10/13  11:12:25  mike
 * Prototype some door functions.
 *
 * Revision 1.33  1994/10/12  21:28:51  mike
 * Prototype CreateNSegmentPathToDoor
 * Prototype ai_open_doors_in_segment
 * Prototype AIDoorIsOpenable.
 *
 * Revision 1.32  1994/10/11  15:59:41  mike
 * Prototype Robot_firing_enabled.
 *
 * Revision 1.31  1994/10/09  22:02:48  mike
 * Adapt CreatePathPoints and CreateNSegmentPath prototypes to use avoid_seg for tPlayer evasion.
 *
 * Revision 1.30  1994/09/18  18:07:44  mike
 * Update prototypes for CreatePathPoints and CreatePathToPlayer.
 *
 * Revision 1.29  1994/09/15  16:34:08  mike
 * Prototype DoAiRobotHitAttack.
 *
 * Revision 1.28  1994/09/12  19:12:35  mike
 * Prototype attempt_to_resume_path.
 *
 * Revision 1.27  1994/08/25  21:55:32  mike
 * Add some prototypes.
 *
 * Revision 1.26  1994/08/10  19:53:24  mike
 * Prototype CreatePathToPlayer and InitRobotsForLevel.
 *
 * Revision 1.25  1994/08/04  16:32:58  mike
 * prototype CreatePathToPlayer.
 *
 * Revision 1.24  1994/08/03  15:17:20  mike
 * Prototype MakeRandomVector.
 *
 * Revision 1.23  1994/07/31  18:10:34  mike
 * Update prototype for CreatePathPoints.
 *
 * Revision 1.22  1994/07/28  12:36:14  matt
 * Cleaned up tObject bumping code
 *
 */

#ifndef _AI_H
#define _AI_H

#include <stdio.h>

#include "object.h"
#include "fvi.h"
#include "robot.h"

#define PLAYER_AWARENESS_INITIAL_TIME   (3*F1_0)
#define MAX_PATH_LENGTH                 30          // Maximum length of path in ai path following.
#define MAX_DEPTH_TO_SEARCH_FOR_PLAYER  10
#define BOSS_GATE_MATCEN_NUM            -1
#define BOSS_ECLIP_NUM                  53

#define ROBOT_BRAIN 7
#define ROBOT_BOSS1 17

#define ROBOT_FIRE_AGITATION 94

#define BOSS_D2     21 // Minimum D2 boss value.
#define BOSS_COOL   21
#define BOSS_WATER  22
#define BOSS_FIRE   23
#define BOSS_ICE    24
#define BOSS_ALIEN1 25
#define BOSS_ALIEN2 26

#if 0

extern ubyte Boss_teleports[NUM_D2_BOSSES];     // Set byte if this boss can teleport
extern ubyte Boss_spew_more[NUM_D2_BOSSES];     // Set byte if this boss can teleport
//extern ubyte Boss_cloaks[NUM_D2_BOSSES];        // Set byte if this boss can cloak
extern ubyte Boss_spews_bots_energy[NUM_D2_BOSSES];     // Set byte if boss spews bots when hit by energy weapon.
extern ubyte Boss_spews_bots_matter[NUM_D2_BOSSES];     // Set byte if boss spews bots when hit by matter weapon.
extern ubyte Boss_invulnerable_energy[NUM_D2_BOSSES];   // Set byte if boss is invulnerable to energy weapons.
extern ubyte Boss_invulnerable_matter[NUM_D2_BOSSES];   // Set byte if boss is invulnerable to matter weapons.
extern ubyte Boss_invulnerable_spot[NUM_D2_BOSSES];     // Set byte if boss is invulnerable in all but a certain spot.  (Dot product fVec|vec_to_collision < BOSS_INVULNERABLE_DOT)

#endif

extern tAILocal Ai_local_info[MAX_OBJECTS];
extern vmsVector Believed_player_pos;
extern short Believed_player_seg;

extern void move_towards_segment_center(tObject *objp);
extern int GateInRobot(short nObject, ubyte nType, short nSegment);
extern void do_ai_movement(tObject *objp);
extern void ai_move_to_new_segment( tObject * obj, short newseg, int firstTime );
// extern void AIFollowPath( tObject * obj, short newseg, int firstTime );
extern void ai_recover_from_wall_hit(tObject *obj, int nSegment);
extern void ai_move_one(tObject *objp);
extern void DoAIFrame(tObject *objp);
extern void InitAIObject(short nObject, short initial_mode, short nHideSegment);
extern void update_player_awareness(tObject *objp, fix new_awareness);
extern void CreateAwarenessEvent(tObject *objp, int nType);         // tObject *objp can create awareness of tPlayer, amount based on "nType"
extern void DoAIFrameAll(void);
extern void InitAISystem(void);
extern void reset_ai_states(tObject *objp);
extern int CreatePathPoints(tObject *objp, int start_seg, int end_seg, tPointSeg *tPointSegs, short *num_points, int max_depth, int randomFlag, int safetyFlag, int avoid_seg);
extern void create_all_paths(void);
extern void CreatePathToStation(tObject *objp, int max_length);
extern void AIFollowPath(tObject *objp, int player_visibility, int previousVisibility, vmsVector *vec_to_player);
extern void AITurnTowardsVector(vmsVector *vec_to_player, tObject *obj, fix rate);
extern void ai_turn_towards_vel_vec(tObject *objp, fix rate);
extern void InitAIObjects(void);
extern void DoAiRobotHit(tObject *robot, int nType);
extern void CreateNSegmentPath(tObject *objp, int nPathLength, short avoid_seg);
extern void CreateNSegmentPathToDoor(tObject *objp, int nPathLength, short avoid_seg);
extern void MakeRandomVector(vmsVector *vec);
extern void InitRobotsForLevel(void);
extern int AIBehaviorToMode(int behavior);
extern int Robot_firing_enabled;
extern void CreatePathToSegment(tObject *objp, short goalseg, int max_length, int safetyFlag);
extern int ready_to_fire(tRobotInfo *robptr, tAILocal *ailp);
extern int SmoothPath(tObject *objp, tPointSeg *psegs, int num_points);
extern void move_towards_player(tObject *objp, vmsVector *vec_to_player);

// max_length is maximum depth of path to create.
// If -1, use default: MAX_DEPTH_TO_SEARCH_FOR_PLAYER
extern void CreatePathToPlayer(tObject *objp, int max_length, int safetyFlag);
extern void attempt_to_resume_path(tObject *objp);

// When a robot and a tPlayer collide, some robots attack!
extern void DoAiRobotHitAttack(tObject *robot, tObject *tPlayer, vmsVector *collision_point);
extern void ai_open_doors_in_segment(tObject *robot);
extern int AIDoorIsOpenable(tObject *objp, tSegment *segp, short nSide);
extern int ObjectCanSeePlayer(tObject *objp, vmsVector *pos, fix fieldOfView, vmsVector *vec_to_player);
extern void AIResetAllPaths(void);   // Reset all paths.  Call at the start of a level.
extern int AIMultiplayerAwareness(tObject *objp, int awarenessLevel);

// In escort.c
extern void DoEscortFrame(tObject *objp, fix dist_to_player, int player_visibility);
extern void DoSnipeFrame(tObject *objp, fix dist_to_player, int player_visibility, vmsVector *vec_to_player);
extern void DoThiefFrame(tObject *objp, fix dist_to_player, int player_visibility, vmsVector *vec_to_player);

#ifndef NDEBUG
extern void force_dump_aiObjects_all(char *msg);
#else
#define force_dump_aiObjects_all(msg)
#endif

extern void start_boss_death_sequence(tObject *objp);
extern void AIInitBossForShip(void);
extern int Boss_been_hit;
extern fix AI_procTime;

// Stuff moved from ai.c by MK on 05/25/95.
#define ANIM_RATE       (F1_0/16)
#define DELTA_ANG_SCALE 16

#define OVERALL_AGITATION_MAX   100
#define MAX_AI_CLOAK_INFO       8   // Must be a power of 2!

typedef struct {
	fix         lastTime;
	int         last_segment;
	vmsVector  last_position;
} tAICloakInfo;

#define CHASE_TIME_LENGTH   (F1_0*8)
#define DEFAULT_ROBOT_SOUND_VOLUME F1_0

extern fix Dist_to_last_fired_upon_player_pos;
extern vmsVector Last_fired_upon_player_pos;
extern int Laser_rapid_fire;

#define MAX_AWARENESS_EVENTS 64
typedef struct tAwarenessEvent {
	short       nSegment; // tSegment the event occurred in
	short       nType;   // nType of event, defines behavior
	vmsVector  pos;    // absolute 3 space location of event
} tAwarenessEvent;

#define AIS_MAX 8
#define AIE_MAX 4

#define ESCORT_GOAL_UNSPECIFIED -1

#define ESCORT_GOAL_UNSPECIFIED -1
#define ESCORT_GOAL_BLUE_KEY    1
#define ESCORT_GOAL_GOLD_KEY    2
#define ESCORT_GOAL_RED_KEY     3
#define ESCORT_GOAL_CONTROLCEN  4
#define ESCORT_GOAL_EXIT        5

// Custom escort goals.
#define ESCORT_GOAL_ENERGY      6
#define ESCORT_GOAL_ENERGYCEN   7
#define ESCORT_GOAL_SHIELD      8
#define ESCORT_GOAL_POWERUP     9
#define ESCORT_GOAL_ROBOT       10
#define ESCORT_GOAL_HOSTAGE     11
#define ESCORT_GOAL_PLAYER_SPEW 12
#define ESCORT_GOAL_SCRAM       13
#define ESCORT_GOAL_EXIT2       14
#define ESCORT_GOAL_BOSS        15
#define ESCORT_GOAL_MARKER1     16
#define ESCORT_GOAL_MARKER2     17
#define ESCORT_GOAL_MARKER3     18
#define ESCORT_GOAL_MARKER4     19
#define ESCORT_GOAL_MARKER5     20
#define ESCORT_GOAL_MARKER6     21
#define ESCORT_GOAL_MARKER7     22
#define ESCORT_GOAL_MARKER8     23
#define ESCORT_GOAL_MARKER9     24

#define MAX_ESCORT_GOALS        25

#define MAX_ESCORT_DISTANCE     (F1_0*80)
#define MIN_ESCORT_DISTANCE     (F1_0*40)

#define FUELCEN_CHECK           1000

extern fix Escort_last_path_created;
extern int Escort_goalObject, Escort_special_goal, Escort_goal_index;

#define GOAL_WIDTH 11

#define SNIPE_RETREAT_TIME  (F1_0*5)
#define SNIPE_ABORT_RETREAT_TIME (SNIPE_RETREAT_TIME/2) // Can abort a retreat with this amount of time left in retreat
#define SNIPE_ATTACK_TIME   (F1_0*10)
#define SNIPE_WAIT_TIME     (F1_0*5)
#define SNIPE_FIRE_TIME     (F1_0*2)

#define THIEF_PROBABILITY   16384   // 50% chance of stealing an item at each attempt
#define MAX_STOLEN_ITEMS    10      // Maximum number kept track of, will keep stealing, causes stolen weapons to be lost!

extern int   Max_escort_length;
extern int   EscortKillObject;
extern ubyte Stolen_items[MAX_STOLEN_ITEMS];
extern fix   Escort_last_path_created;
extern int   Escort_goalObject, Escort_special_goal, Escort_goal_index;

extern void  CreateBuddyBot(void);

extern int   Max_escort_length;

extern int 	 nEscortGoalText[MAX_ESCORT_GOALS];

extern void  ai_multi_sendRobot_position(short nObject, int force);

extern int   Flinch_scale;
extern int   Attack_scale;
extern sbyte Mike_to_matt_xlate[];

// Amount of time since the current robot was last processed for things such as movement.
// It is not valid to use FrameTime because robots do not get moved every frame.

extern int   Num_boss_teleport_segs[MAX_BOSS_COUNT];
extern short Boss_teleport_segs[MAX_BOSS_COUNT][MAX_BOSS_TELEPORT_SEGS];
extern int   Num_boss_gate_segs[MAX_BOSS_COUNT];
extern short Boss_gate_segs[MAX_BOSS_COUNT][MAX_BOSS_TELEPORT_SEGS];
extern int	 boss_obj_num [MAX_BOSS_COUNT];


// --------- John: These variables must be saved as part of gamesave. ---------
extern int              Ai_initialized;
extern int              nOverallAgitation;
extern tAILocal         Ai_local_info[MAX_OBJECTS];
extern tPointSeg        Point_segs[MAX_POINT_SEGS];
extern tPointSeg        *Point_segs_free_ptr;
extern tAICloakInfo    Ai_cloak_info[MAX_AI_CLOAK_INFO];
// -- extern int              Boss_been_hit;
// ------ John: End of variables which must be saved as part of gamesave. -----

extern int  ai_evaded;

extern sbyte Super_boss_gate_list[];
#define MAX_GATE_INDEX  25

extern int  Ai_info_enabled;
extern int  Robot_firing_enabled;


// These globals are set by a call to FindVectorIntersection, which is a slow routine,
// so we don't want to call it again (for this tObject) unless we have to.
extern vmsVector   Hit_pos;
extern int          HitType, Hit_seg;
extern fvi_info     hitData;

extern int              Num_tAwarenessEvents;
extern tAwarenessEvent  Awareness_events[MAX_AWARENESS_EVENTS];

extern vmsVector       Believed_player_pos;

#ifndef NDEBUG
// Index into this array with ailp->mode
extern char *mode_text[18];

// Index into this array with aip->behavior
extern char behavior_text[6][9];

// Index into this array with aip->GOAL_STATE or aip->CURRENT_STATE
extern char state_text[8][5];

extern int Do_aiFlag, Break_onObject;

extern void mprintf_animation_info(tObject *objp);

#endif //ifndef NDEBUG

extern int Stolen_item_index;   // Used in ai.c for controlling rate of Thief flare firing.

extern void ai_frame_animation(tObject *objp);
extern int do_silly_animation(tObject *objp);
extern int openable_doors_in_segment(short nSegment);
extern void ComputeVisAndVec(tObject *objp, vmsVector *pos, tAILocal *ailp, vmsVector *vec_to_player, int *player_visibility, tRobotInfo *robptr, int *flag);
extern void do_firing_stuff(tObject *obj, int player_visibility, vmsVector *vec_to_player);
extern int maybe_ai_do_actual_firing_stuff(tObject *obj, tAIStatic *aip);
extern void ai_do_actual_firing_stuff(tObject *obj, tAIStatic *aip, tAILocal *ailp, tRobotInfo *robptr, vmsVector *vec_to_player, fix dist_to_player, vmsVector *gun_point, int player_visibility, int object_animates, int gun_num);
extern void do_super_boss_stuff(tObject *objp, fix dist_to_player, int player_visibility);
extern void DoBossStuff(tObject *objp, int player_visibility);
// -- unused, 08/07/95 -- extern void ai_turn_randomly(vmsVector *vec_to_player, tObject *obj, fix rate, int previousVisibility);
extern void ai_move_relative_to_player(tObject *objp, tAILocal *ailp, fix dist_to_player, vmsVector *vec_to_player, fix circleDistance, int evade_only, int player_visibility);
extern void move_away_from_player(tObject *objp, vmsVector *vec_to_player, int attackType);
extern void move_towards_vector(tObject *objp, vmsVector *vec_goal, int dot_based);
extern void InitAIFrame(void);

extern void CreateBfsList(int start_seg, short bfs_list[], int *length, int max_segs);
extern void InitThiefForLevel();

extern int AISaveBinState (CFILE *fp);
extern int AISaveUniState (CFILE *fp);
extern int AIRestoreBinState (CFILE *fp, int version);
extern int AIRestoreUniState (CFILE *fp, int version);

extern void StartRobotDeathSequence(tObject *objp);
extern int DoAnyRobotDyingFrame(tObject *objp);
extern void _CDECL_ BuddyMessage(char * format, ... );

#define SPECIAL_REACTOR_ROBOT   65
extern void SpecialReactorStuff(void);

#endif /* _AI_H */
