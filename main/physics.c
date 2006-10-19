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
 * New function (untested), set_thrust_from_velocity ()
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
 * #ifdef _DEBUG some code.
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
 * fix stupid apply_force_rot () bug.
 *
 * Revision 1.189  1994/12/05  09:42:17  mike
 * fix 0 mag problem when object applies force to itself.
 *
 * Revision 1.188  1994/12/04  22:48:40  matt
 * Physics & FVI now only build segList for player gameData.objs.objects, and they
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
 * Fixed xDrag problems with framerates over 60Hz
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
 * Add useful information to con_printf (1, ... error messages.
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
 * Made PhysApplyForce () do nothing to an object if it's not a phys object
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
 * and adapt to changed interface to CreatePathToPlayer.
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
 * Call CreatePathToPlayer for a stuck object.
 *
 * Revision 1.153  1994/08/04  00:21:02  matt
 * Cleaned up fvi & physics error handling; put in code to make sure gameData.objs.objects
 * are in correct segment; simplified segment finding for gameData.objs.objects and points
 *
 * Revision 1.152  1994/08/01  16:25:34  matt
 * Check for xMovedTime == 0 when computing hit speed
 *
 * Revision 1.151  1994/08/01  13:30:32  matt
 * Made fvi () check holes in transparent walls, and changed fvi () calling
 * parms to take all input data in query structure.
 *
 * Revision 1.150  1994/07/29  23:41:46  matt
 * Fixed turn banking, which changed when I added rotational velocity
 *
 * Revision 1.149  1994/07/27  20:53:23  matt
 * Added rotational xDrag & thrust, so turning now has momemtum like moving
 *
 */


#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid [] = "$Id: physics.c, v 1.4 2003/10/10 09:36:35 btb Exp $";
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
#include "kconfig.h"
#include "input.h"

#ifdef TACTILE
#include "tactile.h"
#endif

//Global variables for physics system

#define FLUID_PHYSICS	0

#define ROLL_RATE 		0x2000
#define DAMP_ANG 			0x400                  //min angle to bank

#define TURNROLL_SCALE	 (0x4ec4/2)

#define MAX_OBJECT_VEL i2f (100)

#define BUMP_HACK	1		//if defined, bump player when he gets stuck

int floor_levelling=0;

//	-----------------------------------------------------------------------------------------------------------
//make sure matrix is orthogonal
void CheckAndFixMatrix (vms_matrix *m)
{
	vms_matrix tempm;

	VmVector2Matrix (&tempm, &m->fvec, &m->uvec, NULL);
	*m = tempm;
}

//	-----------------------------------------------------------------------------------------------------------

void DoPhysicsAlignObject (object * objP)
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
			GetSideNormal (gameData.segs.segments + objP->segnum, i, 0, &_tv1);
			d = VmVecDot (&_tv1, &objP->orient.uvec);
		#else					
			d = VmVecDot (gameData.segs.segments [objP->segnum].sides [i].normals, &objP->orient.uvec);
		#endif

		if (d > largest_d) {largest_d = d; best_side=i;}
	}

	if (floor_levelling) {

		// old way: used floor's normal as upvec
		#ifdef COMPACT_SEGS
			GetSideNormal (gameData.segs.segments + objP->segnum, 3, 0, &desired_upvec);			
		#else
			desired_upvec = gameData.segs.segments [objP->segnum].sides [3].normals [0];
		#endif

	}
	else  // new player leveling code: use normal of side closest to our up vec
		if (GetNumFaces (&gameData.segs.segments [objP->segnum].sides [best_side])==2) {
			#ifdef COMPACT_SEGS
				vms_vector normals [2];
				GetSideNormals (&gameData.segs.segments [objP->segnum], best_side, &normals [0], &normals [1]);			

				desired_upvec.x = (normals [0].x + normals [1].x) / 2;
				desired_upvec.y = (normals [0].y + normals [1].y) / 2;
				desired_upvec.z = (normals [0].z + normals [1].z) / 2;

				VmVecNormalize (&desired_upvec);
			#else
				side *s = &gameData.segs.segments [objP->segnum].sides [best_side];
				desired_upvec.x = (s->normals [0].x + s->normals [1].x) / 2;
				desired_upvec.y = (s->normals [0].y + s->normals [1].y) / 2;
				desired_upvec.z = (s->normals [0].z + s->normals [1].z) / 2;
		
				VmVecNormalize (&desired_upvec);
			#endif
		}
		else
			#ifdef COMPACT_SEGS
				GetSideNormal (&gameData.segs.segments [objP->segnum], best_side, 0, &desired_upvec);			
			#else
				desired_upvec = gameData.segs.segments [objP->segnum].sides [best_side].normals [0];
			#endif

	if (labs (VmVecDot (&desired_upvec, &objP->orient.fvec)) < f1_0/2) {
		fixang save_delta_ang;
		vms_angvec tangles;
		
		VmVector2Matrix (&temp_matrix, &objP->orient.fvec, &desired_upvec, NULL);

		save_delta_ang = delta_ang = VmVecDeltaAng (&objP->orient.uvec, &temp_matrix.uvec, &objP->orient.fvec);

		delta_ang += objP->mtype.phys_info.turnroll;

		if (abs (delta_ang) > DAMP_ANG) {
			vms_matrix rotmat, new_pm;

			roll_ang = FixMul (gameData.time.xFrame, ROLL_RATE);

			if (abs (delta_ang) < roll_ang) roll_ang = delta_ang;
			else if (delta_ang<0) roll_ang = -roll_ang;

			tangles.p = tangles.h = 0;  tangles.b = roll_ang;
			VmAngles2Matrix (&rotmat, &tangles);

			VmMatMul (&new_pm, &objP->orient, &rotmat);
			objP->orient = new_pm;
		}
		else floor_levelling=0;
	}

}

//	-----------------------------------------------------------------------------------------------------------

void SetObjectTurnRoll (object *objP)
{
//if (!gameStates.app.bD1Mission) 
	{
	fixang desired_bank = -FixMul (objP->mtype.phys_info.rotvel.y, TURNROLL_SCALE);
	if (objP->mtype.phys_info.turnroll != desired_bank) {
		fixang delta_ang, max_roll;
		max_roll = FixMul (ROLL_RATE, gameData.time.xFrame);
		delta_ang = desired_bank - objP->mtype.phys_info.turnroll;
		if (labs (delta_ang) < max_roll)
			max_roll = delta_ang;
		else
			if (delta_ang < 0)
				max_roll = -max_roll;
		objP->mtype.phys_info.turnroll += max_roll;
		}
	}
}

//list of segments went through
int physSegList [MAX_FVI_SEGS], nPhysSegs;


#define MAX_IGNORE_OBJS 100

#ifdef _DEBUG
#define EXTRA_DEBUG 1		//no extra debug when NDEBUG is on
#endif

#ifdef EXTRA_DEBUG
object *debugObjP=NULL;
#endif

#define XYZ(v) (v)->x, (v)->y, (v)->z


#ifdef _DEBUG
int	Total_retries=0, Total_sims=0;
int	bDontMoveAIObjects=0;
#endif

#define FT (f1_0/64)

