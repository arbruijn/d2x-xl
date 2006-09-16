/* $Id: robot.c,v 1.5 2003/10/10 09:36:35 btb Exp $ */
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
 * Code for handling robots
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:30:34  allender
 * Initial revision
 *
 * Revision 2.1  1995/03/07  16:52:02  john
 * Fixed robots not moving without edtiro bug.
 *
 * Revision 2.0  1995/02/27  11:31:11  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.19  1995/02/22  13:58:09  allender
 * remove anonymous unions from object structure
 *
 * Revision 1.18  1995/01/27  11:17:06  rob
 * Avoid problems with illegal gun num.
 *
 * Revision 1.17  1994/11/19  15:15:02  mike
 * remove unused code and data
 *
 * Revision 1.16  1994/11/05  16:41:31  adam
 * upped MAX_ROBOT_JOINTS
 *
 * Revision 1.15  1994/09/26  15:29:29  matt
 * Allow morphing gameData.objs.objects to fire
 *
 * Revision 1.14  1994/06/20  14:31:02  matt
 * Don't include joint zero in animation data
 *
 * Revision 1.13  1994/06/10  14:39:58  matt
 * Increased limit of robot joints
 *
 * Revision 1.12  1994/06/10  10:59:18  matt
 * Do error checking on list of angles
 *
 * Revision 1.11  1994/06/09  16:21:32  matt
 * Took out special-case and test code.
 *
 * Revision 1.10  1994/06/07  13:21:14  matt
 * Added support for new chunk-based POF files, with robot animation data.
 *
 * Revision 1.9  1994/06/01  17:58:24  mike
 * Greater flinch effect.
 *
 * Revision 1.8  1994/06/01  14:59:25  matt
 * Fixed calc_gun_position(), which was rotating the wrong way for the
 * object orientation.
 *
 * Revision 1.7  1994/06/01  12:44:04  matt
 * Added flinch state for test robot
 *
 * Revision 1.6  1994/05/31  19:17:24  matt
 * Fixed test robot angles
 *
 * Revision 1.5  1994/05/30  19:43:50  mike
 * Call set_test_robot.
 *
 *
 * Revision 1.4  1994/05/30  00:02:44  matt
 * Got rid of robot render type, and generally cleaned up polygon model
 * render gameData.objs.objects.
 *
 * Revision 1.3  1994/05/29  18:46:15  matt
 * Added stuff for getting robot animation info for different states
 *
 * Revision 1.2  1994/05/26  21:09:15  matt
 * Moved robot stuff out of polygon model and into robot_info struct
 * Made new file, robot.c, to deal with robots
 *
 * Revision 1.1  1994/05/26  18:02:04  matt
 * Initial revision
 *
 *
 */


#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>

#include "error.h"

#include "inferno.h"

#include "robot.h"
#include "object.h"
#include "polyobj.h"
#include "mono.h"
#include "ai.h"
#include "interp.h"

//	-----------------------------------------------------------------------------------------------------------
//Big array of joint positions.  All robots index into this array

#define deg(a) ((int) (a) * 32768 / 180)

//test data for one robot
jointpos test_joints [MAX_ROBOT_JOINTS] = {

//gun 0
	{2,{deg(-30),0,0}},         //rest (2 joints)
	{3,{deg(-40),0,0}},

	{2,{deg(0),0,0}},           //alert
	{3,{deg(0),0,0}},

	{2,{deg(0),0,0}},           //fire
	{3,{deg(0),0,0}},

	{2,{deg(50),0,0}},          //recoil
	{3,{deg(-50),0,0}},

	{2,{deg(10),0,deg(70)}},    //flinch
	{3,{deg(0),deg(20),0}},

//gun 1
	{4,{deg(-30),0,0}},         //rest (2 joints)
	{5,{deg(-40),0,0}},

	{4,{deg(0),0,0}},           //alert
	{5,{deg(0),0,0}},

	{4,{deg(0),0,0}},           //fire
	{5,{deg(0),0,0}},

	{4,{deg(50),0,0}},          //recoil
	{5,{deg(-50),0,0}},

	{4,{deg(20),0,deg(-50)}},   //flinch
	{5,{deg(0),0,deg(20)}},

//rest of body (the head)

	{1,{deg(70),0,0}},          //rest (1 joint, head)

	{1,{deg(0),0,0}},           //alert

	{1,{deg(0),0,0}},           //fire

	{1,{deg(0),0,0}},           //recoil

	{1,{deg(-20),deg(15),0}},   //flinch

};

