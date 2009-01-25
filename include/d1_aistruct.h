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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _D1_AISTRUCT_H
#define _D1_AISTRUCT_H

#include "descent.h"
//#include "polymodel.h"

#define	GREEN_GUY	1

#define MAX_SEGMENTS_PER_PATH		20

#define	D1_PA_NEARBY_ROBOT_FIRED		1		// Level of robot awareness after nearby robot fires a weapon
#define	D1_PA_WEAPON_WALL_COLLISION	2		// Level of robot awareness after player weapon hits nearby wall
#define	D1_PA_PLAYER_COLLISION			3		// Level of robot awareness after player bumps into robot
#define	D1_PA_WEAPON_ROBOT_COLLISION	4		// Level of robot awareness after player weapon hits nearby robot

// #define	PAE_WEAPON_HIT_WALL		1					// weapon hit wall, create player awareness
// #define	PAE_WEAPON_HIT_ROBOT		2					// weapon hit wall, create player awareness

//	Constants indicating currently moving forward or backward through path.
//	Note that you can add aip->direction to aip_path_index to get next segment on path.
#define	D1_AI_DIR_FORWARD		1
#define	D1_AI_DIR_BACKWARD	(-D1_AI_DIR_FORWARD)

//	Behaviors
#define	D1_AIB_STILL						0x80
#define	D1_AIB_NORMAL						0x81
#define	D1_AIB_HIDE							0x82
#define	D1_AIB_RUN_FROM					0x83
#define	D1_AIB_FOLLOW_PATH				0x84
#define	D1_AIB_STATION						0x85

#define	D1_MIN_BEHAVIOR	0x80
#define	D1_MAX_BEHAVIOR	0x85

//	Modes
#define	D1_AIM_STILL						0
#define	D1_AIM_WANDER						1
#define	D1_AIM_FOLLOW_PATH				2
#define	D1_AIM_CHASE_OBJECT				3
#define	D1_AIM_RUN_FROM_OBJECT			4
#define	D1_AIM_HIDE							5
#define	D1_AIM_FOLLOW_PATH_2				6
#define	D1_AIM_OPEN_DOOR					7

#define	D1_AISM_GOHIDE						0
#define	D1_AISM_HIDING						1

#define	D1_AI_MAX_STATE	7
#define	D1_AI_MAX_EVENT	4

#define	D1_AIS_NONE		0
#define	D1_AIS_REST		1
#define	D1_AIS_SRCH		2
#define	D1_AIS_LOCK		3
#define	D1_AIS_FLIN		4
#define	D1_AIS_FIRE		5
#define	D1_AIS_RECO		6
#define	D1_AIS_ERR_		7

#define	D1_AIE_FIRE		0
#define	D1_AIE_HITT		1
#define	D1_AIE_COLL		2
#define	D1_AIE_HURT		3

//typedef struct opath {
//	byte			path_index;					// current index of path
//	byte			path_direction;			// current path direction
//	byte			path_length;				//	length of current path
//	byte			nothing;
//	short			path[MAX_SEGMENTS_PER_PATH];
//	short			always_0xabc;				//	If this is ever not 0xabc, then someone overwrote
//} opath;
//
//typedef struct oD1_AI_state {
//	short			mode;							// 
//	short			counter;						// kind of a hack, frame countdown until switch modes
//	opath			paths[2];
//	CFixVector	movement_vector;			// movement vector for one second
//} oD1_AI_state;

//	Constants defining meaning of flags in D1_AI_state
#define	MAX_D1_AI_FLAGS	11					//	This MUST cause word (4 bytes) alignment in D1_AI_static, allowing for one byte mode

#define	CURRENT_GUN		flags[0]			//	This is the last gun the object fired from
#define	CURRENT_STATE	flags[1]			//	current behavioral state
#define	GOAL_STATE		flags[2]			//	goal state
#define	PATH_DIR			flags[3]			//	direction traveling path, 1 = forward, -1 = backward, other = error!
#define	SUBMODE			flags[4]			//	submode, eg D1_AISM_HIDING if mode == D1_AIM_HIDE
#define	GOALSIDE			flags[5]			//	for guys who open doors, this is the side they are going after.
#define	CLOAKED			flags[6]			//	Cloaked now.
#define	SKIP_D1_AI_COUNT	flags[7]			//	Skip AI this frame, but decrement in do_D1_AI_frame.
#define  REMOTE_OWNER	flags[8]			// Who is controlling this remote AI object (multiplayer use only)
#define  REMOTE_SLOT_NUM flags[9]			// What slot # is this robot in for remote control purposes (multiplayer use only)
#define  MULTI_ANGER		flags[10]		// How angry is a robot in multiplayer mode


#define	D1_MAX_POINT_SEGS	2500

//	These are the information for a robot describing the location of the player last time he wasn't cloaked,
//	and the time at which he was uncloaked.  We should store this for each robot, but that's memory expensive.
//extern	fix			Last_uncloaked_time;
//extern	CFixVector	Last_uncloaked_position;

#endif