extern int bSimpleFVI;
//	-----------------------------------------------------------------------------------------------------------
// add rotational velocity & acceleration
void DoPhysicsSimRot (object *objP)
{
	vms_angvec	tangles;
	vms_matrix	rotmat, new_orient;
	//fix			rotdrag_scale;
	physics_info *pi;

#if 0
	Assert (gameData.time.xFrame > 0); 		//Get MATT if hit this!
#else
	if (gameData.time.xFrame <= 0)
		return;
#endif

	pi = &objP->mtype.phys_info;

	if (!(pi->rotvel.x || pi->rotvel.y || pi->rotvel.z || pi->rotthrust.x || pi->rotthrust.y || pi->rotthrust.z))
		return;

	if (objP->mtype.phys_info.drag) {
		int count;
		vms_vector accel;
		fix xDrag, r, k;

		count = gameData.time.xFrame / FT;
		r = gameData.time.xFrame % FT;
		k = FixDiv (r, FT);

		xDrag = (objP->mtype.phys_info.drag*5)/2;

		if (objP->mtype.phys_info.flags & PF_USES_THRUST) {

			VmVecCopyScale (&accel, 
				&objP->mtype.phys_info.rotthrust, 
				FixDiv (f1_0, objP->mtype.phys_info.mass));

			while (count--) {

				VmVecInc (&objP->mtype.phys_info.rotvel, &accel);

				VmVecScale (&objP->mtype.phys_info.rotvel, f1_0-xDrag);
			}

			//do linear scale on remaining bit of time

			VmVecScaleInc (&objP->mtype.phys_info.rotvel, &accel, k);
			VmVecScale (&objP->mtype.phys_info.rotvel, f1_0-FixMul (k, xDrag));
		}
		else if (!(objP->mtype.phys_info.flags & PF_FREE_SPINNING)) {
			fix xTotalDrag=f1_0;

			while (count--)
				xTotalDrag = FixMul (xTotalDrag, f1_0-xDrag);

			//do linear scale on remaining bit of time

			xTotalDrag = FixMul (xTotalDrag, f1_0-FixMul (k, xDrag));

			VmVecScale (&objP->mtype.phys_info.rotvel, xTotalDrag);
		}

	}

	//now rotate object

	//unrotate object for bank caused by turn
	if (objP->mtype.phys_info.turnroll) {
		vms_matrix new_pm;

		tangles.p = tangles.h = 0;
		tangles.b = -objP->mtype.phys_info.turnroll;
		VmAngles2Matrix (&rotmat, &tangles);
		VmMatMul (&new_pm, &objP->orient, &rotmat);
		objP->orient = new_pm;
	}

	tangles.p = FixMul (objP->mtype.phys_info.rotvel.x, gameData.time.xFrame);
	tangles.h = FixMul (objP->mtype.phys_info.rotvel.y, gameData.time.xFrame);
	tangles.b = FixMul (objP->mtype.phys_info.rotvel.z, gameData.time.xFrame);

	VmAngles2Matrix (&rotmat, &tangles);
	VmMatMul (&new_orient, &objP->orient, &rotmat);
	objP->orient = new_orient;

	if (objP->mtype.phys_info.flags & PF_TURNROLL)
		SetObjectTurnRoll (objP);

	//re-rotate object for bank caused by turn
	if (objP->mtype.phys_info.turnroll) {
		vms_matrix new_pm;

		tangles.p = tangles.h = 0;
		tangles.b = objP->mtype.phys_info.turnroll;
		VmAngles2Matrix (&rotmat, &tangles);
		VmMatMul (&new_pm, &objP->orient, &rotmat);
		objP->orient = new_pm;
	}

	CheckAndFixMatrix (&objP->orient);
}

//	-----------------------------------------------------------------------------------------------------------

void UnstickObject (object *objP)
{
	fvi_info			hi;
	fvi_query		fq;
	int				fate;
	short				nSegment;
	fix				xSideDist, xSideDists [6];

fq.p0 = fq.p1 = &objP->pos;
fq.startSeg = objP->segnum;
fq.rad = objP->size;
fq.thisObjNum = OBJ_IDX (objP);
fq.ignoreObjList = NULL;
fq.flags = 0;
fate = FindVectorIntersection (&fq, &hi);
if (fate != HIT_WALL)
	return;
GetSideDistsAll (&objP->pos, hi.hit.nSideSegment, xSideDists);
if ((xSideDist = xSideDists [hi.hit.nSide]) && (xSideDist < objP->size - objP->size / 100)) {
#if 1
	float r = 0.25f;
#else
	float r;
	xSideDist = objP->size - xSideDist;
	r = ((float) xSideDist / (float) objP->size) * f2fl (objP->size);
#endif
	objP->pos.x += (fix) ((float) hi.hit.vNormal.x * r);
	objP->pos.y += (fix) ((float) hi.hit.vNormal.y * r);
	objP->pos.z += (fix) ((float) hi.hit.vNormal.z * r);
	nSegment = FindSegByPoint (&objP->pos, objP->segnum);
	if (nSegment != objP->segnum)
		RelinkObject (OBJ_IDX (objP), nSegment);
#if 0//def _DEBUG
	if (objP->type == OBJ_PLAYER)
		HUDMessage (0, "PENETRATING WALL (%d, %1.4f)", objP->size - xSideDists [nWallHitSide], r);
#endif
	}
}

//	-----------------------------------------------------------------------------------------------------------

extern object *monsterballP;
extern vms_vector boostedVel, minBoostedVel, maxBoostedVel;