//	-----------------------------------------------------------------------------------------------------------
//given an object and a gun number, return position in 3-space of gun
//fills in gun_point
void calc_gun_point(vms_vector *gun_point,object *objP,int gun_num)
{
	polymodel *pm;
	robot_info *r;
	vms_vector pnt;
	vms_matrix m;
	int mn;				//submodel number

	Assert(objP->render_type==RT_POLYOBJ || objP->render_type==RT_MORPH);
	Assert(objP->id < gameData.bots.nTypes [gameStates.app.bD1Data]);

	r = &gameData.bots.pInfo[objP->id];
	pm =&gameData.models.polyModels [r->model_num];

	if (gun_num >= r->n_guns)
	{
		//Int3();
		gun_num = 0;
	}

//	Assert(gun_num < r->n_guns);

	pnt = r->gun_points[gun_num];
	mn = r->gun_submodels[gun_num];

	//instance up the tree for this gun
	while (mn != 0) {
		vms_vector tpnt;

		VmAngles2Matrix(&m,&objP->rtype.pobj_info.anim_angles[mn]);
		VmTransposeMatrix(&m);
		VmVecRotate(&tpnt,&pnt,&m);

		VmVecAdd(&pnt,&tpnt,&pm->submodel_offsets[mn]);

		mn = pm->submodel_parents[mn];
	}

	//now instance for the entire object

	VmCopyTransposeMatrix(&m,&objP->orient);
	VmVecRotate(gun_point,&pnt,&m);
	VmVecInc(gun_point,&objP->pos);

}

//	-----------------------------------------------------------------------------------------------------------
//fills in ptr to list of joints, and returns the number of joints in list
//takes the robot type (object id), gun number, and desired state
int robot_get_anim_state(jointpos **jp_list_ptr,int robot_type,int gun_num,int state)
{

	Assert(gun_num <= gameData.bots.pInfo[robot_type].n_guns);

	*jp_list_ptr = &gameData.bots.joints[gameData.bots.pInfo[robot_type].anim_states[gun_num][state].offset];

	return gameData.bots.pInfo[robot_type].anim_states[gun_num][state].n_joints;

}


//	-----------------------------------------------------------------------------------------------------------
//for test, set a robot to a specific state
void set_robot_state(object *objP,int state)
{
	int g,j,jo;
	robot_info *ri;
	jointlist *jl;

	Assert(objP->type == OBJ_ROBOT);

	ri = &gameData.bots.pInfo[objP->id];

	for (g=0;g<ri->n_guns+1;g++) {

		jl = &ri->anim_states[g][state];

		jo = jl->offset;

		for (j=0;j<jl->n_joints;j++,jo++) {
			int jn;

			jn = gameData.bots.joints[jo].jointnum;

			objP->rtype.pobj_info.anim_angles[jn] = gameData.bots.joints[jo].angles;

		}
	}
}

//	-----------------------------------------------------------------------------------------------------------
#include "mono.h"

//--unused-- int cur_state=0;

//--unused-- test_anim_states()
//--unused-- {
//--unused-- 	set_robot_state(&gameData.objs.objects[1],cur_state);
//--unused--
//--unused--
//--unused-- 	cur_state = (cur_state+1)%N_ANIM_STATES;
//--unused--
//--unused-- }

//set the animation angles for this robot.  Gun fields of robot info must
//be filled in.
void robot_set_angles(robot_info *r,polymodel *pm,vms_angvec angs[N_ANIM_STATES][MAX_SUBMODELS])
{
	int m,g,state;
	int gun_nums[MAX_SUBMODELS];			//which gun each submodel is part of

	for (m=0;m<pm->n_models;m++)
		gun_nums[m] = r->n_guns;		//assume part of body...

	gun_nums[0] = -1;		//body never animates, at least for now

	for (g=0;g<r->n_guns;g++) {
		m = r->gun_submodels[g];

		while (m != 0) {
			gun_nums[m] = g;				//...unless we find it in a gun
			m = pm->submodel_parents[m];
		}
	}

	for (g=0;g<r->n_guns+1;g++) {

		for (state=0;state<N_ANIM_STATES;state++) {

			r->anim_states[g][state].n_joints = 0;
			r->anim_states[g][state].offset = gameData.bots.nJoints;

			for (m=0;m<pm->n_models;m++) {
				if (gun_nums[m] == g) {
					gameData.bots.joints[gameData.bots.nJoints].jointnum = m;
					gameData.bots.joints[gameData.bots.nJoints].angles = angs[state][m];
					r->anim_states[g][state].n_joints++;
					gameData.bots.nJoints++;
					Assert(gameData.bots.nJoints < MAX_ROBOT_JOINTS);
				}
			}
		}
	}
}

