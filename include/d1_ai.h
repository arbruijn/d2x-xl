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

#define	PLAYER_AWARENESS_INITIAL_TIME		(3*F1_0)
#define	MAX_PATH_LENGTH						30			//	Maximum length of path in ai path following.
#define	MAX_DEPTH_TO_SEARCH_FOR_PLAYER	10
#define	BOSS_GATE_MATCEN_NUM					-1
#define	BOSS_ECLIP_NUM							53

#define	ROBOT_BRAIN	7
#define	ROBOT_BOSS1	17

int Boss_dying;

void move_towards_segment_center(tObject *objP);
int gate_in_robot(int type, int segnum);
void do_ai_movement(tObject *objP);
void ai_move_to_new_segment( tObject * obj, short newseg, int first_time );
// void ai_follow_path( tObject * obj, short newseg, int first_time );
void ai_recover_from_wall_hit(tObject *obj, int segnum);
void ai_move_one(tObject *objP);
void do_ai_frame(tObject *objP);
void init_ai_object(int objnum, int initial_mode, int hide_segment);
void update_player_awareness(tObject *objP, fix new_awareness);
void create_awareness_event(tObject *objP, int type);			// tObject *objP can create awareness of player, amount based on "type"
void do_ai_frame_all(void);
void init_ai_system(void);
void reset_ai_states(tObject *objP);
void create_all_paths(void);
void create_path_to_station(tObject *objP, int max_length);
void ai_follow_path(tObject *objP, int player_visibility);
void ai_turn_towards_vector(vmsVector *vec_to_player, tObject *obj, fix rate);
void ai_turn_towards_vel_vec(tObject *objP, fix rate);
void init_ai_objects(void);
void do_ai_robot_hit(tObject *robot, int type);
void create_n_segment_path(tObject *objP, int path_length, int avoid_seg);
void create_n_segment_path_to_door(tObject *objP, int path_length, int avoid_seg);
void init_robots_for_level(void);
int ai_behavior_to_mode(int behavior);
int Robot_firing_enabled;

//	max_length is maximum depth of path to create.
//	If -1, use default:	MAX_DEPTH_TO_SEARCH_FOR_PLAYER
void create_path_to_player(tObject *objP, int max_length, int safety_flag);
void attempt_to_resume_path(tObject *objP);

//	When a robot and a player collide, some robots attack!
void do_ai_robot_hit_attack(tObject *robot, tObject *player, vmsVector *collision_point);
void ai_open_doors_in_segment(tObject *robot);
int ai_door_is_openable(tObject *objP, tSegment *segp, int sidenum);
int player_is_visible_from_object(tObject *objP, vmsVector *pos, fix field_of_view, vmsVector *vec_to_player);
void ai_reset_all_paths(void);	//	Reset all paths.  Call at the start of a level.
int ai_multiplayer_awareness(tObject *objP, int awareness_level);

#ifndef NDEBUG
void force_dump_ai_objects_all(char *msg);
#else
#define force_dump_ai_objects_all(msg)
#endif

void start_boss_death_sequence(tObject *objP);
void ai_init_boss_for_ship(void);
int Boss_been_hit;
extern fix AI_proc_time;

#endif
