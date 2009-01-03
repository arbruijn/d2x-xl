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

#ifndef _AI_H
#define _AI_H

#include <stdio.h>

#include "object.h"
#include "fvi.h"
#include "robot.h"

#define PLAYER_AWARENESS_INITIAL_TIME   (I2X (3))
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

fix MoveTowardsSegmentCenter (CObject *objP);
fix MoveTowardsPoint (CObject *objP, CFixVector *vGoal, fix xMinDist);
int GateInRobot (short nObject, ubyte nType, short nSegment);
void DoAIMovement (CObject *objP);
void AIMoveToNewSegment ( CObject * obj, short newseg, int firstTime );
// void AIFollowPath ( CObject * obj, short newseg, int firstTime );
void AIRecoverFromWallHit (CObject *obj, int nSegment);
void AIMoveOne (CObject *objP);
void DoAIFrame (CObject *objP);
void DoD1AIFrame (CObject *objP);
void InitAIObject (short nObject, short initial_mode, short nHideSegment);
void UpdatePlayerAwareness (CObject *objP, fix new_awareness);
void CreateAwarenessEvent (CObject *objP, int nType);         // CObject *objP can create awareness of CPlayerData, amount based on "nType"
void DoAIFrameAll (void);
void DoD1AIFrameAll (void);
void InitAISystem (void);
void ResetAIStates (CObject *objP);
int CreatePathPoints (CObject *objP, int start_seg, int end_seg, tPointSeg *tPointSegs, short *num_points, int max_depth, int randomFlag, int safetyFlag, int avoid_seg);
void CreateAllPaths (void);
void CreatePathToStation (CObject *objP, int max_length);
void AIFollowPath (CObject *objP, int player_visibility, int previousVisibility, CFixVector *vec_to_player);
fix AITurnTowardsVector (CFixVector *vec_to_player, CObject *obj, fix rate);
void AITurnTowardsVelVec (CObject *objP, fix rate);
void InitAIObjects (void);
void DoAIRobotHit (CObject *robot, int nType);
void DoD1AIRobotHit (CObject *objP, int type);
void CreateNSegmentPath (CObject *objP, int nPathLength, short avoid_seg);
void CreateNSegmentPathToDoor (CObject *objP, int nPathLength, short avoid_seg);
void InitRobotsForLevel (void);
int AIBehaviorToMode (int behavior);
void CreatePathToSegment (CObject *objP, short goalseg, int max_length, int safetyFlag);
int ReadyToFire (tRobotInfo *robptr, tAILocalInfo *ailp);
int SmoothPath (CObject *objP, tPointSeg *psegs, int num_points);
void MoveTowardsPlayer (CObject *objP, CFixVector *vec_to_player);

int AICanFireAtPlayer (CObject *objP, CFixVector *vGun, CFixVector *vPlayer);

void InitBossData (int i, int nObject);
int AddBoss (int nObject);
void RemoveBoss (int i);
void DoBossDyingFrame (CObject *objP);

// max_length is maximum depth of path to create.
// If -1, use default: MAX_DEPTH_TO_SEARCH_FOR_PLAYER
void CreatePathToPlayer (CObject *objP, int max_length, int safetyFlag);
void AttemptToResumePath (CObject *objP);

// When a robot and a CPlayerData collide, some robots attack!
void DoAIRobotHitAttack (CObject *robot, CObject *CPlayerData, CFixVector *collision_point);
void DoD1AIRobotHitAttack(CObject *robot, CObject *player, CFixVector *collision_point);
void AIOpenDoorsInSegment (CObject *robot);
int AIDoorIsOpenable (CObject *objP, CSegment *segp, short nSide);
int ObjectCanSeePlayer (CObject *objP, CFixVector *pos, fix fieldOfView, CFixVector *vec_to_player);
void AIResetAllPaths (void);   // Reset all paths.  Call at the start of a level.
int AIMultiplayerAwareness (CObject *objP, int awarenessLevel);

// In escort.c
void DoEscortFrame (CObject *objP, fix dist_to_player, int player_visibility);
void DoSnipeFrame (CObject *objP);
void DoThiefFrame (CObject *objP);

#if DBG
void force_dump_aiObjects_all (char *msg);
#else
#define force_dump_aiObjects_all (msg)
#endif