//	-----------------------------------------------------------------------------------------------------------

#define DEG90	(F1_0 / 4)

void InitCamBots (int bReset)
{
	object		*objP = gameData.objs.objects;
	int			i;
	vms_vector	av = {DEG90,DEG90,DEG90};

if ((gameData.bots.nCamBotId < 0) || gameStates.app.bD1Mission)
	return;
gameData.bots.info [0][gameData.bots.nCamBotId].model_num = gameData.bots.nCamBotModel;
gameData.bots.info [0][gameData.bots.nCamBotId].attack_type = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].contains_id = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].contains_count = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].contains_prob = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].contains_type = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].score_value = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].strength = -1;
gameData.bots.info [0][gameData.bots.nCamBotId].mass = F1_0 / 2;
gameData.bots.info [0][gameData.bots.nCamBotId].drag = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].see_sound = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].attack_sound = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].claw_sound = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].taunt_sound = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].behavior = AIB_STILL;
gameData.bots.info [0][gameData.bots.nCamBotId].aim = AIM_STILL;
memset (gameData.bots.info [0][gameData.bots.nCamBotId].turn_time, 0, sizeof (gameData.bots.info [0][gameData.bots.nCamBotId].turn_time));
memset (gameData.bots.info [0][gameData.bots.nCamBotId].max_speed, 0, sizeof (gameData.bots.info [0][gameData.bots.nCamBotId].max_speed));
memset (gameData.bots.info [0][gameData.bots.nCamBotId].circle_distance, 0, sizeof (gameData.bots.info [0][gameData.bots.nCamBotId].circle_distance));
memset (gameData.bots.info [0][gameData.bots.nCamBotId].rapidfire_count, 0, sizeof (gameData.bots.info [0][gameData.bots.nCamBotId].rapidfire_count));
for (i = 0; i <= gameData.objs.nLastObject; i++, objP++)
	if (objP->type == OBJ_CAMBOT) {
		//objP->type = OBJ_ROBOT;
		objP->id	= gameData.bots.nCamBotId;
		objP->size = G3PolyModelSize (gameData.models.polyModels [gameData.bots.nCamBotModel].model_data);
		objP->lifeleft = IMMORTAL_TIME;
		objP->control_type = CT_CAMERA;
		objP->movement_type = MT_NONE;
		objP->rtype.pobj_info.model_num = gameData.bots.nCamBotModel;
#if 0
		VmExtractAnglesMatrix (&a, &objP->orient);
		av.x = a.h;
		av.y = a.b;
		av.z = a.p;
		G3StartInstanceMatrix (&objP->pos, &objP->orient);
		VmVecRotate (&objP->mtype.spin_rate, &av, &objP->orient);
		G3DoneInstance ();
		h = a.b + a.p;
		objP->mtype.spin_rate.x = a.p; //(a.h < 0) ? (a.b < 0) ? -h : h : (a.b < 0) ? h : -h;
		objP->mtype.spin_rate.y = a.h; //(a.b < 0) ? -(DEG90 + a.b) : DEG90 - a.b;
		objP->mtype.spin_rate.z = a.b; //(a.p < 0) ? -(DEG90 + a.p) : DEG90 - a.p;
		objP->mtype.phys_info.brakes = -1;	// used as timeout counter
		//if (!bReset) 
			{
			objP->mtype.phys_info.turnroll = 0;	// used as current heading angle rel. to start pos
			objP->mtype.phys_info.velocity.x = 0;
			}
		//objP->ctype.ai_info.behavior = AIB_NORMAL; //AIB_STILL;
#endif
		gameData.ai.localInfo [i].mode = AIM_STILL;
		}
}

//	-----------------------------------------------------------------------------------------------------------

void UnloadCamBot (void)
{
if (gameData.bots.nCamBotId >= 0) {
	gameData.bots.nTypes [0] = gameData.bots.nCamBotId;
	gameData.bots.nCamBotId = -1;
	}
}

//	-----------------------------------------------------------------------------------------------------------

#ifndef FAST_FILE_IO
/*
 * reads n jointlist structs from a CFILE
 */