//Simulate a physics object for this frame
#if 1
void DoPhysicsSim (object *objP)
{
	short				ignoreObjList [MAX_IGNORE_OBJS], nIgnoreObjs;
	int				iseg;
	int				bRetry;
	int				fate;
	vms_vector		vFrame;			//movement in this frame
	vms_vector		vNewPos, ipos;		//position after this frame
	int				count=0;
	short				objnum;
	short				nWallHitSeg, nWallHitSide;
	fvi_info			hi;
	fvi_query		fq;
	vms_vector		vSavePos;
	int				nSaveSeg;
	fix				xDrag;
	fix				xSimTime, xOldSimTime, xTimeScale;
	vms_vector		vStartPos;
	int				bObjStopped=0;
	fix				xMovedTime;			//how long objected moved before hit something
	vms_vector		vSaveP0, vSaveP1;
	physics_info	*pi;
	short				orig_segnum = objP->segnum;
	int				bBounced=0;
	int				bDoSpeedBoost = gameStates.gameplay.bSpeedBoost && (objP == gameData.objs.console);

Assert (objP->type != OBJ_NONE);
Assert (objP->movement_type == MT_PHYSICS);
if (objP->type == OBJ_PLAYER && Controls.heading_time)
	objP = objP;
#ifdef _DEBUG
if (bDontMoveAIObjects)
	if (objP->control_type == CT_AI)
		return;
#endif
pi = &objP->mtype.phys_info;
DoPhysicsSimRot (objP);
#if 1
if (!(pi->velocity.x || pi->velocity.y || pi->velocity.z)) {
	UnstickObject (objP);
	if (objP == gameData.objs.console)
		gameStates.gameplay.bSpeedBoost = 0;
	if (!(pi->thrust.x || pi->thrust.y || pi->thrust.z))
		return;
	}
#endif
objnum = objP - gameData.objs.objects;
nPhysSegs = 0;
bSimpleFVI = (objP->type != OBJ_PLAYER);
xSimTime = gameData.time.xFrame;
vStartPos = objP->pos;
nIgnoreObjs = 0;
Assert (objP->mtype.phys_info.brakes==0);		//brakes not used anymore?
//if uses thrust, cannot have zero xDrag
Assert (!(objP->mtype.phys_info.flags&PF_USES_THRUST) || objP->mtype.phys_info.drag!=0);
//do thrust & xDrag
if (xDrag = objP->mtype.phys_info.drag) {
	int count;
	vms_vector accel, *vel = &objP->mtype.phys_info.velocity;
	fix r, k, d, a;

	d = f1_0 - xDrag;
	count = xSimTime / FT;
	r = xSimTime % FT;
	k = FixDiv (r, FT);
	if (objP->mtype.phys_info.flags & PF_USES_THRUST) {
		VmVecCopyScale (&accel, &objP->mtype.phys_info.thrust, FixDiv (f1_0, objP->mtype.phys_info.mass));
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
			VmVecScale (vel, f1_0 - FixMul (k, xDrag));
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
		fix xTotalDrag = f1_0;
		while (count--)
			xTotalDrag = FixMul (xTotalDrag, d);
		//do linear scale on remaining bit of time
		xTotalDrag = FixMul (xTotalDrag, f1_0-FixMul (k, xDrag));
		VmVecScale (&objP->mtype.phys_info.velocity, xTotalDrag);
		}
	}
if (extraGameInfo [IsMultiGame].bFluidPhysics) {
	if (gameData.segs.segment2s [objP->segnum].special == SEGMENT_IS_WATER)
		xTimeScale = 75;
	else if (gameData.segs.segment2s [objP->segnum].special == SEGMENT_IS_LAVA)
		xTimeScale = 66;
	else
		xTimeScale = 100;
	}
else
	xTimeScale = 100;
do {
	bRetry = 0;
	//Move the object
	VmVecCopyScale (
		&vFrame, 
		&objP->mtype.phys_info.velocity, 
		FixMulDiv (xSimTime, xTimeScale, 100));
	if ((vFrame.x == 0) && (vFrame.y == 0) && (vFrame.z == 0))
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
	VmVecAdd (&vNewPos, &objP->pos, &vFrame);
	ignoreObjList [nIgnoreObjs] = -1;
	fq.p0 = &objP->pos;
	fq.startSeg = objP->segnum;
	fq.p1 = &vNewPos;
	fq.rad = objP->size;
	fq.thisObjNum = objnum;
	fq.ignoreObjList = ignoreObjList;
	fq.flags = FQ_CHECK_OBJS;

	if (objP->type == OBJ_WEAPON)
		fq.flags |= FQ_TRANSPOINT;
	if (objP->type == OBJ_PLAYER)
		fq.flags |= FQ_GET_SEGLIST;

	vSaveP0 = *fq.p0;
	vSaveP1 = *fq.p1;
	fate = FindVectorIntersection (&fq, &hi);
#ifdef _DEBUG
	if (fate == HIT_WALL)
		fate = FindVectorIntersection (&fq, &hi);
#endif
	//	Matt: Mike's hack.
	if (fate == HIT_OBJECT) {
		object	*objp = gameData.objs.objects + hi.hit.nObject;

		if ((objp->type == OBJ_WEAPON) && ((objp->id == PROXIMITY_ID) || (objp->id == SUPERPROX_ID)))
			count--;
	}

#ifdef _DEBUG
	if (fate == HIT_BAD_P0) {
#if 0 //TRACE				
		con_printf (CON_DEBUG, "Warning: Bad p0 in physics! Object = %i, type = %i [%s]\n", 
			objP - gameData.objs.objects, objP->type, Object_type_names [objP->type]);
#endif
		Int3 ();
	}
#endif

	if (objP->type == OBJ_PLAYER) {
		int i;

		if (nPhysSegs && physSegList [nPhysSegs-1]==hi.segList [0])
			nPhysSegs--;

		for (i=0; (i < hi.nSegments) && (nPhysSegs < MAX_FVI_SEGS - 1); )
			physSegList [nPhysSegs++] = hi.segList [i++];
	}

	ipos = hi.hit.vPoint;
	iseg = hi.hit.nSegment;
	nWallHitSide = hi.hit.nSide;
	nWallHitSeg = hi.hit.nSideSegment;
	if (iseg==-1) {		//some sort of horrible error
		if (objP->type == OBJ_WEAPON)
			objP->flags |= OF_SHOULD_BE_DEAD;
		break;
		}
	Assert (!((fate==HIT_WALL) && ((nWallHitSeg == -1) || (nWallHitSeg > gameData.segs.nLastSegment))));
	vSavePos = objP->pos;			//save the object's position
	nSaveSeg = objP->segnum;
	// update object's position and segment number
	objP->pos = ipos;
	if (iseg != objP->segnum)
		RelinkObject (objnum, iseg);
	//if start point not in segment, move object to center of segment
	if (objP->type == OBJ_PLAYER)
		objP = objP;
	if (GetSegMasks (&objP->pos, objP->segnum, 0).centerMask) {	//object stuck
		vms_vector	vCenter;
		int n = FindObjectSeg (objP);

		if (n == -1) {
			n = FindSegByPoint (&objP->last_pos, objP->segnum);
			if (n == -1) {
				objP->flags |= OF_SHOULD_BE_DEAD;
				return;
				}
			}
		objP->pos = objP->last_pos;
		RelinkObject (objnum, n);
		COMPUTE_SEGMENT_CENTER_I (&vCenter, objP->segnum);
		VmVecDec (&vCenter, &objP->pos);
		if (VmVecMag (&vCenter) > F1_0) {
			VmVecNormalize (&vCenter);
			VmVecScaleFrac (&vCenter, 1, 10);
			}
		VmVecDec (&objP->pos, &vCenter);
		//return;
		}

	//calulate new sim time
	{
		//vms_vector vMoved;
		vms_vector vMoveNormal;
		fix attempted_dist, actual_dist;
		xOldSimTime = xSimTime;
		actual_dist = VmVecNormalizedDir (&vMoveNormal, &objP->pos, &vSavePos);
		if ((fate == HIT_WALL) && (VmVecDot (&vMoveNormal, &vFrame) < 0)) {		//moved backwards
			//don't change position or xSimTime
			objP->pos = vSavePos;
			//iseg = objP->segnum;		//don't change segment
			if (nSaveSeg != iseg)
				RelinkObject (objnum, nSaveSeg);
			if (bDoSpeedBoost) {
//					int h = FindSegByPoint (&vNewPos, -1);
				objP->pos = vStartPos;
				SetSpeedBoostVelocity (objnum, -1, -1, -1, -1, -1, &vStartPos, &speedBoostDest, 0);
				VmVecCopyScale (&vFrame, &boostedVel, xSimTime);
				goto retryMove;
				}
			xMovedTime = 0;
			}
		else {
//retryMove2:
			attempted_dist = VmVecMag (&vFrame);
			xSimTime = FixMulDiv (xSimTime, attempted_dist-actual_dist, attempted_dist);
			xMovedTime = xOldSimTime - xSimTime;
			if ((xSimTime < 0) || (xSimTime > xOldSimTime)) {
				xSimTime = xOldSimTime;
				xMovedTime = 0;
				}
			}
		}

	switch (fate) {
		case HIT_WALL: {
			vms_vector vMoved;
			fix xHitSpeed, xWallPart, xSideDist, xSideDists [6];
			short nSegment;
			// Find hit speed	

#if 0//def _DEBUG
			if (objP->type == OBJ_PLAYER)
				HUDMessage (0, "WALL CONTACT");
			fate = FindVectorIntersection (&fq, &hi);
#endif
			VmVecSub (&vMoved, &objP->pos, &vSavePos);
			xWallPart = VmVecDot (&vMoved, &hi.hit.vNormal);
			if (xWallPart && (xMovedTime > 0) && (xHitSpeed = -FixDiv (xWallPart, xMovedTime)) > 0) {
				CollideObjectWithWall (objP, xHitSpeed, nWallHitSeg, nWallHitSide, &hi.hit.vPoint);
#if 0//def _DEBUG
				if (objP->type == OBJ_PLAYER)
					HUDMessage (0, "BUMP!");
#endif
				}
			else {
#if 0//def _DEBUG
				if (objP->type == OBJ_PLAYER)
					HUDMessage (0, "SCREEEEEEEEEECH");
#endif
				ScrapeObjectOnWall (objP, nWallHitSeg, nWallHitSide, &hi.hit.vPoint);
				}
			Assert (nWallHitSeg > -1);
			Assert (nWallHitSide > -1);
			GetSideDistsAll (&objP->pos, nWallHitSeg, xSideDists);
			if ((xSideDist = xSideDists [nWallHitSide]) && (xSideDist < objP->size - objP->size / 100)) {
#if 1
				float r = 0.1f;
#else
				float r;
				xSideDist = objP->size - xSideDist;
				r = ((float) xSideDist / (float) objP->size) * f2fl (objP->size);
#endif
				objP->pos.x += (fix) ((float) hi.hit.vNormal.x * r);
				objP->pos.y += (fix) ((float) hi.hit.vNormal.y * r);
				objP->pos.z += (fix) ((float) hi.hit.vNormal.z * r);
				nSegment = FindSegByPoint (&objP->pos, objP->segnum);
				if (nSegment != objP->segnum)
					RelinkObject (OBJ_IDX (objP), nSegment);
#if 0//def _DEBUG
				if (objP->type == OBJ_PLAYER)
					HUDMessage (0, "PENETRATING WALL (%d, %1.4f)", objP->size - xSideDists [nWallHitSide], r);
#endif
				bRetry = 1;
				}
			if (!(objP->flags & OF_SHOULD_BE_DEAD)) {
				int forcefield_bounce;		//bounce off a forcefield

				Assert (gameStates.app.cheats.bBouncingWeapons || !(objP->mtype.phys_info.flags & PF_STICK && objP->mtype.phys_info.flags & PF_BOUNCE));	//can't be bounce and stick
				forcefield_bounce = (gameData.pig.tex.pTMapInfo [gameData.segs.segments [nWallHitSeg].sides [nWallHitSide].tmap_num].flags & TMI_FORCE_FIELD);
				if (!forcefield_bounce && (objP->mtype.phys_info.flags & PF_STICK)) {		//stop moving
					AddStuckObject (objP, nWallHitSeg, nWallHitSide);
					VmVecZero (&objP->mtype.phys_info.velocity);
					bObjStopped = 1;
					bRetry = 0;
				}
				else {				// Slide object along wall
					int bCheckVel = 0;
					//We're constrained by wall, so subtract wall part from
					//velocity vector
					xWallPart = VmVecDot (&hi.hit.vNormal, &objP->mtype.phys_info.velocity);
					if (forcefield_bounce || (objP->mtype.phys_info.flags & PF_BOUNCE)) {		//bounce off wall
						xWallPart *= 2;	//Subtract out wall part twice to achieve bounce
						if (forcefield_bounce) {
							bCheckVel = 1;				//check for max velocity
							if (objP->type == OBJ_PLAYER)
								xWallPart *= 2;		//player bounce twice as much
							}
						if (objP->mtype.phys_info.flags & PF_BOUNCES_TWICE) {
							Assert (objP->mtype.phys_info.flags & PF_BOUNCE);
							if (objP->mtype.phys_info.flags & PF_BOUNCED_ONCE)
								objP->mtype.phys_info.flags &= ~ (PF_BOUNCE+PF_BOUNCED_ONCE+PF_BOUNCES_TWICE);
							else
								objP->mtype.phys_info.flags |= PF_BOUNCED_ONCE;
							}
						bBounced = 1;		//this object bBounced
						}
					VmVecScaleInc (&objP->mtype.phys_info.velocity, &hi.hit.vNormal, -xWallPart);
					if (bCheckVel) {
						fix vel = VmVecMag (&objP->mtype.phys_info.velocity);
						if (vel > MAX_OBJECT_VEL)
							VmVecScale (&objP->mtype.phys_info.velocity, FixDiv (MAX_OBJECT_VEL, vel));
						}
					if (bBounced && (objP->type == OBJ_WEAPON))
						VmVector2Matrix (&objP->orient, &objP->mtype.phys_info.velocity, &objP->orient.uvec, NULL);
					bRetry = 1;
					}
				}
			break;
			}

		case HIT_OBJECT: {
			vms_vector vOldVel;
			vms_vector	*ppos0, *ppos1, vHitPos;
			fix			size0, size1;
			// Mark the hit object so that on a retry the fvi code
			// ignores this object.
			//if (bSpeedBoost && (objP == gameData.objs.console))
			//	break;
			Assert (hi.hit.nObject != -1);
			ppos0 = &gameData.objs.objects [hi.hit.nObject].pos;
			ppos1 = &objP->pos;
			size0 = gameData.objs.objects [hi.hit.nObject].size;
			size1 = objP->size;
			//	Calculcate the hit point between the two objects.
			Assert (size0+size1 != 0);	// Error, both sizes are 0, so how did they collide, anyway?!?
			//VmVecScale (VmVecSub (&pos_hit, ppos1, ppos0), FixDiv (size0, size0 + size1);
			//VmVecInc (&pos_hit, ppos0);
			VmVecSub (&vHitPos, ppos1, ppos0);
			VmVecScaleAdd (&vHitPos, ppos0, &vHitPos, FixDiv (size0, size0 + size1));
			vOldVel = objP->mtype.phys_info.velocity;
			CollideTwoObjects (objP, gameData.objs.objects + hi.hit.nObject, &vHitPos);
			if (gameStates.gameplay.bSpeedBoost && (objP == gameData.objs.console))
				objP->mtype.phys_info.velocity = vOldVel;

			// Let object continue its movement
			if (!(objP->flags&OF_SHOULD_BE_DEAD) )	{
				if (objP->mtype.phys_info.flags&PF_PERSISTENT || (vOldVel.x == objP->mtype.phys_info.velocity.x && vOldVel.y == objP->mtype.phys_info.velocity.y && vOldVel.z == objP->mtype.phys_info.velocity.z)) {
					//if (gameData.objs.objects [hi.hit.nObject].type == OBJ_POWERUP)
						ignoreObjList [nIgnoreObjs++] = hi.hit.nObject;
					bRetry = 1;
					}
				}
			break;
			}	
		case HIT_NONE:		
#ifdef TACTILE
			if (TactileStick && objP==gameData.objs.console && !(FrameCount & 15))
				Tactile_Xvibrate_clear ();
	#endif
			break;

		#ifdef _DEBUG
		case HIT_BAD_P0:
			Int3 ();		// Unexpected collision type: start point not in specified segment.
#if TRACE				
			con_printf (CON_DEBUG, "Warning: Bad p0 in physics!!!\n");
#endif
			break;
		default:
			// Unknown collision type returned from FindVectorIntersection!!
			Int3 ();
			break;
		#endif
		}
	} while (bRetry);
	//	Pass retry count info to AI.
	if (objP->control_type == CT_AI) {
		if (count > 0) {
			gameData.ai.localInfo [objnum].retry_count = count-1;
#ifdef _DEBUG
			Total_retries += count-1;
			Total_sims++;
#endif
			}
		}

	//I'm not sure why we do this.  I wish there were a comment that
	//explained it.  I think maybe it only needs to be done if the object
	//is sliding, but I don't know
	if (!(gameStates.gameplay.bSpeedBoost || bObjStopped || bBounced))	{	//Set velocity from actual movement
		vms_vector vMoved;
		VmVecSub (&vMoved, &objP->pos, &vStartPos);
		VmVecCopyScale (&objP->mtype.phys_info.velocity, &vMoved, 
								 FixMulDiv (FixDiv (f1_0, gameData.time.xFrame), 100, xTimeScale));
#ifdef BUMP_HACK
		if (objP == gameData.objs.console && 
			 objP->mtype.phys_info.velocity.x==0 && 
			 objP->mtype.phys_info.velocity.y==0 && 
			 objP->mtype.phys_info.velocity.z==0 &&
			 (objP->mtype.phys_info.thrust.x!=0 || 
			  objP->mtype.phys_info.thrust.y!=0 ||
			  objP->mtype.phys_info.thrust.z!=0)) {
			vms_vector vCenter, vBump;
			//bump player a little towards vCenter of segment to unstick
			COMPUTE_SEGMENT_CENTER_I (&vCenter, objP->segnum);
			//HUDMessage (0, "BUMP! %d %d", d1, d2);
			//don't bump player toward vCenter of reactor segment
			if (gameData.segs.segment2s [objP->segnum].special == SEGMENT_IS_CONTROLCEN)
				VmVecNegate (&vBump);
			VmVecScaleInc (&objP->pos, &vBump, objP->size/5);
			//if moving away from seg, might move out of seg, so update
			if (gameData.segs.segment2s [objP->segnum].special == SEGMENT_IS_CONTROLCEN)
				UpdateObjectSeg (objP);
			}
#endif
		}

	//Assert (check_point_in_seg (&objP->pos, objP->segnum, 0).centerMask==0);
	//if (objP->control_type == CT_FLYING)
	if (objP->mtype.phys_info.flags & PF_LEVELLING)
		DoPhysicsAlignObject (objP);

	//hack to keep player from going through closed doors
	if (objP->type==OBJ_PLAYER && objP->segnum!=orig_segnum && 
		 (gameStates.app.cheats.bPhysics!=0xBADA55!=0xBADA55)) {
		int sidenum;
		sidenum = FindConnectedSide (gameData.segs.segments + objP->segnum, gameData.segs.segments + orig_segnum);
		if (sidenum != -1) {
			if (!(WALL_IS_DOORWAY (gameData.segs.segments + orig_segnum, sidenum, NULL) & WID_FLY_FLAG)) {
				side *sideP;
				int nVertex, nFaces;
				fix dist;
				int vertex_list [6];

				//bump object back
				sideP = gameData.segs.segments [orig_segnum].sides + sidenum;
				if (orig_segnum==-1)
					Error ("orig_segnum == -1 in physics");
				CreateAbsVertexLists (&nFaces, vertex_list, orig_segnum, sidenum);
				//let'sideP pretend this wall is not triangulated
				nVertex = vertex_list [0];
				if (nVertex > vertex_list [1])
					nVertex = vertex_list [1];
				if (nVertex > vertex_list [2])
					nVertex = vertex_list [2];
				if (nVertex > vertex_list [3])
					nVertex = vertex_list [3];
#ifdef COMPACT_SEGS
				{
					vms_vector _vn;
					get_side_normal (gameData.segs.segments + orig_segnum, sidenum, 0, &_vn);
					dist = VmDistToPlane (&vStartPos, &_vn, &gameData.segs.vertices [nVertex]);
					VmVecScaleAdd (&objP->pos, &vStartPos, &_vn, objP->size-dist);
				}
#else
				dist = VmDistToPlane (&vStartPos, sideP->normals, gameData.segs.vertices + nVertex);
				VmVecScaleAdd (&objP->pos, &vStartPos, sideP->normals, objP->size-dist);
#endif
				UpdateObjectSeg (objP);
				}
			}
		}

	//if end point not in segment, move object to last pos, or segment center
	if (GetSegMasks (&objP->pos, objP->segnum, 0).centerMask) {
		if (FindObjectSeg (objP) == -1) {
			int n;

			if (objP->type==OBJ_PLAYER && (n=FindSegByPoint (&objP->last_pos, objP->segnum))!=-1) {
				objP->pos = objP->last_pos;
				RelinkObject (objnum, n);
				}
			else {
				COMPUTE_SEGMENT_CENTER_I (&objP->pos, objP->segnum);
				objP->pos.x += objnum;
				}
			if (objP->type == OBJ_WEAPON)
				objP->flags |= OF_SHOULD_BE_DEAD;
		}
	}
UnstickObject (objP);
}
#else

