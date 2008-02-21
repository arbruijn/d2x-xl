/* $Id: aistruct.h,v 1.2 2003/10/04 03:14:47 btb Exp $ */
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

#ifndef _AISTRUCT_H
#define _AISTRUCT_H

//#include "inferno.h"
//#include "polyobj.h"

#define GREEN_GUY   1

#define MAX_SEGMENTS_PER_PATH       20

#define PA_NEARBY_ROBOT_FIRED       1   // Level of robot awareness after nearby robot fires a weapon
#define PA_WEAPON_WALL_COLLISION    2   // Level of robot awareness after tPlayer weapon hits nearby tWall
//#define PA_PLAYER_VISIBLE         2   // Level of robot awareness if robot is looking towards tPlayer, and tPlayer not hidden
#define PA_PLAYER_COLLISION         3   // Level of robot awareness after tPlayer bumps into robot
#define PA_RETURN_FIRE					4	 // Level of robot awareness while firing back after having been hit by player
#define PA_WEAPON_ROBOT_COLLISION   5   // Level of robot awareness after tPlayer weapon hits nearby robot

//#define PAE_WEAPON_HIT_WALL         1   // weapon hit tWall, create tPlayer awareness
//#define PAE_WEAPON_HIT_ROBOT        2   // weapon hit tWall, create tPlayer awareness

// Constants indicating currently moving forward or backward through
// path.  Note that you can add aip->direction to aip_path_index to
// get next tSegment on path.
#define AI_DIR_FORWARD  1
#define AI_DIR_BACKWARD (-AI_DIR_FORWARD)

// Behaviors
#define AIB_STILL      					0x80
#define AIB_NORMAL      				0x81
#define AIB_BEHIND      				0x82
#define AIB_RUN_FROM    				0x83
#define AIB_SNIPE       				0x84
#define AIB_STATION     				0x85
#define AIB_FOLLOW      				0x86
#define AIB_STATIC						0x87

#define MIN_BEHAVIOR						0x80
#define MAX_BEHAVIOR						0x87

//  Modes
#define AIM_IDLING                  0
#define AIM_WANDER                  1
#define AIM_FOLLOW_PATH             2
#define AIM_CHASE_OBJECT            3
#define AIM_RUN_FROM_OBJECT         4
#define AIM_BEHIND                  5
#define AIM_FOLLOW_PATH_2           6
#define AIM_OPEN_DOOR               7
#define AIM_GOTO_PLAYER             8   //  Only for escort behavior
#define AIM_GOTO_OBJECT             9   //  Only for escort behavior

#define AIM_SNIPE_ATTACK            10
#define AIM_SNIPE_FIRE              11
#define AIM_SNIPE_RETREAT           12
#define AIM_SNIPE_RETREAT_BACKWARDS 13
#define AIM_SNIPE_WAIT              14

#define AIM_THIEF_ATTACK            15
#define AIM_THIEF_RETREAT           16
#define AIM_THIEF_WAIT              17

#define AISM_GOHIDE                 0
#define AISM_HIDING                 1

#define AI_MAX_STATE    7
#define AI_MAX_EVENT    5

#define AIS_NONE        0
#define AIS_REST        1
#define AIS_SEARCH      2
#define AIS_LOCK        3
#define AIS_FLINCH      4
#define AIS_FIRE        5
#define AIS_RECOVER     6
#define AIS_ERROR       7

#define AIE_FIRE        0
#define AIE_HIT         1
#define AIE_COLLIDE     2
#define AIE_HURT        3

//typedef struct opath {
//	sbyte   path_index;     // current index of path
//	sbyte   path_direction; // current path direction
//	sbyte   nPathLength;    // length of current path
//	sbyte   nothing;
//	short   path[MAX_SEGMENTS_PER_PATH];
//	short   always_0xabc;   // If this is ever not 0xabc, then someone overwrote
//} opath;
//
//typedef struct oaiState {
//	short   mode;               //
//	short   counter;            // kind of a hack, frame countdown until switch modes
//	opath   paths[2];
//	vmsVector movement_vector; // movement vector for one second
//} oaiState;

#define SUB_FLAGS_GUNSEG        0x01
#define SUB_FLAGS_SPROX         0x02    // If set, then this bot drops a super prox, not a prox, when it's time to drop something
#define SUB_FLAGS_CAMERA_AWAKE  0x04    // If set, a camera (on a missile) woke this robot up, so don't fire at tPlayer.  Can look real stupid!

//  Constants defining meaning of flags in aiState
#define MAX_AI_FLAGS    11          // This MUST cause word (4 bytes) alignment in tAIStatic, allowing for one byte mode