void StartBossDeathSequence (CObject *objP);
void AIInitBossForShip (void);
extern int Boss_been_hit;
extern fix AI_procTime;

// Stuff moved from ai.c by MK on 05/25/95.
#define ANIM_RATE       (I2X (1) / 16)
#define DELTA_ANG_SCALE 16

#define OVERALL_AGITATION_MAX   100
#define MAX_AI_CLOAK_INFO       32   // Must be a power of 2!

typedef struct {
	fix         lastTime;
	int         nLastSeg;
	CFixVector   vLastPos;
} tAICloakInfo;

#define CHASE_TIME_LENGTH   (I2X (8))
#define DEFAULT_ROBOT_SOUND_VOLUME I2X (1)

extern fix xDistToLastPlayerPosFiredAt;
extern CFixVector vLastPlayerPosFiredAt;

#define MAX_AWARENESS_EVENTS 256
typedef struct tAwarenessEvent {
	short       nSegment; // CSegment the event occurred in
	short       nType;   // nType of event, defines behavior
	CFixVector	pos;    // absolute 3 space location of event
} tAwarenessEvent;

#define AIS_MAX 8
#define AIE_MAX 5

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

#define MAX_ESCORT_DISTANCE     I2X (80)
#define MIN_ESCORT_DISTANCE     I2X (40)

#define FUELCEN_CHECK           1000

extern fix Escort_last_path_created;
extern int Escort_goalObject, Escort_special_goal, Escort_goal_index;

#define GOAL_WIDTH 11

#define SNIPE_RETREAT_TIME  I2X (5)
#define SNIPE_ABORT_RETREAT_TIME (SNIPE_RETREAT_TIME/2) // Can abort a retreat with this amount of time left in retreat
#define SNIPE_ATTACK_TIME   I2X (10)
#define SNIPE_WAIT_TIME     I2X (5)
#define SNIPE_FIRE_TIME     I2X (2)

#define THIEF_PROBABILITY   16384   // 50% chance of stealing an item at each attempt
#define MAX_STOLEN_ITEMS    10      // Maximum number kept track of, will keep stealing, causes stolen weapons to be lost!

#if 0

extern ubyte Stolen_items [MAX_STOLEN_ITEMS];
extern int Stolen_item_index;   // Used in ai.c for controlling rate of Thief flare firing.

extern int   Max_escort_length;
extern int   EscortKillObject;
extern fix   Escort_last_path_created;
extern int   Escort_goalObject, Escort_special_goal, Escort_goal_index;
extern int 	 nEscortGoalText [MAX_ESCORT_GOALS];

extern int   Num_boss_teleport_segs [MAX_BOSS_COUNT];
extern short Boss_teleport_segs [MAX_BOSS_COUNT][MAX_BOSS_TELEPORT_SEGS];
extern int   Num_boss_gate_segs [MAX_BOSS_COUNT];
extern short Boss_gate_segs [MAX_BOSS_COUNT][MAX_BOSS_TELEPORT_SEGS];
extern int	 boss_obj_num [MAX_BOSS_COUNT];

// --------- John: These variables must be saved as part of gamesave. ---------
extern int              Ai_initialized;
extern int              nOverallAgitation;
extern tAILocalInfo         aiLocalInfo [MAX_OBJECTS];
extern tPointSeg        Point_segs [MAX_POINT_SEGS];
extern tPointSeg        *Point_segs_free_ptr;
extern tAICloakInfo    aiCloakInfo [MAX_AI_CLOAK_INFO];

#endif

extern void  CreateBuddyBot (void);

extern void  AIMultiSendRobotPos (short nObject, int force);

extern int   Flinch_scale;
extern int   Attack_scale;
extern sbyte Mike_to_matt_xlate [];

// Amount of time since the current robot was last processed for things such as movement.
// It is not valid to use FrameTime because robots do not get moved every frame.


// -- extern int              Boss_been_hit;
// ------ John: End of variables which must be saved as part of gamesave. -----

extern int  ai_evaded;

extern sbyte Super_boss_gate_list [];
#define MAX_GATE_INDEX  25

// These globals are set by a call to FindVectorIntersection, which is a slow routine,
// so we don't want to call it again (for this CObject) unless we have to.
//extern CFixVector   Hit_pos;
//extern int          HitType, Hit_seg;

