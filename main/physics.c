/* $Id: physics.c, v 1.4 2003/10/10 09:36:35 btb Exp $ */
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
 * Code for flying through the mines
 *
 * Old Log:
 * Revision 1.5  1995/10/12  17:28:08  allender
 * put in code to move and object to center of segment in
 * do_physics sim when fvi fails with bad point
 *
 * Revision 1.4  1995/08/23  21:32:44  allender
 * fix mcc compiler warnings
 *
 * Revision 1.3  1995/07/28  15:38:56  allender
 * removed isqrt thing -- not required here
 *
 * Revision 1.2  1995/07/28  15:13:29  allender
 * fixed vector magnitude thing
 *
 * Revision 1.1  1995/05/16  15:29:42  allender
 * Initial revision
 *
 * Revision 2.2  1995/03/24  14:48:54  john
 * Added cheat for player to go thru walls.
 *
 * Revision 2.1  1995/03/20  18:15:59  john
 * Added code to not store the normals in the segment structure.
 *
 * Revision 2.0  1995/02/27  11:32:06  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.213  1995/02/22  13:40:48  allender
 * remove anonymous unions from object structure
 *
 * Revision 1.212  1995/02/22  13:24:42  john
 * Removed the vecmat anonymous unions.
 *
 * Revision 1.211  1995/02/06  19:46:59  matt
 * New function (untested), set_thrust_from_velocity()
 *
 * Revision 1.210  1995/02/02  16:26:12  matt
 * Changed assert that was causing a problem
 *
 * Revision 1.209  1995/02/02  14:07:00  matt
 * Fixed confusion about which segment you are touching when you're
 * touching a wall.  This manifested itself in spurious lava burns.
 *
 * Revision 1.208  1995/02/01  21:03:24  john
 * Lintified.
 *
 * Revision 1.207  1995/01/25  13:53:35  rob
 * Removed an Int3 from multiplayer games.
 *
 * Revision 1.206  1995/01/23  17:30:47  rob
 * Removed Int3 on bogus sim time.
 *
 * Revision 1.205  1995/01/17  11:08:56  matt
 * Disable new-ish FVI edge checking for all gameData.objs.objects except the player, 
 * since it was causing problems with the fusion cannon.
 *
 * Revision 1.204  1995/01/05  09:43:49  matt
 * Took out int3s from new code
 *
 * Revision 1.203  1995/01/04  22:19:23  matt
 * Added hack to keep player from squeezing through closed walls/doors
 *
 * Revision 1.202  1995/01/02  12:38:48  mike
 * physics hack to crazy josh not get hung up on proximity bombs.  Matt notified via email.
 *
 * Revision 1.201  1994/12/13  15:39:22  mike
 * #ifndef NDEBUG some code.
 *
 * Revision 1.200  1994/12/13  13:28:34  yuan
 * Fixed type.
 *
 * Revision 1.199  1994/12/13  13:25:00  matt
 * Made bump hack compile out if so desired
 *
 * Revision 1.198  1994/12/13  12:02:39  matt
 * Added hack to bump player a little if stuck
 *
 * Revision 1.197  1994/12/12  00:32:23  matt
 * When gameData.objs.objects other than player go out of mine, jerk to center of segment
 *
 * Revision 1.196  1994/12/10  22:52:42  mike
 * make physics left-the-mine checking always be in.
 *
 * Revision 1.195  1994/12/08  00:53:01  mike
 * oops...phys rot bug.
 *
 * Revision 1.194  1994/12/07  12:54:54  mike
 * tweak rotvel applied from collisions.
 *
 * Revision 1.193  1994/12/07  00:36:08  mike
 * fix PhysApplyRot for robots -- ai was bashing effect in next frame.
 *
 * Revision 1.192  1994/12/05  17:23:10  matt
 * Made a bunch of debug code compile out
 *
 * Revision 1.191  1994/12/05  16:30:10  matt
 * Was illegally changing an object's segment...shoot me.
 *
 * Revision 1.190  1994/12/05  11:58:51  mike
 * fix stupid apply_force_rot() bug.
 *
 * Revision 1.189  1994/12/05  09:42:17  mike
 * fix 0 mag problem when object applies force to itself.
 *
 * Revision 1.188  1994/12/04  22:48:40  matt
 * Physics & FVI now only build seglist for player gameData.objs.objects, and they
 * responsilby deal with buffer full conditions
 *
 * Revision 1.187  1994/12/04  22:14:07  mike
 * apply instantaneous rotation to an object due to a force blow.
 *
 * Revision 1.186  1994/12/04  18:51:30  matt
 * When weapons get stuck, delete them!
 *
 * Revision 1.185  1994/12/04  18:38:56  matt
 * Added better handling of point-not-in-seg problem
 *
 * Revision 1.184  1994/11/27  23:13:42  matt
 * Made changes for new con_printf calling convention
 *
 * Revision 1.183  1994/11/25  23:46:18  matt
 * Fixed drag problems with framerates over 60Hz
 *
 * Revision 1.182  1994/11/25  22:15:52  matt
 * Added asserts to try to trap frametime < 0 bug
 *
 * Revision 1.181  1994/11/21  11:42:44  mike
 * ndebug stuff.
 *
 * Revision 1.180  1994/11/19  15:15:04  mike
 * remove unused code and data
 *
 * Revision 1.179  1994/11/16  11:25:22  matt
 * Abort physics if negative frametime
 *
 * Revision 1.178  1994/10/05  19:50:41  rob
 * Removed a non-critical Int3 where an object's segnum is checked.
 * Left con_printf message.
 *
 * Revision 1.177  1994/10/03  22:57:50  matt
 * Fixed problem with matrix corruption of non-moving (but rotating) gameData.objs.objects
 *
 * Revision 1.176  1994/09/28  09:23:28  mike
 * Add useful information to con_printf(1, ... error messages.
 *
 * Revision 1.175  1994/09/21  17:16:54  mike
 * Make gameData.objs.objects stuck in doors go away when door opens.
 *
 * Revision 1.174  1994/09/12  14:19:06  matt
 * Drag & thrust now handled differently
 *
 * Revision 1.173  1994/09/09  14:21:12  matt
 * Use new thrust flag
 *
 * Revision 1.172  1994/09/08  16:21:57  matt
 * Cleaned up player-hit-wall code, and added object scrape handling
 * Also added weapon-on-weapon hit sound
 *
 * Revision 1.171  1994/09/02  12:30:37  matt
 * Fixed weapons which go through gameData.objs.objects
 *
 * Revision 1.170  1994/09/02  11:55:14  mike
 * Kill redefinition of a constant which is properly defined in object.h
 *
 * Revision 1.169  1994/09/02  11:35:01  matt
 * Fixed typo
 *
 * Revision 1.168  1994/09/02  11:32:48  matt
 * Fixed object/object collisions, so you can't fly through robots anymore.
 * Cleaned up object damage system.
 *
 * Revision 1.167  1994/08/30  21:58:15  matt
 * Made PhysApplyForce() do nothing to an object if it's not a phys object
 *
 * Revision 1.166  1994/08/26  10:47:01  john
 * New version of controls.
 *
 * Revision 1.165  1994/08/25  21:53:57  mike
 * Prevent counts of -1 which eventually add up to a positive number in DoAIFrame, causing
 * the too-many-retries behavior.
 *
 * Revision 1.164  1994/08/25  18:43:33  john
 * First revision of new control code.
 *
 * Revision 1.163  1994/08/17  22:18:05  mike
 * Make robots which have rotvel or rotthrust, but not movement, move.
 *
 * Revision 1.162  1994/08/13  17:31:18  mike
 * retry count stuff.
 *
 * Revision 1.161  1994/08/11  18:59:16  mike
 * *** empty log message ***
 *
 * Revision 1.160  1994/08/10  19:53:47  mike
 * Debug code (which is still in...)
 * and adapt to changed interface to create_path_to_player.
 *
 * Revision 1.159  1994/08/08  21:38:43  matt
 * Cleaned up a code a little and optimized a little
 *
 * Revision 1.158  1994/08/08  15:21:50  mike
 * Trap retry count >= 4, but don't do AI hack unless >= 6.
 *
 * Revision 1.157  1994/08/08  11:47:15  matt
 * Cleaned up fvi and physics a little
 *
 * Revision 1.156  1994/08/05  10:10:10  yuan
 * Commented out debug stuff that was killing framerate.
 *
 * Revision 1.155  1994/08/04  19:12:36  matt
 * Changed a bunch of vecmat calls to use multiple-function routines, and to
 * allow the use of C macros for some functions
 *
 * Revision 1.154  1994/08/04  16:33:57  mike
 * Kill a pile of RCS stuff.
 * Call create_path_to_player for a stuck object.
 *
 * Revision 1.153  1994/08/04  00:21:02  matt
 * Cleaned up fvi & physics error handling; put in code to make sure gameData.objs.objects
 * are in correct segment; simplified segment finding for gameData.objs.objects and points
 *
 * Revision 1.152  1994/08/01  16:25:34  matt
 * Check for moved_time == 0 when computing hit speed
 *
 * Revision 1.151  1994/08/01  13:30:32  matt
 * Made fvi() check holes in transparent walls, and changed fvi() calling
 * parms to take all input data in query structure.
 *
 * Revision 1.150  1994/07/29  23:41:46  matt
 * Fixed turn banking, which changed when I added rotational velocity
 *
 * Revision 1.149  1994/07/27  20:53:23  matt
 * Added rotational drag & thrust, so turning now has momemtum like moving
 *
 */


#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: physics.c, v 1.4 2003/10/10 09:36:35 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>

#include "joy.h"
#include "mono.h"
#include "error.h"

#include "inferno.h"
#include "segment.h"
#include "object.h"
#include "physics.h"
#include "key.h"
#include "game.h"
#include "collide.h"
#include "fvi.h"
#include "newdemo.h"
#include "timer.h"
#include "ai.h"
#include "wall.h"
#include "switch.h"
#include "laser.h"
#include "bm.h"
#include "player.h"
#include "network.h"
#include "hudmsg.h"
#include "gameseg.h"

#ifdef TACTILE
#include "tactile.h"
#endif

//Global variables for physics system

#define FLUID_PHYSICS	0

#define ROLL_RATE 		0x2000
#define DAMP_ANG 			0x400                  //min angle to bank

#define TURNROLL_SCALE	(0x4ec4/2)

#define MAX_OBJECT_VEL i2f(100)

#define BUMP_HACK	1		//if defined, bump player when he gets stuck

//--unused-- int mike_mode=0;

//check point against each side of segment. return bitmask, where bit
//set means behind that side


//##//returns the distance of a point (checkp) from a plane (defined by norm & planep)
//##fix dist_to_plane(vms_vector *checkp, vms_vector *norm, vms_vector *planep)
//##{
//##    vms_vector deltap;
//##
//##    VmVecSub(&deltap, checkp, planep);
//##
//##    return VmVecDot(&deltap, norm);
//##}

//--unused-- int dpjm_old_joy_x, dpjm_old_joy_y;

int floor_levelling=0;

//--unused-- level_with_floor()
//--unused-- {
//--unused-- 	floor_levelling=1;
//--unused-- }

//make sure matrix is orthogonal
void check_and_fix_matrix(vms_matrix *m)
{
	vms_matrix tempm;

	VmVector2Matrix(&tempm, &m->fvec, &m->uvec, NULL);
	*m = tempm;
}


void do_physics_align_object(object * objP)
{
	vms_vector desired_upvec;
	fixang delta_ang, roll_ang;
	//vms_vector forvec = {0, 0, f1_0};
	vms_matrix temp_matrix;
	fix d, largest_d=-f1_0;
	int i, best_side;

        best_side=0;
	// bank player according to segment orientation

	//find side of segment that player is most alligned with

	for (i=0;i<6;i++) {
		#ifdef COMPACT_SEGS
			vms_vector _tv1;
			get_side_normal(gameData.segs.segments + objP->segnum, i, 0, &_tv1);
			d = VmVecDot(&_tv1, &objP->orient.uvec);
		#else					
			d = VmVecDot(gameData.segs.segments [objP->segnum].sides[i].normals, &objP->orient.uvec);
		#endif

		if (d > largest_d) {largest_d = d; best_side=i;}
	}

	if (floor_levelling) {

		// old way: used floor's normal as upvec
		#ifdef COMPACT_SEGS
			get_side_normal(gameData.segs.segments + objP->segnum, 3, 0, &desired_upvec);			
		#else
			desired_upvec = gameData.segs.segments[objP->segnum].sides[3].normals[0];
		#endif

	}
	else  // new player leveling code: use normal of side closest to our up vec
		if (get_num_faces(&gameData.segs.segments[objP->segnum].sides[best_side])==2) {
			#ifdef COMPACT_SEGS
				vms_vector normals[2];
				get_side_normals(&gameData.segs.segments[objP->segnum], best_side, &normals[0], &normals[1]);			

				desired_upvec.x = (normals[0].x + normals[1].x) / 2;
				desired_upvec.y = (normals[0].y + normals[1].y) / 2;
				desired_upvec.z = (normals[0].z + normals[1].z) / 2;

				VmVecNormalize(&desired_upvec);
			#else
				side *s = &gameData.segs.segments[objP->segnum].sides[best_side];
				desired_upvec.x = (s->normals[0].x + s->normals[1].x) / 2;
				desired_upvec.y = (s->normals[0].y + s->normals[1].y) / 2;
				desired_upvec.z = (s->normals[0].z + s->normals[1].z) / 2;
		
				VmVecNormalize(&desired_upvec);
			#endif
		}
		else
			#ifdef COMPACT_SEGS
				get_side_normal(&gameData.segs.segments[objP->segnum], best_side, 0, &desired_upvec);			
			#else
				desired_upvec = gameData.segs.segments[objP->segnum].sides[best_side].normals[0];
			#endif

	if (labs(VmVecDot(&desired_upvec, &objP->orient.fvec)) < f1_0/2) {
		fixang save_delta_ang;
		vms_angvec tangles;
		
		VmVector2Matrix(&temp_matrix, &objP->orient.fvec, &desired_upvec, NULL);

		save_delta_ang = delta_ang = VmVecDeltaAng(&objP->orient.uvec, &temp_matrix.uvec, &objP->orient.fvec);

		delta_ang += objP->mtype.phys_info.turnroll;

		if (abs(delta_ang) > DAMP_ANG) {
			vms_matrix rotmat, new_pm;

			roll_ang = fixmul(gameData.app.xFrameTime, ROLL_RATE);

			if (abs(delta_ang) < roll_ang) roll_ang = delta_ang;
			else if (delta_ang<0) roll_ang = -roll_ang;

			tangles.p = tangles.h = 0;  tangles.b = roll_ang;
			VmAngles2Matrix(&rotmat, &tangles);

			VmMatMul(&new_pm, &objP->orient, &rotmat);
			objP->orient = new_pm;
		}
		else floor_levelling=0;
	}

}

void set_object_turnroll(object *objP)
{
//if (!gameStates.app.bD1Mission) 
	{
	fixang desired_bank = -fixmul(objP->mtype.phys_info.rotvel.y, TURNROLL_SCALE);
	if (objP->mtype.phys_info.turnroll != desired_bank) {
		fixang delta_ang, max_roll;
		max_roll = fixmul(ROLL_RATE, gameData.app.xFrameTime);
		delta_ang = desired_bank - objP->mtype.phys_info.turnroll;
		if (labs(delta_ang) < max_roll)
			max_roll = delta_ang;
		else
			if (delta_ang < 0)
				max_roll = -max_roll;
		objP->mtype.phys_info.turnroll += max_roll;
		}
	}
}

//list of segments went through
int phys_seglist[MAX_FVI_SEGS], n_phys_segs;


#define MAX_IGNORE_OBJS 100

#ifndef NDEBUG
#define EXTRA_DEBUG 1		//no extra debug when NDEBUG is on
#endif

#ifdef EXTRA_DEBUG
object *debugObjP=NULL;
#endif

#define XYZ(v) (v)->x, (v)->y, (v)->z


#ifndef NDEBUG
int	Total_retries=0, Total_sims=0;
int	Dont_move_ai_objects=0;
#endif

#define FT (f1_0/64)

extern int disable_new_fvi_stuff;
//	-----------------------------------------------------------------------------------------------------------
// add rotational velocity & acceleration
void do_physics_sim_rot(object *objP)
{
	vms_angvec	tangles;
	vms_matrix	rotmat, new_orient;
	//fix			rotdrag_scale;
	physics_info *pi;

#if 0
	Assert(gameData.app.xFrameTime > 0); 		//Get MATT if hit this!
#else
	if (gameData.app.xFrameTime <= 0)
		return;
#endif

	pi = &objP->mtype.phys_info;

	if (!(pi->rotvel.x || pi->rotvel.y || pi->rotvel.z || pi->rotthrust.x || pi->rotthrust.y || pi->rotthrust.z))
		return;

	if (objP->mtype.phys_info.drag) {
		int count;
		vms_vector accel;
		fix drag, r, k;

		count = gameData.app.xFrameTime / FT;
		r = gameData.app.xFrameTime % FT;
		k = fixdiv(r, FT);

		drag = (objP->mtype.phys_info.drag*5)/2;

		if (objP->mtype.phys_info.flags & PF_USES_THRUST) {

			VmVecCopyScale(&accel, 
				&objP->mtype.phys_info.rotthrust, 
				fixdiv(f1_0, objP->mtype.phys_info.mass));

			while (count--) {

				VmVecInc(&objP->mtype.phys_info.rotvel, &accel);

				VmVecScale(&objP->mtype.phys_info.rotvel, f1_0-drag);
			}

			//do linear scale on remaining bit of time

			VmVecScaleInc(&objP->mtype.phys_info.rotvel, &accel, k);
			VmVecScale(&objP->mtype.phys_info.rotvel, f1_0-fixmul(k, drag));
		}
		else if (! (objP->mtype.phys_info.flags & PF_FREE_SPINNING)) {
			fix total_drag=f1_0;

			while (count--)
				total_drag = fixmul(total_drag, f1_0-drag);

			//do linear scale on remaining bit of time

			total_drag = fixmul(total_drag, f1_0-fixmul(k, drag));

			VmVecScale(&objP->mtype.phys_info.rotvel, total_drag);
		}

	}

	//now rotate object

	//unrotate object for bank caused by turn
	if (objP->mtype.phys_info.turnroll) {
		vms_matrix new_pm;

		tangles.p = tangles.h = 0;
		tangles.b = -objP->mtype.phys_info.turnroll;
		VmAngles2Matrix(&rotmat, &tangles);
		VmMatMul(&new_pm, &objP->orient, &rotmat);
		objP->orient = new_pm;
	}

	tangles.p = fixmul(objP->mtype.phys_info.rotvel.x, gameData.app.xFrameTime);
	tangles.h = fixmul(objP->mtype.phys_info.rotvel.y, gameData.app.xFrameTime);
	tangles.b = fixmul(objP->mtype.phys_info.rotvel.z, gameData.app.xFrameTime);

	VmAngles2Matrix(&rotmat, &tangles);
	VmMatMul(&new_orient, &objP->orient, &rotmat);
	objP->orient = new_orient;

	if (objP->mtype.phys_info.flags & PF_TURNROLL)
		set_object_turnroll(objP);

	//re-rotate object for bank caused by turn
	if (objP->mtype.phys_info.turnroll) {
		vms_matrix new_pm;

		tangles.p = tangles.h = 0;
		tangles.b = objP->mtype.phys_info.turnroll;
		VmAngles2Matrix(&rotmat, &tangles);
		VmMatMul(&new_pm, &objP->orient, &rotmat);
		objP->orient = new_pm;
	}

	check_and_fix_matrix(&objP->orient);
}

//	-----------------------------------------------------------------------------------------------------------

extern object *monsterballP;
extern vms_vector boostedVel, minBoostedVel, maxBoostedVel;

//Simulate a physics object for this frame
void DoPhysicsSim(object *objP)
{
	short ignore_obj_list[MAX_IGNORE_OBJS], n_ignore_objs;
	int iseg;
	int try_again;
	int fate;
	vms_vector frame_vec;			//movement in this frame
	vms_vector new_pos, ipos;		//position after this frame
	int count=0;
	short objnum;
	short WallHitSeg, WallHitSide;
	fvi_info hit_info;
	fvi_query fq;
	vms_vector save_pos;
	int save_seg;
	fix drag;
	fix sim_time, old_sim_time, time_scale;
	vms_vector start_pos;
	int obj_stopped=0;
	fix moved_time;			//how long objected moved before hit something
	vms_vector save_p0, save_p1;
	physics_info *pi;
	short orig_segnum = objP->segnum;
	int bounced=0;
	int bDoSpeedBoost = gameStates.gameplay.bSpeedBoost && (objP == gameData.objs.console);

	Assert(objP->type != OBJ_NONE);
	Assert(objP->movement_type == MT_PHYSICS);

#ifndef NDEBUG
if (Dont_move_ai_objects)
	if (objP->control_type == CT_AI)
		return;
#endif
	pi = &objP->mtype.phys_info;
	do_physics_sim_rot(objP);
	if (!(pi->velocity.x || pi->velocity.y || pi->velocity.z)) {
		if (objP == gameData.objs.console)
			gameStates.gameplay.bSpeedBoost = 0;
		if (!(pi->thrust.x || pi->thrust.y || pi->thrust.z))
			return;
		}
	objnum = OBJ_IDX (objP);
	n_phys_segs = 0;
	disable_new_fvi_stuff = (objP->type != OBJ_PLAYER);
	sim_time = gameData.app.xFrameTime;
//debugObjP = obj;
#ifdef EXTRA_DEBUG
	if (objP == debugObjP) {
		//printf("object %d:\n  start pos = %x %x %x\n", objnum, XYZ(&objP->pos));
		//printf("  thrust = %x %x %x\n", XYZ(&objP->mtype.phys_info.thrust));
		//printf("  sim_time = %x\n", sim_time);
	}
	//check for correct object segment
	if(GetSegMasks(&objP->pos, objP->segnum, 0).centermask!=0) {
#if TRACE				
		con_printf (CON_DEBUG, "Warning: object %d not in given seg!\n", objnum);
#endif
		//Int3();  Removed by Rob 10/5/94
		if (!UpdateObjectSeg(objP)) {
#if TRACE				
			con_printf (CON_DEBUG, "Warning: can't find seg for object %d - moving\n", objnum);
#endif
			if (!(gameData.app.nGameMode & GM_MULTI))
				Int3();
			COMPUTE_SEGMENT_CENTER_I (&objP->pos, objP->segnum);
			objP->pos.x += objnum;
		}
	}
	#endif
	start_pos = objP->pos;
	n_ignore_objs = 0;
	Assert(objP->mtype.phys_info.brakes==0);		//brakes not used anymore?
	//if uses thrust, cannot have zero drag
	Assert(!(objP->mtype.phys_info.flags&PF_USES_THRUST) || objP->mtype.phys_info.drag!=0);
	//do thrust & drag
	//if (!bDoSpeedBoost) 
	{
		if (drag = objP->mtype.phys_info.drag) {

			int count;
			vms_vector accel, *vel = &objP->mtype.phys_info.velocity;
			fix r, k, d, a;

			d = f1_0 - drag;
			count = sim_time / FT;
			r = sim_time % FT;
			k = fixdiv (r, FT);
			if (objP->mtype.phys_info.flags & PF_USES_THRUST) {
				VmVecCopyScale (&accel, &objP->mtype.phys_info.thrust, fixdiv (f1_0, objP->mtype.phys_info.mass));
				a = (accel.x || accel.y || accel.z);
				if (bDoSpeedBoost && !(a || gameStates.input.bControlsSkipFrame))
					*vel = boostedVel;
				else {
					while (count--) {
						if (a)
							VmVecInc (vel, &accel);
						VmVecScale (vel, d);
					}
					//do linear scale on remaining bit of time
					VmVecScaleInc (vel, &accel, k);
					VmVecScale (vel, f1_0 - fixmul (k, drag));
					if (bDoSpeedBoost) {
						if (vel->x < minBoostedVel.x)
							vel->x = minBoostedVel.x;
						else if (vel->x > maxBoostedVel.x)
							vel->x = maxBoostedVel.x;
						if (vel->y < minBoostedVel.y)
							vel->y = minBoostedVel.y;
						else if (vel->y > maxBoostedVel.y)
							vel->y = maxBoostedVel.y;
						if (vel->z < minBoostedVel.z)
							vel->z = minBoostedVel.z;
						else if (vel->z > maxBoostedVel.z)
							vel->z = maxBoostedVel.z;
					}
				}
			}
			else {
				fix total_drag = f1_0;
				while (count--)
					total_drag = fixmul(total_drag, d);
				//do linear scale on remaining bit of time
				total_drag = fixmul(total_drag, f1_0-fixmul(k, drag));
				VmVecScale(&objP->mtype.phys_info.velocity, total_drag);
			}
		}
	}
#ifdef EXTRA_DEBUG
	if (objP == debugObjP)
		//printf("  velocity = %x %x %x\n", XYZ(&objP->mtype.phys_info.velocity))
		;
#endif
#if FLUID_PHYSICS
	if (EGI_FLAG (bFluidPhysics, 0, 0)) {
		if (gameData.segs.segment2s [objP->segnum].special == SEGMENT_IS_WATER)
			time_scale = 75;
		else if (gameData.segs.segment2s [objP->segnum].special == SEGMENT_IS_LAVA)
			time_scale = 66;
		else
			time_scale = 100;
		}
	else
#endif
		time_scale = 100;
	do {
		try_again = 0;
		//Move the object
		VmVecCopyScale(
			&frame_vec, 
			&objP->mtype.phys_info.velocity, 
#if FLUID_PHYSICS
			fixmuldiv (sim_time, time_scale, 100)
#else
			sim_time
#endif
);
#ifdef EXTRA_DEBUG
		//if (objP == debugObjP)
			//printf("  pass %d, frame_vec = %x %x %x\n", count, XYZ(&frame_vec));
#endif
		if ((frame_vec.x==0) && (frame_vec.y==0) && (frame_vec.z==0))	
			break;
retryMove:
		count++;
		//	If retry count is getting large, then we are trying to do something stupid.
		if (count > 3) 	{
			if (objP->type == OBJ_PLAYER) {
				if (count > 8) {
					if (gameStates.gameplay.bSpeedBoost)
						gameStates.gameplay.bSpeedBoost = 0;
					break;
				}
			} else
				break;
		}
		VmVecAdd(&new_pos, &objP->pos, &frame_vec);
		#ifdef EXTRA_DEBUG
		//if (objP == debugObjP)
			//printf("   desired_pos = %x %x %x\n", XYZ(&new_pos));
		#endif
		ignore_obj_list[n_ignore_objs] = -1;
		#ifdef EXTRA_DEBUG
		if (objP == debugObjP) {
			//printf("   FVI parms: p0 = %8x %8x %8x, segnum=%x, size=%x\n", XYZ(&objP->pos), objP->segnum, objP->size);
			//printf("              p1 = %8x %8x %8x\n", XYZ(&new_pos));
		}
		#endif
#ifdef _DEBUG
//iSegError:
#endif
		fq.p0 = &objP->pos;
		fq.startseg = objP->segnum;
		fq.p1 = &new_pos;
		fq.rad = objP->size;
		fq.thisobjnum = objnum;
		fq.ignore_obj_list = ignore_obj_list;
		fq.flags = FQ_CHECK_OBJS;

		if (objP->type == OBJ_WEAPON)
			fq.flags |= FQ_TRANSPOINT;
		if (objP->type == OBJ_PLAYER)
			fq.flags |= FQ_GET_SEGLIST;

//@@			if (GetSegMasks(&objP->pos, objP->segnum, 0).centermask!=0)
//@@				Int3();

save_p0 = *fq.p0;
save_p1 = *fq.p1;

		fate = FindVectorIntersection(&fq, &hit_info);
#ifdef _DEBUG
		if (fate == HIT_BAD_P0) {
			objP->segnum = FindSegByPoint (&objP->pos, objP->segnum);
			if (fq.startseg != objP->segnum) {
				fq.startseg = objP->segnum;
				fate = FindVectorIntersection(&fq, &hit_info);
				}
			}
		else if (fate)// == HIT_WALL)
			fate = FindVectorIntersection(&fq, &hit_info);
#endif
		//	Matt: Mike's hack.
		if (fate == HIT_OBJECT) {
			object	*objP = gameData.objs.objects + hit_info.hit_object;

			if ((objP->type == OBJ_WEAPON) && ((objP->id == PROXIMITY_ID) || (objP->id == SUPERPROX_ID)))
				count--;
		}

#ifndef NDEBUG
		if (fate == HIT_BAD_P0) {
#if TRACE				
			con_printf (CON_DEBUG, "Warning: Bad p0 in physics!  Object = %i, type = %i [%s]\n", 
				OBJ_IDX (objP), objP->type, szObjectTypeNames[objP->type]);
#endif
			Int3();
		}
#endif

		if (objP->type == OBJ_PLAYER) {
			int i;

			if (n_phys_segs && phys_seglist[n_phys_segs-1]==hit_info.seglist[0])
				n_phys_segs--;

			for (i=0;(i<hit_info.n_segs) && (n_phys_segs<MAX_FVI_SEGS-1); )
				phys_seglist[n_phys_segs++] = hit_info.seglist[i++];
		}

		#ifdef EXTRA_DEBUG
		//if (objP == debugObjP)
			//printf("   fate = %d, hit_pnt = %8x %8x %8x\n", fate, XYZ(&hit_info.hit_pnt));
		#endif

		ipos = hit_info.hit_pnt;
		iseg = hit_info.hit_seg;
		WallHitSide = hit_info.hit_side;
		WallHitSeg = hit_info.hit_side_seg;
		if (iseg==-1) {		//some sort of horrible error
#ifndef NDEBUG
#if TRACE				
			con_printf (1, "iseg==-1 in physics!  Object = %i, type = %i (%s)\n", 
				OBJ_IDX (objP), objP->type, szObjectTypeNames[objP->type]);
//			CBRK (1);
//			goto iSegError;
#endif
#endif
			//Int3();
			//COMPUTE_SEGMENT_CENTER(&ipos, &gameData.segs.segments[objP->segnum]);
			//ipos.x += objnum;
			//iseg = objP->segnum;
			//fate = HIT_NONE;
			if (objP->type == OBJ_WEAPON)
				objP->flags |= OF_SHOULD_BE_DEAD;
			break;
		}

		Assert(!((fate==HIT_WALL) && ((WallHitSeg == -1) || (WallHitSeg > gameData.segs.nLastSegment))));

		//if(!GetSegMasks(&hit_info.hit_pnt, hit_info.hit_seg, 0).centermask==0)
		//	Int3();

		save_pos = objP->pos;			//save the object's position
		save_seg = objP->segnum;

		// update object's position and segment number
		objP->pos = ipos;

		#ifdef EXTRA_DEBUG
		//if (objP == debugObjP)
			//printf("   new pos = %x %x %x\n", XYZ(&objP->pos));
		#endif

		if (iseg != objP->segnum)
			RelinkObject(objnum, iseg);

		//if start point not in segment, move object to center of segment
		if (GetSegMasks(&objP->pos, objP->segnum, 0).centermask!=0) {
			int n;

			if ((n=FindObjectSeg(objP))==-1) {
				//Int3();
				if (objP->type==OBJ_PLAYER && (n=FindSegByPoint(&objP->last_pos, objP->segnum))!=-1) {
					objP->pos = objP->last_pos;
					RelinkObject(objnum, n);
				}
				else {
					COMPUTE_SEGMENT_CENTER_I (&objP->pos, objP->segnum);
					objP->pos.x += objnum;
				}
				if (objP->type == OBJ_WEAPON)
					objP->flags |= OF_SHOULD_BE_DEAD;
			}
			return;
		}

		//calulate new sim time
		{
			//vms_vector vMoved;
			vms_vector moved_vec_n;
			fix attempted_dist, actual_dist;
			old_sim_time = sim_time;
			actual_dist = VmVecNormalizedDir(&moved_vec_n, &objP->pos, &save_pos);
			if (fate==HIT_WALL && VmVecDot(&moved_vec_n, &frame_vec) < 0) {		//moved backwards
				//don't change position or sim_time
				#ifdef EXTRA_DEBUG
				//if (objP == debugObjP)
					//printf("   Warning: moved backwards!\n");
				#endif
				objP->pos = save_pos;
				//iseg = objP->segnum;		//don't change segment
				if (save_seg != iseg)
					RelinkObject(objnum, save_seg);
				if (bDoSpeedBoost) {
//					int h = FindSegByPoint(&new_pos, -1);
#if 1
					objP->pos = start_pos;
					SetSpeedBoostVelocity (objnum, -1, -1, -1, -1, -1, &start_pos, &speedBoostDest, 0);
					VmVecCopyScale (&frame_vec, &boostedVel, sim_time);
#else
#	if 0
					vms_vector	v;
					v.x = (objP->mtype.phys_info.velocity.x < 0) ? F1_0 * -60 : F1_0 * 60;
					v.y = (objP->mtype.phys_info.velocity.y < 0) ? F1_0 * -60 : F1_0 * 60;
					v.z = (objP->mtype.phys_info.velocity.z < 0) ? F1_0 * -60 : F1_0 * 60;
					VmVecCopyScale(&frame_vec, &v, sim_time);
#	else
#		if 1

					VmVecCopyScale (&frame_vec, &boostedVel, 
#			if FLUID_PHYSICS
						fixmuldiv (sim_time, time_scale, 100)
#			else
						sim_time
#			endif
						);
#		else
					frame_vec = boostedVel;
#		endif
#	endif
					objP->mtype.phys_info.velocity = boostedVel;
#endif
					goto retryMove;
					}
				moved_time = 0;
			}
			else {
//retryMove2:
				//if (objP == debugObjP)
				//	//printf("   vMoved = %x %x %x\n", XYZ(&vMoved);
				attempted_dist = VmVecMag(&frame_vec);
				sim_time = fixmuldiv(sim_time, attempted_dist-actual_dist, attempted_dist);
				moved_time = old_sim_time - sim_time;
				if (sim_time < 0 || sim_time>old_sim_time) {
#if TRACE				
					con_printf (CON_DEBUG, "Bogus sim_time = %x, old = %x\n", sim_time, old_sim_time);
//					if (objP == debugObjP)
//						con_printf(CON_DEBUG, "   Bogus sim_time = %x, old = %x, attempted_dist = %x, actual_dist = %x\n", sim_time, old_sim_time, attempted_dist, actual_dist);
					//Int3(); Removed by Rob
#endif
					sim_time = old_sim_time;
					//WHY DOES THIS HAPPEN??
					moved_time = 0;
				}
			}

			#ifdef EXTRA_DEBUG
			//if (objP == debugObjP)
				//printf("   new sim_time = %x\n", sim_time);
			#endif

		}

		switch (fate) {

			case HIT_WALL: {
				vms_vector moved_v;
				//@@fix total_d, moved_d;
				fix hit_speed, wall_part;
	
				// Find hit speed	
//				if (gameStates.gameplay.bSpeedBoost && (objP == gameData.objs.console))
//					break;

				VmVecSub(&moved_v, &objP->pos, &save_pos);
				wall_part = VmVecDot(&moved_v, &hit_info.hit_wallnorm);
				if (wall_part != 0 && moved_time>0 && (hit_speed=-fixdiv(wall_part, moved_time))>0)
					CollideObjectWithWall(objP, hit_speed, WallHitSeg, WallHitSide, &hit_info.hit_pnt);
				else
					ScrapeObjectOnWall(objP, WallHitSeg, WallHitSide, &hit_info.hit_pnt);
				Assert(WallHitSeg > -1);
				Assert(WallHitSide > -1);
				if (!(objP->flags&OF_SHOULD_BE_DEAD))	{
					int forcefield_bounce;		//bounce off a forcefield

					Assert(gameStates.app.cheats.bBouncingWeapons || !(objP->mtype.phys_info.flags & PF_STICK && objP->mtype.phys_info.flags & PF_BOUNCE));	//can't be bounce and stick
					forcefield_bounce = (gameData.pig.tex.pTMapInfo[gameData.segs.segments[WallHitSeg].sides[WallHitSide].tmap_num].flags & TMI_FORCE_FIELD);
					if (!forcefield_bounce && (objP->mtype.phys_info.flags & PF_STICK)) {		//stop moving
						AddStuckObject(objP, WallHitSeg, WallHitSide);
						VmVecZero(&objP->mtype.phys_info.velocity);
						obj_stopped = 1;
						try_again = 0;
					}
					else {					// Slide object along wall
						int check_vel=0;
						//We're constrained by wall, so subtract wall part from
						//velocity vector
						wall_part = VmVecDot(&hit_info.hit_wallnorm, &objP->mtype.phys_info.velocity);
						if (forcefield_bounce || (objP->mtype.phys_info.flags & PF_BOUNCE)) {		//bounce off wall
							wall_part *= 2;	//Subtract out wall part twice to achieve bounce
							if (forcefield_bounce) {
								check_vel = 1;				//check for max velocity
								if (objP->type == OBJ_PLAYER)
									wall_part *= 2;		//player bounce twice as much
							}
							if (objP->mtype.phys_info.flags & PF_BOUNCES_TWICE) {
								Assert(objP->mtype.phys_info.flags & PF_BOUNCE);
								if (objP->mtype.phys_info.flags & PF_BOUNCED_ONCE)
									objP->mtype.phys_info.flags &= ~(PF_BOUNCE+PF_BOUNCED_ONCE+PF_BOUNCES_TWICE);
								else
									objP->mtype.phys_info.flags |= PF_BOUNCED_ONCE;
							}
							bounced = 1;		//this object bounced
						}
						VmVecScaleInc(&objP->mtype.phys_info.velocity, &hit_info.hit_wallnorm, -wall_part);
						if (check_vel) {
							fix vel = VmVecMagQuick(&objP->mtype.phys_info.velocity);
							if (vel > MAX_OBJECT_VEL)
								VmVecScale(&objP->mtype.phys_info.velocity, fixdiv(MAX_OBJECT_VEL, vel));
						}
						if (bounced && objP->type == OBJ_WEAPON)
							VmVector2Matrix(&objP->orient, &objP->mtype.phys_info.velocity, &objP->orient.uvec, NULL);
						#ifdef EXTRA_DEBUG
						if (objP == debugObjP) {
							//printf("   sliding - wall_norm %x %x %x %x\n", wall_part, XYZ(&hit_info.hit_wallnorm));
							//printf("   wall_part %x, new velocity = %x %x %x\n", wall_part, XYZ(&objP->mtype.phys_info.velocity));
						}
						#endif

						try_again = 1;
					}
				}
				break;
			}

			case HIT_OBJECT:		{
				vms_vector old_vel;
				// Mark the hit object so that on a retry the fvi code
				// ignores this object.
				//if (gameStates.gameplay.bSpeedBoost && (objP == gameData.objs.console))
				//	break;
				Assert(hit_info.hit_object != -1);
				//	Calculcate the hit point between the two gameData.objs.objects.
				{	vms_vector	*ppos0, *ppos1, pos_hit;
					fix			size0, size1;
					ppos0 = &gameData.objs.objects[hit_info.hit_object].pos;
					ppos1 = &objP->pos;
					size0 = gameData.objs.objects[hit_info.hit_object].size;
					size1 = objP->size;
					Assert(size0+size1 != 0);	// Error, both sizes are 0, so how did they collide, anyway?!?
					//VmVecScale(VmVecSub(&pos_hit, ppos1, ppos0), fixdiv(size0, size0 + size1);
					//VmVecInc(&pos_hit, ppos0);
					VmVecSub(&pos_hit, ppos1, ppos0);
					VmVecScaleAdd(&pos_hit, ppos0, &pos_hit, fixdiv(size0, size0 + size1));
					old_vel = objP->mtype.phys_info.velocity;
					CollideTwoObjects (objP, gameData.objs.objects + hit_info.hit_object, &pos_hit);
					if (gameStates.gameplay.bSpeedBoost && (objP == gameData.objs.console))
						objP->mtype.phys_info.velocity = old_vel;
				}

				// Let object continue its movement
				if (!(objP->flags&OF_SHOULD_BE_DEAD) )	{
					//objP->pos = save_pos;

					if (objP->mtype.phys_info.flags&PF_PERSISTENT || (old_vel.x == objP->mtype.phys_info.velocity.x && old_vel.y == objP->mtype.phys_info.velocity.y && old_vel.z == objP->mtype.phys_info.velocity.z)) {
						//if (gameData.objs.objects[hit_info.hit_object].type == OBJ_POWERUP)
							ignore_obj_list[n_ignore_objs++] = hit_info.hit_object;
						try_again = 1;
					}
				}

				break;
			}	
			case HIT_NONE:		
			#ifdef TACTILE
				if (TactileStick && obj==gameData.objs.console && !(gameData.app.nFrameCount & 15))
				 Tactile_Xvibrate_clear ();
			#endif
				break;

			#ifndef NDEBUG
			case HIT_BAD_P0:
				Int3();		// Unexpected collision type: start point not in specified segment.
#if TRACE				
				con_printf (CON_DEBUG, "Warning: Bad p0 in physics!!!\n");
#endif
				break;
			default:
				// Unknown collision type returned from FindVectorIntersection!!
				Int3();
				break;
			#endif
		}

	} while (try_again);

	//	Pass retry count info to AI.
	if (objP->control_type == CT_AI) {
		if (count > 0) {
			gameData.ai.localInfo[objnum].retry_count = count-1;
#ifndef NDEBUG
			Total_retries += count-1;
			Total_sims++;
#endif
		}
	}

	//I'm not sure why we do this.  I wish there were a comment that
	//explained it.  I think maybe it only needs to be done if the object
	//is sliding, but I don't know
	if (!(gameStates.gameplay.bSpeedBoost || obj_stopped || bounced))	{	//Set velocity from actual movement
		vms_vector vMoved;
		VmVecSub (&vMoved, &objP->pos, &start_pos);
		if (vMoved.x || vMoved.y || vMoved.z)
			VmVecCopyScale (&objP->mtype.phys_info.velocity, &vMoved, 
#if FLUID_PHYSICS
				fixmuldiv (fixdiv(f1_0, gameData.app.xFrameTime), 100, time_scale)
#else
				fixdiv(f1_0, gameData.app.xFrameTime)
#endif
				);
#ifdef BUMP_HACK
		if (objP == gameData.objs.console && 
			 objP->mtype.phys_info.velocity.x==0 && 
			 objP->mtype.phys_info.velocity.y==0 && 
			 objP->mtype.phys_info.velocity.z==0 &&
			(objP->mtype.phys_info.thrust.x!=0 || 
			 objP->mtype.phys_info.thrust.y!=0 ||
			 objP->mtype.phys_info.thrust.z!=0)) {
			vms_vector center, vBump;
			//bump player a little towards center of segment to unstick
			COMPUTE_SEGMENT_CENTER_I(&center, objP->segnum);
			VmVecNormalizedDirQuick(&vBump, &center, &objP->pos);
			//don't bump player toward center of reactor segment
			if (gameData.segs.segment2s[objP->segnum].special == SEGMENT_IS_CONTROLCEN)
				VmVecNegate(&vBump);
			VmVecScaleInc(&objP->pos, &vBump, objP->size/5);
			//if moving away from seg, might move out of seg, so update
			if (gameData.segs.segment2s[objP->segnum].special == SEGMENT_IS_CONTROLCEN)
				UpdateObjectSeg(objP);
		}
#endif
	}

	//Assert(check_point_in_seg(&objP->pos, objP->segnum, 0).centermask==0);
	//if (objP->control_type == CT_FLYING)
	if (objP->mtype.phys_info.flags & PF_LEVELLING)
		do_physics_align_object(objP);

	//hack to keep player from going through closed doors
	if (objP->type==OBJ_PLAYER && objP->segnum!=orig_segnum && (gameStates.app.cheats.bPhysics!=0xBADA55)) {
		short sidenum = FindConnectedSide(gameData.segs.segments + objP->segnum, gameData.segs.segments + orig_segnum);
		if (sidenum != -1) {
			if (! (WALL_IS_DOORWAY(gameData.segs.segments + orig_segnum, sidenum, NULL) & WID_FLY_FLAG)) {
				side *s;
				int vertnum, num_faces, i;
				fix dist;
				int vertex_list[6];

				//bump object back
				s = gameData.segs.segments[orig_segnum].sides + sidenum;
				if (orig_segnum==-1)
					Error("orig_segnum == -1 in physics");
				create_abs_vertex_lists(&num_faces, vertex_list, orig_segnum, sidenum);
				//let's pretend this wall is not triangulated
				vertnum = vertex_list[0];
				for (i=1;i<4;i++)
					if (vertex_list[i] < vertnum)
						vertnum = vertex_list[i];
				#ifdef COMPACT_SEGS
					{
					vms_vector _vn;
					get_side_normal(gameData.segs.segments + orig_segnum, sidenum, 0, &_vn);
					dist = VmDistToPlane(&start_pos, &_vn, &gameData.segs.vertices[vertnum]);
					VmVecScaleAdd(&objP->pos, &start_pos, &_vn, objP->size-dist);
					}
				#else
					dist = VmDistToPlane(&start_pos, s->normals, gameData.segs.vertices + vertnum);
					VmVecScaleAdd(&objP->pos, &start_pos, s->normals, objP->size-dist);
				#endif
				UpdateObjectSeg(objP);

			}
		}
	}

//--WE ALWYS WANT THIS IN, MATT AND MIKE DECISION ON 12/10/94, TWO MONTHS AFTER FINAL 	#ifndef NDEBUG
	//if end point not in segment, move object to last pos, or segment center
	if (GetSegMasks(&objP->pos, objP->segnum, 0).centermask!=0) {
		if (FindObjectSeg(objP)==-1) {
			int n;

			//Int3();
			if (objP->type==OBJ_PLAYER && (n=FindSegByPoint(&objP->last_pos, objP->segnum))!=-1) {
				objP->pos = objP->last_pos;
				RelinkObject(objnum, n);
			}
			else {
				COMPUTE_SEGMENT_CENTER_I (&objP->pos, objP->segnum);
				objP->pos.x += objnum;
			}
			if (objP->type == OBJ_WEAPON)
				objP->flags |= OF_SHOULD_BE_DEAD;
		}
	}
}

//--unused-- //tell us what the given object will do (as far as hiting walls) in
//--unused-- //the given time (in seconds) t.  Igores acceleration (sorry)
//--unused-- //if check_objects is set, check with gameData.objs.objects, else just with walls
//--unused-- //returns fate, fills in hit time.  If fate==HIT_NONE, hit_time undefined
//--unused-- int physics_lookahead(object *objP, fix t, int fvi_flags, fix *hit_time, fvi_info *hit_info)
//--unused-- {
//--unused-- 	vms_vector new_pos;
//--unused-- 	int objnum, fate;
//--unused-- 	fvi_query fq;
//--unused--
//--unused-- 	Assert(objP->movement_type == MT_PHYSICS);
//--unused--
//--unused-- 	objnum = OBJ_IDX (objP);
//--unused--
//--unused-- 	VmVecScaleAdd(&new_pos, &objP->pos, &objP->mtype.phys_info.velocity, t);
//--unused--
//--unused-- 	fq.p0					 = &objP->pos;
//--unused-- 	fq.startseg			 = objP->segnum;
//--unused-- 	fq.p1					 = &new_pos;
//--unused-- 	fq.rad				 = objP->size;
//--unused-- 	fq.thisobjnum		 = objnum;
//--unused-- 	fq.ignore_obj_list = NULL;
//--unused-- 	fq.flags				 = fvi_flags;
//--unused--
//--unused-- 	fate = FindVectorIntersection(&fq, hit_info);
//--unused--
//--unused-- 	if (fate != HIT_NONE) {
//--unused-- 		fix dist, speed;
//--unused--
//--unused-- 		dist = VmVecDist(&objP->pos, &hit_info->hit_pnt);
//--unused--
//--unused-- 		speed = VmVecMag(&objP->mtype.phys_info.velocity);
//--unused--
//--unused-- 		*hit_time = fixdiv(dist, speed);
//--unused--
//--unused-- 	}
//--unused--
//--unused-- 	return fate;
//--unused--
//--unused-- }

//Applies an instantaneous force on an object, resulting in an instantaneous
//change in velocity.

void PhysApplyForce(object *objP, vms_vector *vForce)
{

	//	Put in by MK on 2/13/96 for force getting applied to Omega blobs, which have 0 mass, 
	//	in collision with crazy reactor robot thing on d2levf-s.
if (objP->mtype.phys_info.mass == 0)
	return;
if (objP->movement_type != MT_PHYSICS)
	return;
#ifdef TACTILE
  if (TactileStick && obj==&gameData.objs.objects[gameData.multi.players[gameData.multi.nLocalPlayer].objnum])
	Tactile_apply_force (vForce, &objP->orient);
#endif
//Add in acceleration due to force
if (!gameStates.gameplay.bSpeedBoost || (objP != gameData.objs.console))
	VmVecScaleInc (&objP->mtype.phys_info.velocity, 
						vForce, 
						fixdiv (f1_0, objP->mtype.phys_info.mass));


}

//	----------------------------------------------------------------
//	Do *dest = *delta unless:
//				*delta is pretty small
//		and	they are of different signs.
void PhysicsSetRotVelAndSaturate(fix *dest, fix delta)
{
	if ((delta ^ *dest) < 0) {
		if (abs(delta) < F1_0/8) {
			*dest = delta/4;
		} else
			*dest = delta;
	} else {
		*dest = delta;
	}
}

//	------------------------------------------------------------------------------------------------------
//	Note: This is the old ai_turn_towards_vector code.
//	PhysApplyRot used to call ai_turn_towards_vector until I fixed it, which broke PhysApplyRot.
void physics_turn_towards_vector(vms_vector *goal_vector, object *objP, fix rate)
{
	vms_angvec	dest_angles, cur_angles;
	fix			delta_p, delta_h;
	vms_vector	*rotvel_ptr = &objP->mtype.phys_info.rotvel;

	// Make this object turn towards the goal_vector.  Changes orientation, doesn't change direction of movement.
	// If no one moves, will be facing goal_vector in 1 second.

	//	Detect null vector.
	if ((goal_vector->x == 0) && (goal_vector->y == 0) && (goal_vector->z == 0))
		return;

	//	Make morph gameData.objs.objects turn more slowly.
	if (objP->control_type == CT_MORPH)
		rate *= 2;

	VmExtractAnglesVector(&dest_angles, goal_vector);
	VmExtractAnglesVector(&cur_angles, &objP->orient.fvec);

	delta_p = (dest_angles.p - cur_angles.p);
	delta_h = (dest_angles.h - cur_angles.h);

	if (delta_p > F1_0/2) delta_p = dest_angles.p - cur_angles.p - F1_0;
	if (delta_p < -F1_0/2) delta_p = dest_angles.p - cur_angles.p + F1_0;
	if (delta_h > F1_0/2) delta_h = dest_angles.h - cur_angles.h - F1_0;
	if (delta_h < -F1_0/2) delta_h = dest_angles.h - cur_angles.h + F1_0;

	delta_p = fixdiv(delta_p, rate);
	delta_h = fixdiv(delta_h, rate);

	if (abs(delta_p) < F1_0/16) delta_p *= 4;
	if (abs(delta_h) < F1_0/16) delta_h *= 4;

	PhysicsSetRotVelAndSaturate(&rotvel_ptr->x, delta_p);
	PhysicsSetRotVelAndSaturate(&rotvel_ptr->y, delta_h);
	rotvel_ptr->z = 0;
}

//	-----------------------------------------------------------------------------
//	Applies an instantaneous whack on an object, resulting in an instantaneous
//	change in orientation.
void PhysApplyRot(object *objP, vms_vector *vForce)
{
	fix	rate, vecmag;

	if (objP->movement_type != MT_PHYSICS)
		return;

	vecmag = VmVecMag(vForce)/8;
	if (vecmag < F1_0/256)
		rate = 4*F1_0;
	else if (vecmag < objP->mtype.phys_info.mass >> 14)
		rate = 4*F1_0;
	else {
		rate = fixdiv(objP->mtype.phys_info.mass, vecmag);
		if (objP->type == OBJ_ROBOT) {
			if (rate < F1_0/4)
				rate = F1_0/4;
			//	Changed by mk, 10/24/95, claw guys should not slow down when attacking!
			if (!gameData.bots.pInfo[objP->id].thief && !gameData.bots.pInfo[objP->id].attack_type) {
				if (objP->ctype.ai_info.SKIP_AI_COUNT * gameData.app.xFrameTime < 3*F1_0/4) {
					fix	tval = fixdiv(F1_0, 8*gameData.app.xFrameTime);
					int	addval;

					addval = f2i(tval);

					if ((d_rand() * 2) < (tval & 0xffff))
						addval++;
					objP->ctype.ai_info.SKIP_AI_COUNT += addval;
				}
			}
		} else {
			if (rate < F1_0/2)
				rate = F1_0/2;
		}
	}

	//	Turn amount inversely proportional to mass.  Third parameter is seconds to do 360 turn.
	physics_turn_towards_vector(vForce, objP, rate);


}


//this routine will set the thrust for an object to a value that will
//(hopefully) maintain the object's current velocity
void set_thrust_from_velocity(object *objP)
{
	fix k;

	Assert(objP->movement_type == MT_PHYSICS);

	k = fixmuldiv(objP->mtype.phys_info.mass, objP->mtype.phys_info.drag, (f1_0-objP->mtype.phys_info.drag));

	VmVecCopyScale(&objP->mtype.phys_info.thrust, &objP->mtype.phys_info.velocity, k);

}


