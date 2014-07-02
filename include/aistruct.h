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

#include "d1_aistruct.h"
//#include "descent.h"
//#include "polymodel.h"

#define GREEN_GUY   1

#define MAX_SEGMENTS_PER_PATH       20

#define PA_NEARBY_ROBOT_FIRED       1   // Level of robot awareness after nearby robot fires a weapon
#define PA_WEAPON_WALL_COLLISION    2   // Level of robot awareness after player weapon hits nearby CWall
//#define PA_PLAYER_VISIBLE         2   // Level of robot awareness if robot is looking towards player, and player not hidden
#define PA_PLAYER_COLLISION         3   // Level of robot awareness after player bumps into robot
#define PA_RETURN_FIRE					4	 // Level of robot awareness while firing back after having been hit by player
#define PA_WEAPON_ROBOT_COLLISION   5   // Level of robot awareness after player weapon hits nearby robot

#define WEAPON_WALL_COLLISION 	(USE_D1_AI ? D1_PA_WEAPON_WALL_COLLISION : PA_WEAPON_WALL_COLLISION)
#define WEAPON_ROBOT_COLLISION 	(USE_D1_AI ? D1_PA_WEAPON_ROBOT_COLLISION : PA_WEAPON_ROBOT_COLLISION)

//#define PAE_WEAPON_HIT_WALL         1   // weapon hit CWall, create player awareness
//#define PAE_WEAPON_HIT_ROBOT        2   // weapon hit CWall, create player awareness

// Constants indicating currently moving forward or backward through
// path.  Note that you can add aip->direction to aip_path_index to
// get next CSegment on path.
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

#define SUB_FLAGS_GUNSEG        0x01
#define SUB_FLAGS_SPROX         0x02    // If set, then this bot drops a super prox, not a prox, when it's time to drop something
#define SUB_FLAGS_CAMERA_AWAKE  0x04    // If set, a camera (on a missile) woke this robot up, so don't fire at player.  Can look real stupid!

//  Constants defining meaning of flags in aiState
#define MAX_AI_FLAGS    11          // This MUST cause word (4 bytes) alignment in tAIStaticInfo, allowing for one byte mode

#define CURRENT_GUN     flags[0]    // This is the last gun the CObject fired from
#define CURRENT_STATE   flags[1]    // current behavioral state
#define GOAL_STATE      flags[2]    // goal state
#define PATH_DIR        flags[3]    // direction traveling path, 1 = forward, -1 = backward, other = error!
#define SUB_FLAGS       flags[4]    // bit 0: Set -> Robot's current gun in different CSegment than robot's center.
#define GOALSIDE        flags[5]    // for guys who open doors, this is the CSide they are going after.
#define CLOAKED         flags[6]    // Cloaked now.
#define SKIP_AI_COUNT   flags[7]    // Skip AI this frame, but decrement in DoAIFrame.
#define  REMOTE_OWNER   flags[8]    // Who is controlling this remote AI CObject (multiplayer use only)
#define  REMOTE_SLOT_NUM flags[9]   // What slot # is this robot in for remote control purposes (multiplayer use only)
#define  MULTI_ANGER    flags[10]   // How angry is a robot in multiplayer mode

// This is the stuff that is permanent for an AI CObject and is
// therefore saved to disk.
typedef struct tAIStaticInfo {
	uint8_t   behavior;               //
	int8_t   flags[MAX_AI_FLAGS];    // various flags, meaning defined by constants
	int16_t   nHideSegment;           // Segment to go to for hiding.
	int16_t   nHideIndex;             // Index in Path_seg_points
	int16_t   nPathLength;            // Length of hide path.
	int8_t   nCurPathIndex;         // Current index in path.
	int8_t   bDyingSoundPlaying;    // !0 if this robot is playing its dying sound.
	int16_t   nDangerLaser;
	int32_t     nDangerLaserSig;
	fix     xDyingStartTime;       // Time at which this robot started dying.
} __pack__ tAIStaticInfo;

class CAIStaticInfo {
	private:
		tAIStaticInfo m_info;
	public:
		inline tAIStaticInfo* GetInfo (void) { return &m_info; }
		inline uint8_t Behavior() { return m_info.behavior; }
		inline int8_t Flags(int32_t i) { return m_info.flags [i]; }
		inline int16_t HideSegment() { return m_info.nHideSegment; }
		inline int16_t HideIndex() { return m_info.nHideIndex; }
		inline int16_t PathLength() { return m_info.nPathLength; }
		inline int8_t CurPathIndex() { return m_info.nCurPathIndex; }
		inline int8_t DyingSoundPlaying() { return m_info.bDyingSoundPlaying; }
		inline int16_t DangerLaser() { return m_info.nDangerLaser; }
		inline int32_t DangerLaserSig() { return m_info.nDangerLaserSig; }
		inline fix DyingStartTime() { return m_info.xDyingStartTime; }
};

// This is the stuff which doesn't need to be saved to disk.
typedef struct tAILocalInfo {
// These used to be bytes, changed to ints so I could set watchpoints on them.
// targetAwarenessType..nRapidFireCount used to be bytes
// nGoalSegment used to be int16_t.
	int32_t     targetAwarenessType;	// nType of awareness of player
	int32_t     nRetryCount;          // number of retries in physics last time this CObject got moved.
	int32_t     nConsecutiveRetries;  // number of retries in consecutive frames (ie, without a nRetryCount of 0)
	int32_t     mode;                 // current mode within behavior
	int32_t     nPrevVisibility;		// Visibility of player last time we checked.
	int32_t     nRapidFireCount;      // number of shots fired rapidly
	int32_t     nGoalSegment;         // goal CSegment for current path

	// -- MK, 10/21/95, unused -- fix     last_seeTime, last_attackTime; // For sound effects, time at which player last seen, attacked

	fix     nextActionTime;						// time in seconds until something happens, mode dependent
	fix     nextPrimaryFire;               // time in seconds until can fire again
	fix     nextSecondaryFire;             // time in seconds until can fire again from second weapon
	fix     targetAwarenessTime;				// time in seconds robot will be aware of player, 0 means not aware of player
	fix     timeTargetSeen;						// absolute time in seconds at which player was last seen, might cause to go into follow_path mode
	fix     timeTargetSoundAttacked;			// absolute time in seconds at which player was last seen with visibility of 2.
	fix     nextMiscSoundTime;					// absolute time in seconds at which this robot last made an angry or lurking sound.
	fix     timeSinceProcessed;				// time since this robot last processed in DoAIFrame
	CAngleVector goalAngles [MAX_SUBMODELS];  // angles for each subobject
	CAngleVector deltaAngles [MAX_SUBMODELS]; // angles for each subobject
	int8_t   goalState [MAX_SUBMODELS];     // Goal state for this sub-CObject
	int8_t   achievedState [MAX_SUBMODELS]; // Last achieved state
} __pack__ tAILocalInfo;

typedef struct {
	int32_t         nSegment;
	CFixVector	point;
	uint8_t			nConnSide;
} __pack__ tPointSeg;

typedef struct {
	int16_t       start, end;
	uint8_t			nConnSide;
} __pack__ segQueueEntry;

#define MAX_POINT_SEGS_D2  2500
#define MAX_POINT_SEGS  	(MAX_SEGMENTS_D2X * 4)

// These are the information for a robot describing the location of
// the player last time he wasn't cloaked, and the time at which he
// was uncloaked.  We should store this for each robot, but that's
// memory expensive.
//extern fix        Last_uncloakedTime;
//extern CFixVector Last_uncloaked_position;

extern void AIDoCloakStuff(void);

#endif /* _AISTRUCT_H */