#if DBG
// Index into this array with ailp->mode
// Index into this array with aip->behavior
extern char behavior_text [6][9];

// Index into this array with aip->GOAL_STATE or aip->CURRENT_STATE
extern char state_text [8][5];

extern int bDoAIFlag, nBreakOnObject;

extern void mprintf_animation_info (CObject *objP);

#endif //if DBG

void AIFrameAnimation (CObject *objP);
void AIIdleAnimation (CObject *objP);
int DoSillyAnimation (CObject *objP);
void ComputeVisAndVec (CObject *objP, CFixVector *pos, tAILocalInfo *ailp, tRobotInfo *robptr, int *flag, fix xMaxVisibleDist);
void DoFiringStuff (CObject *obj, int player_visibility, CFixVector *vec_to_player);
int AIMaybeDoActualFiringStuff (CObject *obj, tAIStaticInfo *aip);
void AIDoActualFiringStuff (CObject *obj, tAIStaticInfo *aip, tAILocalInfo *ailp, tRobotInfo *robptr, int gun_num);
void DoSuperBossStuff (CObject *objP, fix dist_to_player, int player_visibility);
void DoBossStuff (CObject *objP, int player_visibility);
// -- unused, 08/07/95 -- void ai_turn_randomly (CFixVector *vec_to_player, CObject *obj, fix rate, int previousVisibility);
void AIMoveRelativeToPlayer (CObject *objP, tAILocalInfo *ailp, fix dist_to_player, CFixVector *vec_to_player, fix circleDistance, int evade_only, int player_visibility);
void MoveAwayFromPlayer (CObject *objP, CFixVector *vec_to_player, int attackType);
void MoveTowardsVector (CObject *objP, CFixVector *vec_goal, int dot_based);
void InitAIFrame (void);
void MakeNearbyRobotSnipe (void);

void CreateBfsList (int start_seg, short bfs_list [], int *length, int max_segs);
void InitThiefForLevel ();

int AISaveBinState (CFile& cf);
int AISaveUniState (CFile& cf);
int AIRestoreBinState (CFile& cf, int version);
int AIRestoreUniState (CFile& cf, int version);

int CheckObjectObjectIntersection (CFixVector *pos, fix size, CSegment *segP);
int DoRobotDyingFrame (CObject *objP, fix StartTime, fix xRollDuration, sbyte *bDyingSoundPlaying, short deathSound, fix xExplScale, fix xSoundScale);

void TeleportBoss (CObject *objP);
int BossFitsInSeg (CObject *bossObjP, int nSegment);

void StartRobotDeathSequence (CObject *objP);
int DoAnyRobotDyingFrame (CObject *objP);

#define SPECIAL_REACTOR_ROBOT   65
extern void SpecialReactorStuff (void);

// --------------------------------------------------------------------------------------------------------------------

static inline fix AIMaxDist (int i, tRobotInfo *botInfoP)
{
if (gameOpts->gameplay.nAIAwareness) {
	switch (i) {
		case 0:
			return I2X (100 + gameStates.app.nDifficultyLevel * 100);
		case 1:
			return I2X (300);
		case 2:
			return I2X (500);
		case 3:
			return I2X (750);
		case 4:
			return I2X (50 * (2 * gameStates.app.nDifficultyLevel + botInfoP->pursuit));
		}
	}
else {
	switch (i) {
		case 0:
			return I2X (120 + gameStates.app.nDifficultyLevel * 20);
		case 1:
			return I2X (80);
		case 2:
			return I2X (200);
		case 3:
			return I2X (200);
		case 4:
			return I2X (20 * (2 * gameStates.app.nDifficultyLevel + botInfoP->pursuit));
		}
	}
return 0;
}

#define MAX_WAKEUP_DIST		AIMaxDist (0, NULL)
#define MAX_CHASE_DIST		AIMaxDist (1, NULL)
#define MAX_SNIPE_DIST		AIMaxDist (2, NULL)
#define MAX_REACTION_DIST	AIMaxDist (3, NULL)
#define MAX_PURSUIT_DIST(_botInfoP)	AIMaxDist (4, _botInfoP)

#define USE_D1_AI (gameStates.app.bD1Mission && gameOpts->gameplay.bUseD1AI)

// --------------------------------------------------------------------------------------------------------------------

#endif /* _AI_H */