static int jointlist_read_n(jointlist *jl, int n, CFILE *fp)
{
	int i;

	for (i = 0; i < n; i++) {
		jl[i].n_joints = CFReadShort(fp);
		jl[i].offset = CFReadShort(fp);
	}
	return i;
}

//	-----------------------------------------------------------------------------------------------------------
/*
 * reads n robot_info structs from a CFILE
 */
int robot_info_read_n(robot_info *ri, int n, CFILE *fp)
{
	int i, j;

	for (i = 0; i < n; i++) {
		ri[i].model_num = CFReadInt(fp);
		for (j = 0; j < MAX_GUNS; j++)
			CFReadVector(&(ri[i].gun_points[j]), fp);
		CFRead(ri[i].gun_submodels, MAX_GUNS, 1, fp);

		ri[i].exp1_vclip_num = CFReadShort(fp);
		ri[i].exp1_sound_num = CFReadShort(fp);

		ri[i].exp2_vclip_num = CFReadShort(fp);
		ri[i].exp2_sound_num = CFReadShort(fp);

		ri[i].weapon_type = CFReadByte(fp);
		ri[i].weapon_type2 = CFReadByte(fp);
		ri[i].n_guns = CFReadByte(fp);
		ri[i].contains_id = CFReadByte(fp);

		ri[i].contains_count = CFReadByte(fp);
		ri[i].contains_prob = CFReadByte(fp);
		ri[i].contains_type = CFReadByte(fp);
		ri[i].kamikaze = CFReadByte(fp);

		ri[i].score_value = CFReadShort(fp);
		ri[i].badass = CFReadByte(fp);
		ri[i].energy_drain = CFReadByte(fp);

		ri[i].lighting = CFReadFix(fp);
		ri[i].strength = CFReadFix(fp);

		ri[i].mass = CFReadFix(fp);
		ri[i].drag = CFReadFix(fp);

		for (j = 0; j < NDL; j++)
			ri[i].field_of_view[j] = CFReadFix(fp);
		for (j = 0; j < NDL; j++)
			ri[i].firing_wait[j] = CFReadFix(fp);
		for (j = 0; j < NDL; j++)
			ri[i].firing_wait2[j] = CFReadFix(fp);
		for (j = 0; j < NDL; j++)
			ri[i].turn_time[j] = CFReadFix(fp);
		for (j = 0; j < NDL; j++)
			ri[i].max_speed[j] = CFReadFix(fp);
		for (j = 0; j < NDL; j++)
			ri[i].circle_distance[j] = CFReadFix(fp);
		CFRead(ri[i].rapidfire_count, NDL, 1, fp);

		CFRead(ri[i].evade_speed, NDL, 1, fp);

		ri[i].cloak_type = CFReadByte(fp);
		ri[i].attack_type = CFReadByte(fp);

		ri[i].see_sound = CFReadByte(fp);
		ri[i].attack_sound = CFReadByte(fp);
		ri[i].claw_sound = CFReadByte(fp);
		ri[i].taunt_sound = CFReadByte(fp);

		ri[i].boss_flag = CFReadByte(fp);
		ri[i].companion = CFReadByte(fp);
		ri[i].smart_blobs = CFReadByte(fp);
		ri[i].energy_blobs = CFReadByte(fp);

		ri[i].thief = CFReadByte(fp);
		ri[i].pursuit = CFReadByte(fp);
		ri[i].lightcast = CFReadByte(fp);
		ri[i].death_roll = CFReadByte(fp);

		ri[i].flags = CFReadByte(fp);
		CFRead(ri[i].pad, 3, 1, fp);

		ri[i].deathroll_sound = CFReadByte(fp);
		ri[i].glow = CFReadByte(fp);
		ri[i].behavior = CFReadByte(fp);
		ri[i].aim = CFReadByte(fp);

		for (j = 0; j < MAX_GUNS + 1; j++)
			jointlist_read_n(ri[i].anim_states[j], N_ANIM_STATES, fp);

		ri[i].always_0xabcd = CFReadInt(fp);
	}
	return i;
}

//	-----------------------------------------------------------------------------------------------------------
/*
 * reads n jointpos structs from a CFILE
 */
int jointpos_read_n(jointpos *jp, int n, CFILE *fp)
{
	int i;

	for (i = 0; i < n; i++) {
		jp[i].jointnum = CFReadShort(fp);
		CFReadAngVec(&jp[i].angles, fp);
	}
	return i;
}
#endif

//	-----------------------------------------------------------------------------------------------------------
//eof