#define CURRENT_GUN     flags[0]    // This is the last gun the tObject fired from
#define CURRENT_STATE   flags[1]    // current behavioral state
#define GOAL_STATE      flags[2]    // goal state
#define PATH_DIR        flags[3]    // direction traveling path, 1 = forward, -1 = backward, other = error!
#define SUB_FLAGS       flags[4]    // bit 0: Set -> Robot's current gun in different tSegment than robot's center.
#define GOALSIDE        flags[5]    // for guys who open doors, this is the tSide they are going after.
#define CLOAKED         flags[6]    // Cloaked now.
#define SKIP_AI_COUNT   flags[7]    // Skip AI this frame, but decrement in DoAIFrame.
#define  REMOTE_OWNER   flags[8]    // Who is controlling this remote AI tObject (multiplayer use only)
#define  REMOTE_SLOT_NUM flags[9]   // What slot # is this robot in for remote control purposes (multiplayer use only)
#define  MULTI_ANGER    flags[10]   // How angry is a robot in multiplayer mode

// This is the stuff that is permanent for an AI tObject and is
// therefore saved to disk.
typedef struct tAIStatic {
	ubyte   behavior;               //
	sbyte   flags[MAX_AI_FLAGS];    // various flags, meaning defined by constants
	short   nHideSegment;           // Segment to go to for hiding.
	short   nHideIndex;             // Index in Path_seg_points
	short   nPathLength;            // Length of hide path.
	sbyte   nCurPathIndex;         // Current index in path.
	sbyte   bDyingSoundPlaying;    // !0 if this robot is playing its dying sound.

	// -- not needed! -- short   follow_path_start_seg;  // Start tSegment for robot which follows path.
	// -- not needed! -- short   follow_path_end_seg;    // End tSegment for robot which follows path.

	short   nDangerLaser;
	int     nDangerLaserSig;
	fix     xDyingStartTime;       // Time at which this robot started dying.

	//sbyte   extras[28];             // 32 extra bytes for storing stuff so we don't have to change versions on disk
} __pack__ tAIStatic;

// This is the stuff which doesn't need to be saved to disk.
typedef struct tAILocal {
// These used to be bytes, changed to ints so I could set watchpoints on them.
// playerAwarenessType..nRapidFireCount used to be bytes
// nGoalSegment used to be short.
	int     playerAwarenessType;	// nType of awareness of tPlayer
	int     nRetryCount;          // number of retries in physics last time this tObject got moved.
	int     nConsecutiveRetries;  // number of retries in consecutive frames (ie, without a nRetryCount of 0)
	int     mode;                 // current mode within behavior
	int     nPrevVisibility;		// Visibility of tPlayer last time we checked.
	int     nRapidFireCount;      // number of shots fired rapidly
	int     nGoalSegment;         // goal tSegment for current path

	// -- MK, 10/21/95, unused -- fix     last_seeTime, last_attackTime; // For sound effects, time at which tPlayer last seen, attacked

	fix     nextActionTime;						// time in seconds until something happens, mode dependent
	fix     nextPrimaryFire;               // time in seconds until can fire again
	fix     nextSecondaryFire;             // time in seconds until can fire again from second weapon
	fix     playerAwarenessTime;				// time in seconds robot will be aware of tPlayer, 0 means not aware of tPlayer
	fix     timePlayerSeen;						// absolute time in seconds at which tPlayer was last seen, might cause to go into follow_path mode
	fix     timePlayerSoundAttacked;			// absolute time in seconds at which tPlayer was last seen with visibility of 2.
	fix     nextMiscSoundTime;					// absolute time in seconds at which this robot last made an angry or lurking sound.
	fix     timeSinceProcessed;				// time since this robot last processed in DoAIFrame
	vmsAngVec goalAngles [MAX_SUBMODELS];  // angles for each subobject
	vmsAngVec deltaAngles [MAX_SUBMODELS]; // angles for each subobject
	sbyte   goalState [MAX_SUBMODELS];     // Goal state for this sub-tObject
	sbyte   achievedState [MAX_SUBMODELS]; // Last achieved state
} tAILocal;

typedef struct {
	int         nSegment;
	vmsVector	point;
	ubyte			nConnSide;
} tPointSeg;

typedef struct {
	short       start, end;
	ubyte			nConnSide;
} segQueueEntry;

#define MAX_POINT_SEGS_D2  2500
#define MAX_POINT_SEGS  	(MAX_SEGMENTS_D2X * 4)

// These are the information for a robot describing the location of
// the tPlayer last time he wasn't cloaked, and the time at which he
// was uncloaked.  We should store this for each robot, but that's
// memory expensive.
//extern fix        Last_uncloakedTime;
//extern vmsVector Last_uncloaked_position;

extern void AIDoCloakStuff(void);

#endif /* _AISTRUCT_H */
