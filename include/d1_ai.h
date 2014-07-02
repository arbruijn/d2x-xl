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

#ifndef _D1_AI_H
#define _D1_AI_H

#include "object.h"
#include "d1_aistruct.h"

#ifndef PLAYER_AWARENESS_INITIAL_TIME
#define	PLAYER_AWARENESS_INITIAL_TIME		I2X (3)
#endif
#define	MAX_PATH_LENGTH						30			//	Maximum length of path in ai path following.
#define	MAX_DEPTH_TO_SEARCH_FOR_PLAYER	10
#define	BOSS_GATE_PRODUCER_NUM					-1
#define	BOSS_ECLIP_NUM							53

#define	ROBOT_BRAIN	7
#define	ROBOT_BOSS1	17

int32_t Boss_dying;

void move_towards_segment_center(CObject *objP);
int32_t gate_in_robot(int32_t type, int32_t segnum);
void do_ai_movement(CObject *objP);
void ai_move_to_new_segment( CObject * obj, int16_t newseg, int32_t first_time );
// void ai_follow_path( CObject * obj, int16_t newseg, int32_t first_time );
void ai_recover_from_wall_hit(CObject *obj, int32_t segnum);
void ai_move_one(CObject *objP);
void do_ai_frame(CObject *objP);
void init_ai_object(int32_t objnum, int32_t initial_mode, int32_t hide_segment);
void update_player_awareness(CObject *objP, fix new_awareness);
void create_awareness_event(CObject *objP, int32_t type);			// CObject *objP can create awareness of player, amount based on "type"
void do_ai_frame_all(void);
void init_ai_system(void);
void reset_ai_states(CObject *objP);
void create_all_paths(void);
void create_path_to_station(CObject *objP, int32_t max_length);
void ai_follow_path(CObject *objP, int32_t player_visibility);
void ai_turn_towards_vector(CFixVector *vec_to_player, CObject *obj, fix rate);
void ai_turn_towards_vel_vec(CObject *objP, fix rate);
void init_ai_objects(void);
void do_ai_robot_hit(CObject *robot, int32_t type);
void create_n_segment_path(CObject *objP, int32_t path_length, int32_t avoid_seg);
void create_n_segment_path_to_door(CObject *objP, int32_t path_length, int32_t avoid_seg);
void init_robots_for_level(void);
int32_t ai_behavior_to_mode(int32_t behavior);
int32_t Robot_firing_enabled;

//	max_length is maximum depth of path to create.
//	If -1, use default:	MAX_DEPTH_TO_SEARCH_FOR_PLAYER
void create_path_to_player(CObject *objP, int32_t max_length, int32_t safety_flag);
void attempt_to_resume_path(CObject *objP);

//	When a robot and a player collide, some robots attack!
void do_ai_robot_hit_attack(CObject *robot, CObject *player, CFixVector *collision_point);
void ai_open_doors_in_segment(CObject *robot);
int32_t ai_door_is_openable(CObject *objP, CSegment *segp, int32_t sidenum);
int32_t player_is_visible_from_object(CObject *objP, CFixVector *pos, fix field_of_view, CFixVector *vec_to_player);
void ai_reset_all_paths(void);	//	Reset all paths.  Call at the start of a level.
int32_t ai_multiplayer_awareness(CObject *objP, int32_t awareness_level);

#if DBG
void force_dump_ai_objects_all(char *msg);
#else
#define force_dump_ai_objects_all(msg)
#endif

void start_boss_death_sequence(CObject *objP);
void ai_init_boss_for_ship(void);
int32_t Boss_been_hit;
extern fix AI_proc_time;

#endif