void DoPhysicsSim (object *objP)
{
	short				ignoreObjList [MAX_IGNORE_OBJS], nIgnoreObjs;
	int				iseg;
	int				bRetry;
	int				fate;
	vms_vector		vFrame;			//movement in this frame
	vms_vector		vNewPos, ipos;		//position after this frame
	int				count=0;
	short				objnum;
	short				nWallHitSeg, nWallHitSide;
	fvi_info			hi;
	fvi_query		fq;
	vms_vector		vSavePos;
	int				nSaveSeg;
	fix				xDrag;
	fix				xSimTime, xOldSimTime, xTimeScale;
	vms_vector		vStartPos;
	int				bObjStopped=0;
	fix				xMovedTime;			//how long objected moved before hit something
	vms_vector		vSaveP0, vSaveP1;
	physics_info	*pi;
	short				orig_segnum = objP->segnum;
	int				bBounced=0;
	int				bDoSpeedBoost = gameStates.gameplay.bSpeedBoost && (objP == gameData.objs.console);

Assert (objP->type != OBJ_NONE);
Assert (objP->movement_type == MT_PHYSICS);

#ifdef _DEBUG
if (bDontMoveAIObjects)
	if (objP->control_type == CT_AI)
		return;
#endif
	pi = &objP->mtype.phys_info;
	DoPhysicsSimRot (objP);
	if (!(pi->velocity.x || pi->velocity.y || pi->velocity.z)) {
		if (objP == gameData.objs.console)
			gameStates.gameplay.bSpeedBoost = 0;
		if (!(pi->thrust.x || pi->thrust.y || pi->thrust.z))
			return;
		}
	objnum = OBJ_IDX (objP);
	nPhysSegs = 0;
	bSimpleFVI = (objP->type != OBJ_PLAYER);
	xSimTime = gameData.time.xFrame;
//debugObjP = obj;
	vStartPos = objP->pos;
	nIgnoreObjs = 0;
	Assert (objP->mtype.phys_info.brakes==0);		//brakes not used anymore?
	//if uses thrust, cannot have zero xDrag
	Assert (!(objP->mtype.phys_info.flags&PF_USES_THRUST) || objP->mtype.phys_info.drag!=0);
	//do thrust & xDrag
	//if (!bDoSpeedBoost) 
	{
		if (xDrag = objP->mtype.phys_info.drag) {

			int count;
			vms_vector accel, *vel = &objP->mtype.phys_info.velocity;
			fix r, k, d, a;

			d = f1_0 - xDrag;
			count = xSimTime / FT;
			r = xSimTime % FT;
			k = FixDiv (r, FT);
			if (objP->mtype.phys_info.flags & PF_USES_THRUST) {
				VmVecCopyScale (&accel, &objP->mtype.phys_info.thrust, FixDiv (f1_0, objP->mtype.phys_info.mass));
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
					VmVecScale (vel, f1_0 - FixMul (k, xDrag));
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
				fix xTotalDrag = f1_0;
				while (count--)
					xTotalDrag = FixMul (xTotalDrag, d);
				//do linear scale on remaining bit of time
				xTotalDrag = FixMul (xTotalDrag, f1_0-FixMul (k, xDrag));
				VmVecScale (&objP->mtype.phys_info.velocity, xTotalDrag);
			}
		}
	}
#if FLUID_PHYSICS
	if (EGI_FLAG (bFluidPhysics, 0, 0)) {
		if (gameData.segs.segment2s [objP->segnum].special == SEGMENT_IS_WATER)
			xTimeScale = 75;
		else if (gameData.segs.segment2s [objP->segnum].special == SEGMENT_IS_LAVA)
			xTimeScale = 66;
		else
			xTimeScale = 100;
		}
	else
#endif
		xTimeScale = 100;
	do {
		bRetry = 0;
		//Move the object
		VmVecCopyScale (
			&vFrame, 
			&objP->mtype.phys_info.velocity, 
#if FLUID_PHYSICS
			FixMulDiv (xSimTime, xTimeScale, 100)
#else
			xSimTime
#endif
);
		if ((vFrame.x==0) && (vFrame.y==0) && (vFrame.z==0))	
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
		VmVecAdd (&vNewPos, &objP->pos, &vFrame);
		ignoreObjList [nIgnoreObjs] = -1;
#ifdef _DEBUG
//iSegError:
#endif
		fq.p0 = &objP->pos;
		fq.p1 = &vNewPos;
		fq.rad = objP->size;
		fq.startSeg = objP->segnum;
		fq.thisObjNum = objnum;
		fq.ignoreObjList = ignoreObjList;
		fq.flags = FQ_CHECK_OBJS;

		if (objP->type == OBJ_WEAPON)
			fq.flags |= FQ_TRANSPOINT;
		if (objP->type == OBJ_PLAYER)
			fq.flags |= FQ_GET_SEGLIST;

//@@			if (GetSegMasks (&objP->pos, objP->segnum, 0).centerMask)
//@@				Int3 ();

vSaveP0 = *fq.p0;
vSaveP1 = *fq.p1;

		fate = FindVectorIntersection (&fq, &hi);
#ifdef _DEBUG
		if (fate == HIT_BAD_P0) {
			objP->segnum = FindSegByPoint (&objP->pos, objP->segnum);
			if (fq.startSeg != objP->segnum) {
				fq.startSeg = objP->segnum;
				fate = FindVectorIntersection (&fq, &hi);
				}
			}
		else if (fate)// == HIT_WALL)
			fate = FindVectorIntersection (&fq, &hi);
#endif
		//	Matt: Mike's hack.
		if (fate == HIT_OBJECT) {
			object	*objP = gameData.objs.objects + hi.hit.nObject;

			if ((objP->type == OBJ_WEAPON) && ((objP->id == PROXIMITY_ID) || (objP->id == SUPERPROX_ID)))
				count--;
		}

#ifdef _DEBUG
		if (fate == HIT_BAD_P0) {
#if TRACE				
			con_printf (CON_DEBUG, "Warning: Bad p0 in physics! Object = %i, type = %i [%s]\n", 
				OBJ_IDX (objP), objP->type, szObjectTypeNames [objP->type]);
#endif
			Int3 ();
		}
#endif

		if (objP->type == OBJ_PLAYER) {
			int i;

			if (nPhysSegs && physSegList [nPhysSegs-1] == hi.segList [0])
				nPhysSegs--;

			for (i = 0; (i < hi.nSegments) && (nPhysSegs < MAX_FVI_SEGS-1);)
				physSegList [nPhysSegs++] = hi.segList [i++];
			}

		ipos = hi.hit.vPoint;
		iseg = hi.hit.nSegment;
		nWallHitSide = hi.hit.nSide;
		nWallHitSeg = hi.hit.nSideSegment;
		if (iseg==-1) {		//some sort of horrible error
#ifdef _DEBUG
#	if TRACE				
			con_printf (1, "iseg==-1 in physics! Object = %i, type = %i (%s)\n", 
				OBJ_IDX (objP), objP->type, szObjectTypeNames [objP->type]);
			fate = FindVectorIntersection (&fq, &hi);
//			CBRK (1);
//			goto iSegError;
#	endif
#endif
			//Int3 ();
			//COMPUTE_SEGMENT_CENTER (&ipos, &gameData.segs.segments [objP->segnum]);
			//ipos.x += objnum;
			//iseg = objP->segnum;
			//fate = HIT_NONE;
			if (objP->type == OBJ_WEAPON)
				objP->flags |= OF_SHOULD_BE_DEAD;
			break;
		}

		Assert (!((fate==HIT_WALL) && ((nWallHitSeg == -1) || (nWallHitSeg > gameData.segs.nLastSegment))));
		// update object's position and segment number
		vSavePos = objP->pos;
		nSaveSeg = objP->segnum;
		objP->pos = ipos;
		if (iseg != objP->segnum)
			RelinkObject (objnum, iseg);

		//if start point not in segment, move object to center of segment
		if (GetSegMasks (&objP->pos, objP->segnum, 0).centerMask) {
			int n = FindObjectSeg (objP);

			if (n == -1) {
				//Int3 ();
				if (objP->type==OBJ_PLAYER && (n=FindSegByPoint (&objP->last_pos, objP->segnum))!=-1) {
					objP->pos = objP->last_pos;
					RelinkObject (objnum, n);
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
			vms_vector vMoveNorm, vFrame;
			fix attempted_dist, actual_dist;
			xOldSimTime = xSimTime;
			actual_dist = VmVecNormalizedDir (&vMoveNorm, &objP->pos, &vSavePos);
			if (fate == HIT_WALL)
				HUDMessage (0, "HIT WALL");
			VmVecCopyNormalize (&vFrame, &vFrame);
			if ((fate == HIT_WALL) && (VmVecDot (&vMoveNorm, &vFrame) < 0)) {		//moved backwards
				//don't change position or xSimTime
				HUDMessage (0, "stopped");
				objP->pos = vSavePos;
				if (nSaveSeg != iseg)
					RelinkObject (objnum, nSaveSeg);
				if (bDoSpeedBoost) {
//					int h = FindSegByPoint (&vNewPos, -1);
#if 1
					objP->pos = vStartPos;
					SetSpeedBoostVelocity (objnum, -1, -1, -1, -1, -1, &vStartPos, &speedBoostDest, 0);
					VmVecCopyScale (&vFrame, &boostedVel, xSimTime);
#else
#	if 0
					vms_vector	v;
					v.x = (objP->mtype.phys_info.velocity.x < 0) ? F1_0 * -60 : F1_0 * 60;
					v.y = (objP->mtype.phys_info.velocity.y < 0) ? F1_0 * -60 : F1_0 * 60;
					v.z = (objP->mtype.phys_info.velocity.z < 0) ? F1_0 * -60 : F1_0 * 60;
					VmVecCopyScale (&vFrame, &v, xSimTime);
#	else
#		if 1

					VmVecCopyScale (&vFrame, &boostedVel, 
#			if FLUID_PHYSICS
						FixMulDiv (xSimTime, xTimeScale, 100)
#			else
						xSimTime
#			endif
						);
#		else
					vFrame = boostedVel;
#		endif
#	endif
					objP->mtype.phys_info.velocity = boostedVel;
#endif
					goto retryMove;
					}
				xMovedTime = 0;
				}
			else {
//retryMove2:
				//if (objP == debugObjP)
				//	//printf ("   vMoved = %x %x %x\n", XYZ (&vMoved);
				attempted_dist = VmVecMag (&vFrame);
				HUDMessage (0, "moved %d", attempted_dist);
				xSimTime = FixMulDiv (xSimTime, attempted_dist-actual_dist, attempted_dist);
				xMovedTime = xOldSimTime - xSimTime;
				if (xSimTime < 0 || xSimTime>xOldSimTime) {
#if TRACE				
					con_printf (CON_DEBUG, "Bogus xSimTime = %x, old = %x\n", xSimTime, xOldSimTime);
//					if (objP == debugObjP)
//						con_printf (CON_DEBUG, "   Bogus xSimTime = %x, old = %x, attempted_dist = %x, actual_dist = %x\n", xSimTime, xOldSimTime, attempted_dist, actual_dist);
					//Int3 (); Removed by Rob
#endif
					xSimTime = xOldSimTime;
					//WHY DOES THIS HAPPEN??
					xMovedTime = 0;
				}
			}
		}

		switch (fate) {
			case HIT_WALL: {
				vms_vector vMoved;
				fix xHitSpeed, xWallPart;
	
				// Find hit speed	
//				if (gameStates.gameplay.bSpeedBoost && (objP == gameData.objs.console))
//					break;

				VmVecSub (&vMoved, &objP->pos, &vSavePos);
				xWallPart = VmVecDot (&vMoved, &hi.hit.vNormal);
				if (xWallPart != 0 && xMovedTime>0 && (xHitSpeed=-FixDiv (xWallPart, xMovedTime))>0)
					CollideObjectWithWall (objP, xHitSpeed, nWallHitSeg, nWallHitSide, &hi.hit.vPoint);
				else
					ScrapeObjectOnWall (objP, nWallHitSeg, nWallHitSide, &hi.hit.vPoint);
				Assert (nWallHitSeg > -1);
				Assert (nWallHitSide > -1);
				if (!(objP->flags&OF_SHOULD_BE_DEAD))	{
					int forcefield_bounce;		//bounce off a forcefield

					Assert (gameStates.app.cheats.bBouncingWeapons || !(objP->mtype.phys_info.flags & PF_STICK && objP->mtype.phys_info.flags & PF_BOUNCE));	//can't be bounce and stick
					forcefield_bounce = (gameData.pig.tex.pTMapInfo [gameData.segs.segments [nWallHitSeg].sides [nWallHitSide].tmap_num].flags & TMI_FORCE_FIELD);
					if (!forcefield_bounce && (objP->mtype.phys_info.flags & PF_STICK)) {		//stop moving
						AddStuckObject (objP, nWallHitSeg, nWallHitSide);
						VmVecZero (&objP->mtype.phys_info.velocity);
						bObjStopped = 1;
						bRetry = 0;
						}
					else {					// Slide object along wall
						int bCheckVel=0;
						//We're constrained by wall, so subtract wall part from
						//velocity vector
						xWallPart = VmVecDot (&hi.hit.vNormal, &objP->mtype.phys_info.velocity);
						if (forcefield_bounce || (objP->mtype.phys_info.flags & PF_BOUNCE)) {		//bounce off wall
							xWallPart *= 2;	//Subtract out wall part twice to achieve bounce
							if (forcefield_bounce) {
								bCheckVel = 1;				//check for max velocity
								if (objP->type == OBJ_PLAYER)
									xWallPart *= 2;		//player bounce twice as much
								}
							if (objP->mtype.phys_info.flags & PF_BOUNCES_TWICE) {
								Assert (objP->mtype.phys_info.flags & PF_BOUNCE);
								if (objP->mtype.phys_info.flags & PF_BOUNCED_ONCE)
									objP->mtype.phys_info.flags &= ~ (PF_BOUNCE+PF_BOUNCED_ONCE+PF_BOUNCES_TWICE);
								else
									objP->mtype.phys_info.flags |= PF_BOUNCED_ONCE;
								}
							bBounced = 1;		//this object bBounced
						}
						VmVecScaleInc (&objP->mtype.phys_info.velocity, &hi.hit.vNormal, -xWallPart);
						if (bCheckVel) {
							fix vel = VmVecMagQuick (&objP->mtype.phys_info.velocity);
							if (vel > MAX_OBJECT_VEL)
								VmVecScale (&objP->mtype.phys_info.velocity, FixDiv (MAX_OBJECT_VEL, vel));
							}
						if (bBounced && objP->type == OBJ_WEAPON)
							VmVector2Matrix (&objP->orient, &objP->mtype.phys_info.velocity, &objP->orient.uvec, NULL);
						bRetry = 1;
					}
				}
				break;
			}

			case HIT_OBJECT: {
				vms_vector vOldVel;
				// Mark the hit object so that on a retry the fvi code
				// ignores this object.
				//if (gameStates.gameplay.bSpeedBoost && (objP == gameData.objs.console))
				//	break;
				Assert (hi.hit.nObject != -1);
				//	Calculcate the hit point between the two gameData.objs.objects.
				{	vms_vector	*ppos0, *ppos1, pos_hit;
					fix			size0, size1;
					ppos0 = &gameData.objs.objects [hi.hit.nObject].pos;
					ppos1 = &objP->pos;
					size0 = gameData.objs.objects [hi.hit.nObject].size;
					size1 = objP->size;
					Assert (size0+size1 != 0);	// Error, both sizes are 0, so how did they collide, anyway?!?
					//VmVecScale (VmVecSub (&pos_hit, ppos1, ppos0), FixDiv (size0, size0 + size1);
					//VmVecInc (&pos_hit, ppos0);
					VmVecSub (&pos_hit, ppos1, ppos0);
					VmVecScaleAdd (&pos_hit, ppos0, &pos_hit, FixDiv (size0, size0 + size1));
					vOldVel = objP->mtype.phys_info.velocity;
					CollideTwoObjects (objP, gameData.objs.objects + hi.hit.nObject, &pos_hit);
					if (gameStates.gameplay.bSpeedBoost && (objP == gameData.objs.console)))
						objP->mtype.phys_info.velocity = vOldVel;
				}

				// Let object continue its movement
				if (!bCollision)
					bRetry = 0;
				else if (!(objP->flags&OF_SHOULD_BE_DEAD)) {
					//objP->pos = vSavePos;

					if ((objP->mtype.phys_info.flags & PF_PERSISTENT) || 
						 ((vOldVel.x == objP->mtype.phys_info.velocity.x) && 
						  (vOldVel.y == objP->mtype.phys_info.velocity.y) && 
						  (vOldVel.z == objP->mtype.phys_info.velocity.z))) {
						//if (gameData.objs.objects [hi.hit.nObject].type == OBJ_POWERUP)
							ignoreObjList [nIgnoreObjs++] = hi.hit.nObject;
						bRetry = 1;
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

#ifdef _DEBUG
			case HIT_BAD_P0:
				Int3 ();		// Unexpected collision type: start point not in specified segment.
#if TRACE				
				con_printf (CON_DEBUG, "Warning: Bad p0 in physics!!!\n");
#endif
				break;
			default:
				// Unknown collision type returned from FindVectorIntersection!!
				Int3 ();
				break;
#endif
		}

	} while (bRetry);

	//	Pass retry count info to AI.
	if (objP->control_type == CT_AI) {
		if (count > 0) {
			gameData.ai.localInfo [objnum].retry_count = count-1;
#ifdef _DEBUG
			Total_retries += count-1;
			Total_sims++;
#endif
		}
	}

	//I'm not sure why we do this.  I wish there were a comment that
	//explained it.  I think maybe it only needs to be done if the object
	//is sliding, but I don't know
	if (!(gameStates.gameplay.bSpeedBoost || bObjStopped || bBounced))	{	//Set velocity from actual movement
		vms_vector vMoved;
		VmVecSub (&vMoved, &objP->pos, &vStartPos);
		if (vMoved.x || vMoved.y || vMoved.z)
			VmVecCopyScale (&objP->mtype.phys_info.velocity, &vMoved, 
#if FLUID_PHYSICS
				FixMulDiv (FixDiv (f1_0, gameData.time.xFrame), 100, xTimeScale)
#else
				FixDiv (f1_0, gameData.time.xFrame)
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
			COMPUTE_SEGMENT_CENTER_I (&center, objP->segnum);
			VmVecNormalizedDirQuick (&vBump, &center, &objP->pos);
			//don't bump player toward center of reactor segment
			if (gameData.segs.segment2s [objP->segnum].special == SEGMENT_IS_CONTROLCEN)
				VmVecNegate (&vBump);
			VmVecScaleInc (&objP->pos, &vBump, objP->size/5);
			//if moving away from seg, might move out of seg, so update
			if (gameData.segs.segment2s [objP->segnum].special == SEGMENT_IS_CONTROLCEN)
				UpdateObjectSeg (objP);
		}
#endif
	}

	//Assert (check_point_in_seg (&objP->pos, objP->segnum, 0).centerMask==0);
	//if (objP->control_type == CT_FLYING)
	if (objP->mtype.phys_info.flags & PF_LEVELLING)
		DoPhysicsAlignObject (objP);

	//hack to keep player from going through closed doors
	if ((objP->type == OBJ_PLAYER) && (objP->segnum != orig_segnum) && 
		 (gameStates.app.cheats.bPhysics != 0xBADA55)) {
		short sidenum = FindConnectedSide (gameData.segs.segments + objP->segnum, gameData.segs.segments + orig_segnum);
		if (sidenum != -1) {
			if (!(WALL_IS_DOORWAY (gameData.segs.segments + orig_segnum, sidenum, NULL) & WID_FLY_FLAG)) {
				side *s;
				int vertnum, num_faces, i;
				fix dist;
				int vertex_list [6];

				//bump object back
				s = gameData.segs.segments [orig_segnum].sides + sidenum;
				if (orig_segnum==-1)
					Error ("orig_segnum == -1 in physics");
				CreateAbsVertexLists (&num_faces, vertex_list, orig_segnum, sidenum);
				//let's pretend this wall is not triangulated
				vertnum = vertex_list [0];
				for (i=1;i<4;i++)
					if (vertex_list [i] < vertnum)
						vertnum = vertex_list [i];
				#ifdef COMPACT_SEGS
					{
					vms_vector _vn;
					GetSideNormal (gameData.segs.segments + orig_segnum, sidenum, 0, &_vn);
					dist = VmDistToPlane (&vStartPos, &_vn, &gameData.segs.vertices [vertnum]);
					VmVecScaleAdd (&objP->pos, &vStartPos, &_vn, objP->size-dist);
					}
				#else
					dist = VmDistToPlane (&vStartPos, s->normals, gameData.segs.vertices + vertnum);
					VmVecScaleAdd (&objP->pos, &vStartPos, s->normals, objP->size-dist);
				#endif
				UpdateObjectSeg (objP);

			}
		}
	}

//--WE ALWYS WANT THIS IN, MATT AND MIKE DECISION ON 12/10/94, TWO MONTHS AFTER FINAL 	#ifdef _DEBUG
	//if end point not in segment, move object to last pos, or segment center
	if (GetSegMasks (&objP->pos, objP->segnum, 0).centerMask) {
		if (FindObjectSeg (objP)==-1) {
			int n;

			//Int3 ();
			if (objP->type==OBJ_PLAYER && (n=FindSegByPoint (&objP->last_pos, objP->segnum))!=-1) {
				objP->pos = objP->last_pos;
				RelinkObject (objnum, n);
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
#endif
//Applies an instantaneous force on an object, resulting in an instantaneous
//change in velocity.

void PhysApplyForce (object *objP, vms_vector *vForce)
{

	//	Put in by MK on 2/13/96 for force getting applied to Omega blobs, which have 0 mass, 
	//	in collision with crazy reactor robot thing on d2levf-s.
if (objP->mtype.phys_info.mass == 0)
	return;
if (objP->movement_type != MT_PHYSICS)
	return;
#ifdef TACTILE
  if (TactileStick && obj==&gameData.objs.objects [gameData.multi.players [gameData.multi.nLocalPlayer].objnum])
	Tactile_apply_force (vForce, &objP->orient);
#endif
//Add in acceleration due to force
if (!gameStates.gameplay.bSpeedBoost || (objP != gameData.objs.console))
	VmVecScaleInc (&objP->mtype.phys_info.velocity, 
						vForce, 
						FixDiv (f1_0, objP->mtype.phys_info.mass));
}

//	----------------------------------------------------------------
//	Do *dest = *delta unless:
//				*delta is pretty small
//		and	they are of different signs.
void PhysicsSetRotVelAndSaturate (fix *dest, fix delta)
{
	if ((delta ^ *dest) < 0) {
		if (abs (delta) < F1_0/8) {
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
void physics_turn_towards_vector (vms_vector *goal_vector, object *objP, fix rate)
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

	VmExtractAnglesVector (&dest_angles, goal_vector);
	VmExtractAnglesVector (&cur_angles, &objP->orient.fvec);

	delta_p = (dest_angles.p - cur_angles.p);
	delta_h = (dest_angles.h - cur_angles.h);

	if (delta_p > F1_0/2) delta_p = dest_angles.p - cur_angles.p - F1_0;
	if (delta_p < -F1_0/2) delta_p = dest_angles.p - cur_angles.p + F1_0;
	if (delta_h > F1_0/2) delta_h = dest_angles.h - cur_angles.h - F1_0;
	if (delta_h < -F1_0/2) delta_h = dest_angles.h - cur_angles.h + F1_0;

	delta_p = FixDiv (delta_p, rate);
	delta_h = FixDiv (delta_h, rate);

	if (abs (delta_p) < F1_0/16) delta_p *= 4;
	if (abs (delta_h) < F1_0/16) delta_h *= 4;

	PhysicsSetRotVelAndSaturate (&rotvel_ptr->x, delta_p);
	PhysicsSetRotVelAndSaturate (&rotvel_ptr->y, delta_h);
	rotvel_ptr->z = 0;
}

//	-----------------------------------------------------------------------------
//	Applies an instantaneous whack on an object, resulting in an instantaneous
//	change in orientation.
void PhysApplyRot (object *objP, vms_vector *vForce)
{
	fix	rate, vecmag;

	if (objP->movement_type != MT_PHYSICS)
		return;

	vecmag = VmVecMag (vForce)/8;
	if (vecmag < F1_0/256)
		rate = 4*F1_0;
	else if (vecmag < objP->mtype.phys_info.mass >> 14)
		rate = 4*F1_0;
	else {
		rate = FixDiv (objP->mtype.phys_info.mass, vecmag);
		if (objP->type == OBJ_ROBOT) {
			if (rate < F1_0/4)
				rate = F1_0/4;
			//	Changed by mk, 10/24/95, claw guys should not slow down when attacking!
			if (!gameData.bots.pInfo [objP->id].thief && !gameData.bots.pInfo [objP->id].attack_type) {
				if (objP->ctype.ai_info.SKIP_AI_COUNT * gameData.time.xFrame < 3*F1_0/4) {
					fix	tval = FixDiv (F1_0, 8*gameData.time.xFrame);
					int	addval;

					addval = f2i (tval);

					if ((d_rand () * 2) < (tval & 0xffff))
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
	physics_turn_towards_vector (vForce, objP, rate);


}


//this routine will set the thrust for an object to a value that will
// (hopefully) maintain the object's current velocity
void set_thrust_from_velocity (object *objP)
{
	fix k;

	Assert (objP->movement_type == MT_PHYSICS);

	k = FixMulDiv (objP->mtype.phys_info.mass, objP->mtype.phys_info.drag, (f1_0-objP->mtype.phys_info.drag));

	VmVecCopyScale (&objP->mtype.phys_info.thrust, &objP->mtype.phys_info.velocity, k);

}


