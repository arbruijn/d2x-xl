/* $Id: aipath.c, v 1.5 2003/10/04 03:14:47 btb Exp $ */
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
 * AI path forming stuff.
 *
 * Old Log:
 * Revision 1.5  1995/10/26  14:12:03  allender
 * prototype functions for mcc compiler
 *
 * Revision 1.4  1995/10/25  09:38:22  allender
 * prototype some functions causing mcc grief
 *
 * Revision 1.3  1995/10/10  11:48:43  allender
 * PC ai code
 *
 * Revision 2.0  1995/02/27  11:30:48  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.101  1995/02/22  13:42:44  allender
 * remove anonymous unions for object structure
 *
 * Revision 1.100  1995/02/10  16:20:04  mike
 * fix bogosity in CreatePathPoints, assumed all gameData.objs.objects were robots.
 *
 * Revision 1.99  1995/02/07  21:09:30  mike
 * make run_from guys have diff level based speed.
 *
 * Revision 1.98  1995/02/04  17:28:29  mike
 * make station guys return better.
 *
 * Revision 1.97  1995/02/04  10:28:39  mike
 * fix compile error!
 *
 * Revision 1.96  1995/02/04  10:03:37  mike
 * Fly to exit cheat.
 *
 * Revision 1.95  1995/02/01  21:10:36  mike
 * Array name was dereferenced.  Not a bug, but unclean.
 *
 * Revision 1.94  1995/02/01  17:14:12  mike
 * comment out some common mprintfs which didn't matter.
 *
 * Revision 1.93  1995/01/30  13:01:23  mike
 * Make robots fire at player other than one they are controlled by sometimes.
 *
 * Revision 1.92  1995/01/29  22:29:32  mike
 * add more debug info for guys that get lost.
 *
 * Revision 1.91  1995/01/20  16:56:05  mike
 * station stuff.
 *
 * Revision 1.90  1995/01/18  10:59:45  mike
 * comment out some mprintfs.
 *
 * Revision 1.89  1995/01/17  16:58:34  mike
 * make path following work for multiplayer.
 *
 * Revision 1.88  1995/01/17  14:21:44  mike
 * make run_from guys run better.
 *
 * Revision 1.87  1995/01/14  17:09:04  mike
 * playing with crazy josh, he's kinda slow and dumb now.
 *
 * Revision 1.86  1995/01/13  18:52:28  mike
 * comment out int3.
 *
 * Revision 1.85  1995/01/05  09:42:11  mike
 * compile out code based on SHAREWARE.
 *
 * Revision 1.84  1995/01/02  12:38:32  mike
 * make crazy josh turn faster, therefore evade player better.
 *
 * Revision 1.83  1994/12/27  15:59:40  mike
 * tweak ai_multiplayer_awareness constants.
 *
 * Revision 1.82  1994/12/19  17:07:10  mike
 * deal with new ai_multiplayer_awareness which returns a value saying whether this object can be moved by this player.
 *
 * Revision 1.81  1994/12/15  13:04:30  mike
 * Replace gameData.multi.players [gameData.multi.nLocalPlayer].time_total references with gameData.time.xGame.
 *
 * Revision 1.80  1994/12/09  16:13:23  mike
 * remove debug code.
 *
 * Revision 1.79  1994/12/07  00:36:54  mike
 * make robots get out of matcens better and be aware of player.
 *
 * Revision 1.78  1994/11/30  00:59:05  mike
 * optimizations.
 *
 * Revision 1.77  1994/11/27  23:13:39  matt
 * Made changes for new con_printf calling convention
 *
 * Revision 1.76  1994/11/23  21:59:34  mike
 * comment out some mprintfs.
 *
 * Revision 1.75  1994/11/21  16:07:14  mike
 * flip PARALLAX flag, prevent annoying debug information.
 *
 * Revision 1.74  1994/11/19  15:13:28  mike
 * remove unused code and data.
 *
 * Revision 1.73  1994/11/17  14:53:15  mike
 * segment validation functions moved from editor to main.
 *
 * Revision 1.72  1994/11/16  23:38:42  mike
 * new improved boss teleportation behavior.
 *
 * Revision 1.71  1994/11/13  17:18:30  mike
 * debug code, then comment it out.
 *
 * Revision 1.70  1994/11/11  16:41:43  mike
 * flip the PARALLAX flag.
 *
 * Revision 1.69  1994/11/11  16:33:45  mike
 * twiddle the PARALLAX flag.
 *
 *
 * Revision 1.68  1994/11/10  21:32:29  mike
 * debug code.
 *
 * Revision 1.67  1994/11/10  20:15:07  mike
 * fix stupid bug: uninitialized pointer.
 *
 * Revision 1.66  1994/11/10  17:45:15  mike
 * debugging.
 *
 * Revision 1.65  1994/11/10  17:28:10  mike
 * debugging.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>		//	for printf ()
#include <stdlib.h>		// for d_rand () and qsort ()
#include <string.h>		// for memset ()

#include "inferno.h"
#include "mono.h"
#include "u_mem.h"
#include "3d.h"

#include "object.h"
#include "error.h"
#include "ai.h"
#include "robot.h"
#include "fvi.h"
#include "physics.h"
#include "wall.h"
#ifdef EDITOR
#include "editor/editor.h"
#endif
#include "player.h"
#include "fireball.h"
#include "game.h"
#include "gameseg.h"

#define	PARALLAX	0		//	If !0, then special debugging for Parallax eyes enabled.

//	Length in segments of avoidance path
#define	AVOID_SEG_LENGTH	7

#ifdef NDEBUG
#define	PATH_VALIDATION	0
#else
#define	PATH_VALIDATION	1
#endif

void validate_all_paths (void);
void ai_path_set_orient_and_vel (object *objP, vms_vector* goal_point, int player_visibility, vms_vector *vec_to_player);
void maybe_ai_path_garbage_collect (void);
void ai_path_garbage_collect (void);
#if PATH_VALIDATION
int validate_path (int debug_flag, point_seg* psegs, int num_points);
#endif

//	------------------------------------------------------------------------
void create_random_xlate (sbyte *xt)
{
	int	i;

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++)
		xt [i] = i;

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		int	j = (d_rand ()*MAX_SIDES_PER_SEGMENT)/ (D_RAND_MAX+1);
		sbyte temp_byte;
		Assert ((j >= 0) && (j < MAX_SIDES_PER_SEGMENT));

		temp_byte = xt [j];
		xt [j] = xt [i];
		xt [i] = temp_byte;
	}

}

//	-----------------------------------------------------------------------------------------------------------
//	Insert the point at the center of the side connecting two segments between the two points.
// This is messy because we must insert into the list.  The simplest (and not too slow) way to do this is to start
// at the end of the list and go backwards.
void insert_center_points (point_seg *psegs, int *num_points)
{
	int	i, j, last_point;
	int	count=*num_points;

	last_point = *num_points-1;

	for (i=last_point; i>0; i--) {
		int			connect_side, temp_segnum;
		vms_vector	center_point, new_point;

		psegs [2*i] = psegs [i];
		connect_side = FindConnectedSide (&gameData.segs.segments [psegs [i].segnum], &gameData.segs.segments [psegs [i-1].segnum]);
		Assert (connect_side != -1);	//	Impossible! These two segments must be connected, they were created by CreatePathPoints (which was created by mk!)
		if (connect_side == -1)			//	Try to blow past the assert, this should at least prevent a hang.
			connect_side = 0;
		COMPUTE_SIDE_CENTER (&center_point, &gameData.segs.segments [psegs [i-1].segnum], connect_side);
		VmVecSub (&new_point, &psegs [i-1].point, &center_point);
		new_point.x /= 16;
		new_point.y /= 16;
		new_point.z /= 16;
		VmVecSub (&psegs [2*i-1].point, &center_point, &new_point);
		temp_segnum = FindSegByPoint (&psegs [2*i-1].point, psegs [2*i].segnum);
		if (temp_segnum == -1) {
#if TRACE
			con_printf (1, "Warning: point not in ANY segment in aipath.c/insert_center_points.\n");
#endif
			psegs [2*i-1].point = center_point;
			FindSegByPoint (&psegs [2*i-1].point, psegs [2*i].segnum);
		}

		psegs [2*i-1].segnum = psegs [2*i].segnum;
		count++;
	}

	//	Now, remove unnecessary center points.
	//	A center point is unnecessary if it is close to the line between the two adjacent points.
	//	MK, OPTIMIZE! Can get away with about half the math since every vector gets computed twice.
	for (i=1; i<count-1; i+=2) {
		vms_vector	temp1, temp2;
		fix			dot;

		VmVecSub (&temp1, &psegs [i].point, &psegs [i-1].point);
		VmVecSub (&temp2, &psegs [i+1].point, &psegs [i].point);
		dot = VmVecDot (&temp1, &temp2);

		if (dot * 9/8 > FixMul (VmVecMag (&temp1), VmVecMag (&temp2)))
			psegs [i].segnum = -1;

	}

	//	Now, scan for points with segnum == -1
	j = 0;
	for (i=0; i<count; i++)
		if (psegs [i].segnum != -1)
			psegs [j++] = psegs [i];

	*num_points = j;
}

#ifdef EDITOR
int	Safety_flag_override = 0;
int	Random_flag_override = 0;
int	Ai_path_debug=0;
#endif

//	-----------------------------------------------------------------------------------------------------------
//	Move points halfway to outside of segment.
static point_seg newPtSegs [MAX_SEGMENTS];

void MoveTowardsOutside (point_seg *ptSegs, int *nPoints, object *objP, int bRandom)
{
	int			i, j;
	int			nNewSeg;
	fix			xSegSize;
	int			nSegment;
	vms_vector	a, b, c, d, e;
	vms_vector	vGoalPos;
	int			count;
	int			nTempSeg;
	fvi_query	fq;
	fvi_info		hit_data;
	int			nHitType;

j = *nPoints;
if (j > MAX_SEGMENTS)
	j = MAX_SEGMENTS;
for (i = 1, j--; i < j; i++) {
	// -- ptSegs [i].nSegment = FindSegByPoint (&ptSegs [i].point, ptSegs [i].nSegment);
	nTempSeg = FindSegByPoint (&ptSegs [i].point, ptSegs [i].segnum);
	Assert (nTempSeg != -1);
	ptSegs [i].segnum = nTempSeg;
	nSegment = ptSegs [i].segnum;

	VmVecSub (&a, &ptSegs [i].point, &ptSegs [i-1].point);
	VmVecSub (&b, &ptSegs [i+1].point, &ptSegs [i].point);
	VmVecSub (&c, &ptSegs [i+1].point, &ptSegs [i-1].point);
	//	I don't think we can use quick version here and this is _very_ rarely called. --MK, 07/03/95
	VmVecNormalize (&a);
	VmVecNormalize (&b);
	if (abs (VmVecDot (&a, &b)) > 3*F1_0/4 ) {
		if (abs (a.z) < F1_0/2) {
			if (bRandom) {
				e.x = (d_rand ()-16384)/2;
				e.y = (d_rand ()-16384)/2;
				e.z = abs (e.x) + abs (e.y) + 1;
				VmVecNormalizeQuick (&e);
				} 
			else {
				e.x =
				e.y = 0;
				e.z = F1_0;
				}
			} 
		else {
			if (bRandom) {
				e.y = (d_rand ()-16384)/2;
				e.z = (d_rand ()-16384)/2;
				e.x = abs (e.y) + abs (e.z) + 1;
				VmVecNormalize (&e);
				}
			else {
				e.x = F1_0;
				e.y =
				e.z = 0;
				}
			}
		}
	else {
		VmVecCross (&d, &a, &b);
		VmVecCross (&e, &c, &d);
		VmVecNormalize (&e);
		}
#ifdef _DEBUG
	if (VmVecMag (&e) < F1_0/2)
		Int3 ();
#endif
	xSegSize = VmVecDistQuick (&gameData.segs.vertices [gameData.segs.segments [nSegment].verts [0]], &gameData.segs.vertices [gameData.segs.segments [nSegment].verts [6]]);
	if (xSegSize > F1_0*40)
		xSegSize = F1_0*40;
	VmVecScaleAdd (&vGoalPos, &ptSegs [i].point, &e, xSegSize/4);
	count = 3;
	while (count) {
		fq.p0					= &ptSegs [i].point;
		fq.startSeg			= ptSegs [i].segnum;
		fq.p1					= &vGoalPos;
		fq.rad				= objP->size;
		fq.thisObjNum		= OBJ_IDX (objP);
		fq.ignoreObjList	= NULL;
		fq.flags				= 0;
		nHitType = FindVectorIntersection (&fq, &hit_data);
		if (nHitType == HIT_NONE)
			count = 0;
		else {
			if ((count == 3) && (nHitType == HIT_BAD_P0))
				Int3 ();
			vGoalPos.x = (fq.p0->x + hit_data.hit.vPoint.x)/2;
			vGoalPos.y = (fq.p0->y + hit_data.hit.vPoint.y)/2;
			vGoalPos.z = (fq.p0->z + hit_data.hit.vPoint.z)/2;
			count--;
			if (count == 0)	//	Couldn't move towards outside, that's ok, sometimes things can't be moved.
				vGoalPos = ptSegs [i].point;
			}
		}
	//	Only move towards outside if remained inside segment.
	nNewSeg = FindSegByPoint (&vGoalPos, ptSegs [i].segnum);
	if (nNewSeg == ptSegs [i].segnum) {
		newPtSegs [i].point = vGoalPos;
		newPtSegs [i].segnum = nNewSeg;
		}
	else {
		newPtSegs [i].point = ptSegs [i].point;
		newPtSegs [i].segnum = ptSegs [i].segnum;
		}
	}
memcpy (ptSegs + 1, newPtSegs + 1, j * sizeof (*ptSegs));
}


//	-----------------------------------------------------------------------------------------------------------
//	Create a path from objP->pos to the center of end_seg.
//	Return a list of (segment_num, point_locations) at psegs
//	Return number of points in *num_points.
//	if max_depth == -1, then there is no maximum depth.
//	If unable to create path, return -1, else return 0.
//	If random_flag !0, then introduce randomness into path by looking at sides in random order.  This means
//	that a path between two segments won't always be the same, unless it is unique.
//	If safety_flag is set, then additional points are added to "make sure" that points are reachable.  I would
//	like to say that it ensures that the object can move between the points, but that would require knowing what
//	the object is (which isn't passed, right?) and making fvi calls (slow, right?).  So, consider it the more_or_less_safe_flag.
//	If end_seg == -2, then end seg will never be found and this routine will drop out due to depth (probably called by create_n_segment_path).
int CreatePathPoints (object *objP, int start_seg, int end_seg, point_seg *psegs, short *num_points, 
							  int max_depth, int random_flag, int safety_flag, int avoid_seg)
{
	short			cur_seg;
	short			sidenum, snum;
	int			qtail = 0, qhead = 0;
	int			i;
	sbyte			visited [MAX_SEGMENTS];
	seg_seg		seg_queue [MAX_SEGMENTS];
	short			depth [MAX_SEGMENTS];
	int			cur_depth;
	sbyte			random_xlate [MAX_SIDES_PER_SEGMENT];
	point_seg	*original_psegs = psegs;
	int			l_num_points;

#if PATH_VALIDATION
	validate_all_paths ();
#endif

if ((objP->type == OBJ_ROBOT) && (objP->ctype.ai_info.behavior == AIB_RUN_FROM)) {
	if (avoid_seg != -32767) {
		random_flag = 1;
		avoid_seg = gameData.objs.console->segnum;
		}	
	// Int3 ();
	}

	if (max_depth == -1)
		max_depth = MAX_PATH_LENGTH;

	l_num_points = 0;
//random_flag = Random_flag_override; //!!debug!!
//safety_flag = Safety_flag_override; //!!debug!!

//	for (i=0; i<=gameData.segs.nLastSegment; i++) {
//		visited [i] = 0;
//		depth [i] = 0;
//	}
	memset (visited, 0, sizeof (visited [0])* (gameData.segs.nLastSegment+1));
	memset (depth, 0, sizeof (depth [0])* (gameData.segs.nLastSegment+1));

	//	If there is a segment we're not allowed to visit, mark it.
	if (avoid_seg != -1) {
		Assert (avoid_seg <= gameData.segs.nLastSegment);
		if ((start_seg != avoid_seg) && (end_seg != avoid_seg)) {
			visited [avoid_seg] = 1;
			depth [avoid_seg] = 0;
		} else
			; 
	}

	if (random_flag)
		create_random_xlate (random_xlate);

	cur_seg = start_seg;
	visited [cur_seg] = 1;
	cur_depth = 0;

	while (cur_seg != end_seg) {
		segment	*segp = gameData.segs.segments + cur_seg;

		if (random_flag)
			if (d_rand () < 8192)
				create_random_xlate (random_xlate);

		for (sidenum = 0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++) {
			snum = sidenum;
			if (random_flag)
				snum = random_xlate [sidenum];

			if (IS_CHILD (segp->children [snum]) && 
				 ((WALL_IS_DOORWAY (segp, snum, NULL) & WID_FLY_FLAG) || 
				  (AIDoorIsOpenable (objP, segp, snum)))) {
				int			this_seg = segp->children [snum];
				Assert (this_seg > -1 && this_seg <= gameData.segs.nLastSegment);
				if (( (cur_seg == avoid_seg) || (this_seg == avoid_seg)) && (gameData.objs.console->segnum == avoid_seg)) {
					vms_vector	center_point;

					fvi_query	fq;
					fvi_info		hit_data;
					int			hit_type;
	
					COMPUTE_SIDE_CENTER (&center_point, segp, snum);

					fq.p0						= &objP->pos;
					fq.startSeg				= objP->segnum;
					fq.p1						= &center_point;
					fq.rad					= objP->size;
					fq.thisObjNum			= OBJ_IDX (objP);
					fq.ignoreObjList	= NULL;
					fq.flags					= 0;

					hit_type = FindVectorIntersection (&fq, &hit_data);
					if (hit_type != HIT_NONE) {
						goto dont_add;
					}
				}

				if (!visited [this_seg]) {
				Assert (this_seg > -1 && this_seg <= gameData.segs.nLastSegment);
				Assert (cur_seg > -1 && cur_seg <= gameData.segs.nLastSegment);
					seg_queue [qtail].start = cur_seg;
					seg_queue [qtail].end = this_seg;
					visited [this_seg] = 1;
					depth [qtail++] = cur_depth+1;
					if (depth [qtail-1] == max_depth) {
						end_seg = seg_queue [qtail-1].end;
						goto cpp_done1;
					}	// end if (depth [...
				}	// end if (!visited...
			}	// if (WALL_IS_DOORWAY (...
dont_add: ;
		}	//	for (sidenum...

		if (qhead >= qtail) {
			//	Couldn't get to goal, return a path as far as we got, which probably acceptable to the unparticular caller.
			end_seg = seg_queue [qtail-1].end;
			break;
		}

		cur_seg = seg_queue [qhead].end;
		cur_depth = depth [qhead];
		qhead++;

cpp_done1: ;
	}	//	while (cur_seg ...

	//	Set qtail to the segment which ends at the goal.
	while (seg_queue [--qtail].end != end_seg)
		if (qtail < 0) {
			// printf ("UNABLE TO FORM PATH");
			// Int3 ();
			*num_points = l_num_points;
			return -1;
		}

	#ifdef EDITOR
	// -- N_selected_segs = 0;
	#endif
//printf ("Object #%3i, start: %3i ", OBJ_IDX (objP), psegs-gameData.ai.pointSegs);
	while (qtail >= 0) {
		int	parent_seg, this_seg;

		this_seg = seg_queue [qtail].end;
		parent_seg = seg_queue [qtail].start;
		Assert ((this_seg >= 0) && (this_seg <= gameData.segs.nLastSegment));
		psegs->segnum = this_seg;
//printf ("%3i ", this_seg);
		COMPUTE_SEGMENT_CENTER_I (&psegs->point, this_seg);
		psegs++;
		l_num_points++;
		#ifdef EDITOR
		// -- Selected_segs [N_selected_segs++] = this_seg;
		#endif

		if (parent_seg == start_seg)
			break;

		while (seg_queue [--qtail].end != parent_seg)
			Assert (qtail >= 0);
	}

	Assert ((start_seg >= 0) && (start_seg <= gameData.segs.nLastSegment));
	psegs->segnum = start_seg;
////printf ("%3i\n", start_seg);
	COMPUTE_SEGMENT_CENTER_I (&psegs->point, start_seg);
	psegs++;
	l_num_points++;

#if PATH_VALIDATION
	validate_path (1, original_psegs, l_num_points);
#endif

	//	Now, reverse point_segs in place.
	for (i=0; i< l_num_points/2; i++) {
		point_seg		temp_point_seg = * (original_psegs + i);
		* (original_psegs + i) = * (original_psegs + l_num_points - i - 1);
		* (original_psegs + l_num_points - i - 1) = temp_point_seg;
	}
#if PATH_VALIDATION
	validate_path (2, original_psegs, l_num_points);
#endif

	//	Now, if safety_flag set, then insert the point at the center of the side connecting two segments
	//	between the two points.  This is messy because we must insert into the list.  The simplest (and not too slow)
	//	way to do this is to start at the end of the list and go backwards.
	if (safety_flag) {
		if (psegs - gameData.ai.pointSegs + l_num_points + 2 > MAX_POINT_SEGS) {
			//	Ouch! Cannot insert center points in path.  So return unsafe path.
//			Int3 ();	// Contact Mike:  This is impossible.
//			force_dump_ai_objects_all ("Error in CreatePathPoints");
#if TRACE
			con_printf (CON_DEBUG, "Resetting all paths because of safety_flag.\n");
#endif
			AIResetAllPaths ();
			*num_points = l_num_points;
			return -1;
		} else {
			// int	old_num_points = l_num_points;
			insert_center_points (original_psegs, &l_num_points);
		}
	}

#if PATH_VALIDATION
	validate_path (3, original_psegs, l_num_points);
#endif

// -- MK, 10/30/95 -- This code causes apparent discontinuities in the path, moving a point
//	into a new segment.  It is not necessarily bad, but it makes it hard to track down actual
//	discontinuity problems.
	if (objP->type == OBJ_ROBOT)
		if (gameData.bots.pInfo [objP->id].companion)
			MoveTowardsOutside (original_psegs, &l_num_points, objP, 0);

#if PATH_VALIDATION
	validate_path (4, original_psegs, l_num_points);
#endif

	*num_points = l_num_points;
	return 0;
}

int	Last_buddy_polish_path_frame;

//	-------------------------------------------------------------------------------------------------------
//	polish_path
//	Takes an existing path and makes it nicer.
//	Drops as many leading points as possible still maintaining direct accessibility
//	from current position to first point.
//	Will not shorten path to fewer than 3 points.
//	Returns number of points.
//	Starting position in psegs doesn't change.
//	Changed, MK, 10/18/95.  I think this was causing robots to get hung up on walls.
//				Only drop up to the first three points.
int polish_path (object *objP, point_seg *psegs, int num_points)
{
	int	i, first_point=0;

	if (num_points <= 4)
		return num_points;

	//	Prevent the buddy from polishing his path twice in one frame, which can cause him to get hung up.  Pretty ugly, huh?
	if (gameData.bots.pInfo [objP->id].companion)
	{
		if (gameData.app.nFrameCount == Last_buddy_polish_path_frame)
			return num_points;
		else
			Last_buddy_polish_path_frame = gameData.app.nFrameCount;
	}

	// -- MK: 10/18/95: for (i=0; i<num_points-3; i++) {
	for (i=0; i<2; i++) {
		fvi_query	fq;
		fvi_info		hit_data;
		int			hit_type;
	
		fq.p0						= &objP->pos;
		fq.startSeg				= objP->segnum;
		fq.p1						= &psegs [i].point;
		fq.rad					= objP->size;
		fq.thisObjNum			= OBJ_IDX (objP);
		fq.ignoreObjList	= NULL;
		fq.flags					= 0;

		hit_type = FindVectorIntersection (&fq, &hit_data);
	
		if (hit_type == HIT_NONE)
			first_point = i+1;
		else
			break;		
	}

	if (first_point) {
		//	Scrunch down all the psegs.
		for (i=first_point; i<num_points; i++)
			psegs [i-first_point] = psegs [i];
	}

	return num_points - first_point;
}

#ifndef NDEBUG
//	-------------------------------------------------------------------------------------------------------
//	Make sure that there are connections between all segments on path.
//	Note that if path has been optimized, connections may not be direct, so this function is useless, or worse.
//	Return true if valid, else return false.
int validate_path (int debug_flag, point_seg *psegs, int num_points)
{
#if PATH_VALIDATION
	int		i, curseg;

	curseg = psegs->segnum;
	if ((curseg < 0) || (curseg > gameData.segs.nLastSegment)) {
#if TRACE
		con_printf (CON_DEBUG, "Path beginning at index %i, length=%i is bogus!\n", psegs-gameData.ai.pointSegs, num_points);
#endif
		Int3 ();		//	Contact Mike: Debug trap for elusive, nasty bug.
		return 0;
	}
#if TRACE
if (debug_flag == 999)
	con_printf (CON_DEBUG, "That's curious...\n");
#endif
if (num_points == 0)
	return 1;

// //printf (" (%i) Validating path at psegs=%i, num_points=%i, segments = %3i ", debug_flag, psegs-gameData.ai.pointSegs, num_points, psegs [0].segnum);
	for (i=1; i<num_points; i++) {
		int	sidenum;
		int	nextseg = psegs [i].segnum;

		if ((nextseg < 0) || (nextseg > gameData.segs.nLastSegment)) {
#if TRACE
			con_printf (CON_DEBUG, "Path beginning at index %i, length=%i is bogus!\n", psegs-gameData.ai.pointSegs, num_points);
#endif
			Int3 ();		//	Contact Mike: Debug trap for elusive, nasty bug.
			return 0;
		}

// //printf ("%3i ", nextseg);
		if (curseg != nextseg) {
			for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++)
				if (gameData.segs.segments [curseg].children [sidenum] == nextseg)
					break;

			// Assert (sidenum != MAX_SIDES_PER_SEGMENT);	//	Hey, created path is not contiguous, why!?
			if (sidenum == MAX_SIDES_PER_SEGMENT) {
#if TRACE
				con_printf (CON_DEBUG, "Path beginning at index %i, length=%i is bogus!\n", psegs-gameData.ai.pointSegs, num_points);
#endif
				// //printf ("BOGUS");
				Int3 ();
				return 0;
			}
			curseg = nextseg;
		}
	}
////printf ("\n");
#endif
	return 1;

}
#endif

#ifndef NDEBUG
//	-----------------------------------------------------------------------------------------------------------
void validate_all_paths (void)
{

#if PATH_VALIDATION
	int	i;

	for (i=0; i<=gameData.objs.nLastObject; i++) {
		if (gameData.objs.objects [i].type == OBJ_ROBOT) {
			object		*objP = &gameData.objs.objects [i];
			ai_static	*aip = &objP->ctype.ai_info;
			//ai_local		*ailp = &gameData.ai.localInfo [i];

			if (objP->control_type == CT_AI) {
				if ((aip->hide_index != -1) && (aip->path_length > 0))
					if (!validate_path (4, &gameData.ai.pointSegs [aip->hide_index], aip->path_length)) {
						Int3 ();	//	This path is bogus! Who corrupted it! Danger!Danger!
									//	Contact Mike, he caused this mess.
						//force_dump_ai_objects_all ("Error in validate_all_paths");
						aip->path_length=0;	//	This allows people to resume without harm...
					}
			}
		}
	}
#endif

}
#endif

// -- //	-------------------------------------------------------------------------------------------------------
// -- //	Creates a path from the gameData.objs.objects current segment (objP->segnum) to the specified segment for the object to
// -- //	hide in gameData.ai.localInfo [objnum].goal_segment.
// -- //	Sets	objP->ctype.ai_info.hide_index, 		a pointer into gameData.ai.pointSegs, the first point_seg of the path.
// -- //			objP->ctype.ai_info.path_length, 		length of path
// -- //			gameData.ai.freePointSegs				global pointer into gameData.ai.pointSegs array
// -- void create_path (object *objP)
// -- {
// -- 	ai_static	*aip = &objP->ctype.ai_info;
// -- 	ai_local		*ailp = &gameData.ai.localInfo [OBJ_IDX (objP)];
// -- 	int			start_seg, end_seg;
// --
// -- 	start_seg = objP->segnum;
// -- 	end_seg = ailp->goal_segment;
// --
// -- 	if (end_seg == -1)
// -- 		create_n_segment_path (objP, 3, -1);
// --
// -- 	if (end_seg == -1) {
// -- 		; //con_printf (CON_DEBUG, "Object %i, hide_segment = -1, not creating path.\n", OBJ_IDX (objP));
// -- 	} else {
// -- 		CreatePathPoints (objP, start_seg, end_seg, gameData.ai.freePointSegs, &aip->path_length, -1, 0, 0, -1);
// -- 		aip->hide_index = gameData.ai.freePointSegs - gameData.ai.pointSegs;
// -- 		aip->cur_path_index = 0;
// -- #if PATH_VALIDATION
// -- 		validate_path (5, gameData.ai.freePointSegs, aip->path_length);
// -- #endif
// -- 		gameData.ai.freePointSegs += aip->path_length;
// -- 		if (gameData.ai.freePointSegs - gameData.ai.pointSegs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
// -- 			//Int3 ();	//	Contact Mike: This is curious, though not deadly. /eip++;g
// -- 			//force_dump_ai_objects_all ("Error in create_path");
// -- 			AIResetAllPaths ();
// -- 		}
// -- 		aip->PATH_DIR = 1;		//	Initialize to moving forward.
// -- 		aip->SUBMODE = AISM_HIDING;		//	Pretend we are hiding, so we sit here until bothered.
// -- 	}
// --
// -- 	maybe_ai_path_garbage_collect ();
// --
// -- }

//	-------------------------------------------------------------------------------------------------------
//	Creates a path from the gameData.objs.objects current segment (objP->segnum) to the specified segment for the object to
//	hide in gameData.ai.localInfo [objnum].goal_segment.
//	Sets	objP->ctype.ai_info.hide_index, 		a pointer into gameData.ai.pointSegs, the first point_seg of the path.
//			objP->ctype.ai_info.path_length, 		length of path
//			gameData.ai.freePointSegs				global pointer into gameData.ai.pointSegs array
//	Change, 10/07/95: Used to create path to gameData.objs.console->pos.  Now creates path to gameData.ai.vBelievedPlayerPos.
void CreatePathToPlayer (object *objP, int max_length, int safety_flag)
{
	ai_static	*aip = &objP->ctype.ai_info;
	ai_local		*ailp = &gameData.ai.localInfo [OBJ_IDX (objP)];
	int			start_seg, end_seg;

	if (max_length == -1)
		max_length = MAX_DEPTH_TO_SEARCH_FOR_PLAYER;

	ailp->time_player_seen = gameData.time.xGame;			//	Prevent from resetting path quickly.
	ailp->goal_segment = gameData.ai.nBelievedPlayerSeg;

	start_seg = objP->segnum;
	end_seg = ailp->goal_segment;

	if (end_seg == -1) {
		; 
	} else {
		CreatePathPoints (objP, start_seg, end_seg, gameData.ai.freePointSegs, &aip->path_length, max_length, 1, safety_flag, -1);
		aip->path_length = polish_path (objP, gameData.ai.freePointSegs, aip->path_length);
		aip->hide_index = (int) (gameData.ai.freePointSegs - gameData.ai.pointSegs);
		aip->cur_path_index = 0;
		gameData.ai.freePointSegs += aip->path_length;
		if (gameData.ai.freePointSegs - gameData.ai.pointSegs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
			//Int3 ();	//	Contact Mike: This is stupid.  Should call maybe_ai_garbage_collect before the add.
			//force_dump_ai_objects_all ("Error in CreatePathToPlayer");
			AIResetAllPaths ();
			return;
		}
//		Assert (gameData.ai.freePointSegs - gameData.ai.pointSegs + MAX_PATH_LENGTH*2 < MAX_POINT_SEGS);
		aip->PATH_DIR = 1;		//	Initialize to moving forward.
		// -- UNUSED!aip->SUBMODE = AISM_GOHIDE;		//	This forces immediate movement.
		ailp->mode = AIM_FOLLOW_PATH;
		ailp->player_awareness_type = 0;		//	If robot too aware of player, will set mode to chase
	}

	maybe_ai_path_garbage_collect ();

}

//	-------------------------------------------------------------------------------------------------------
//	Creates a path from the object's current segment (objP->segnum) to segment goalseg.
void CreatePathToSegment (object *objP, short goalseg, int max_length, int safety_flag)
{
	ai_static	*aip = &objP->ctype.ai_info;
	ai_local		*ailp = &gameData.ai.localInfo [OBJ_IDX (objP)];
	short			start_seg, end_seg;

	if (max_length == -1)
		max_length = MAX_DEPTH_TO_SEARCH_FOR_PLAYER;

	ailp->time_player_seen = gameData.time.xGame;			//	Prevent from resetting path quickly.
	ailp->goal_segment = goalseg;

	start_seg = objP->segnum;
	end_seg = ailp->goal_segment;

	if (end_seg == -1) {
		;
	} else {
		CreatePathPoints (objP, start_seg, end_seg, gameData.ai.freePointSegs, &aip->path_length, max_length, 1, safety_flag, -1);
		aip->hide_index = (int) (gameData.ai.freePointSegs - gameData.ai.pointSegs);
		aip->cur_path_index = 0;
		gameData.ai.freePointSegs += aip->path_length;
		if (gameData.ai.freePointSegs - gameData.ai.pointSegs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
			AIResetAllPaths ();
			return;
		}

		aip->PATH_DIR = 1;		//	Initialize to moving forward.
		// -- UNUSED!aip->SUBMODE = AISM_GOHIDE;		//	This forces immediate movement.
		ailp->player_awareness_type = 0;		//	If robot too aware of player, will set mode to chase
	}

	maybe_ai_path_garbage_collect ();

}

//	-------------------------------------------------------------------------------------------------------
//	Creates a path from the gameData.objs.objects current segment (objP->segnum) to the specified segment for the object to
//	hide in gameData.ai.localInfo [objnum].goal_segment
//	Sets	objP->ctype.ai_info.hide_index, 		a pointer into gameData.ai.pointSegs, the first point_seg of the path.
//			objP->ctype.ai_info.path_length, 		length of path
//			gameData.ai.freePointSegs				global pointer into gameData.ai.pointSegs array
void create_path_to_station (object *objP, int max_length)
{
	ai_static	*aip = &objP->ctype.ai_info;
	ai_local		*ailp = &gameData.ai.localInfo [OBJ_IDX (objP)];
	int			start_seg, end_seg;

	if (max_length == -1)
		max_length = MAX_DEPTH_TO_SEARCH_FOR_PLAYER;

	ailp->time_player_seen = gameData.time.xGame;			//	Prevent from resetting path quickly.

	start_seg = objP->segnum;
	end_seg = aip->hide_segment;


	if (end_seg == -1) {
		; 
	} else {
		CreatePathPoints (objP, start_seg, end_seg, gameData.ai.freePointSegs, &aip->path_length, max_length, 1, 1, -1);
		aip->path_length = polish_path (objP, gameData.ai.freePointSegs, aip->path_length);
		aip->hide_index = (int) (gameData.ai.freePointSegs - gameData.ai.pointSegs);
		aip->cur_path_index = 0;

		gameData.ai.freePointSegs += aip->path_length;
		if (gameData.ai.freePointSegs - gameData.ai.pointSegs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
			//Int3 ();	//	Contact Mike: Stupid.
			//force_dump_ai_objects_all ("Error in create_path_to_station");
			AIResetAllPaths ();
			return;
		}
//		Assert (gameData.ai.freePointSegs - gameData.ai.pointSegs + MAX_PATH_LENGTH*2 < MAX_POINT_SEGS);
		aip->PATH_DIR = 1;		//	Initialize to moving forward.
		// aip->SUBMODE = AISM_GOHIDE;		//	This forces immediate movement.
		ailp->mode = AIM_FOLLOW_PATH;
		ailp->player_awareness_type = 0;
	}


	maybe_ai_path_garbage_collect ();

}


//	-------------------------------------------------------------------------------------------------------
//	Create a path of length path_length for an object, stuffing info in ai_info field.
void create_n_segment_path (object *objP, int path_length, short avoid_seg)
{
	ai_static	*aip=&objP->ctype.ai_info;
	ai_local		*ailp = &gameData.ai.localInfo [OBJ_IDX (objP)];

	if (CreatePathPoints (objP, objP->segnum, -2, gameData.ai.freePointSegs, &aip->path_length, path_length, 1, 0, avoid_seg) == -1) {
		gameData.ai.freePointSegs += aip->path_length;
		while ((CreatePathPoints (objP, objP->segnum, -2, gameData.ai.freePointSegs, &aip->path_length, --path_length, 1, 0, -1) == -1)) {
			Assert (path_length);
		}
	}

	aip->hide_index = (int) (gameData.ai.freePointSegs - gameData.ai.pointSegs);
	aip->cur_path_index = 0;
#if PATH_VALIDATION
	validate_path (8, gameData.ai.freePointSegs, aip->path_length);
#endif
	gameData.ai.freePointSegs += aip->path_length;
	if (gameData.ai.freePointSegs - gameData.ai.pointSegs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
		//Int3 ();	//	Contact Mike: This is curious, though not deadly. /eip++;g
		//force_dump_ai_objects_all ("Error in crete_n_segment_path 2");
		AIResetAllPaths ();
	}

	aip->PATH_DIR = 1;		//	Initialize to moving forward.
	// -- UNUSED!aip->SUBMODE = -1;		//	Don't know what this means.
	ailp->mode = AIM_FOLLOW_PATH;

	//	If this robot is visible (player_visibility is not available) and it's running away, move towards outside with
	//	randomness to prevent a stream of bots from going away down the center of a corridor.
	if (gameData.ai.localInfo [OBJ_IDX (objP)].previous_visibility) {
		if (aip->path_length) {
			int	t_num_points = aip->path_length;
			MoveTowardsOutside (&gameData.ai.pointSegs [aip->hide_index], &t_num_points, objP, 1);
			aip->path_length = t_num_points;
		}
	}
	maybe_ai_path_garbage_collect ();

}

//	-------------------------------------------------------------------------------------------------------
void create_n_segment_path_to_door (object *objP, int path_length, short avoid_seg)
{
	create_n_segment_path (objP, path_length, avoid_seg);
}

extern int Connected_segment_distance;

#define Int3_if (cond) if (!cond) Int3 ();

//	----------------------------------------------------------------------------------------------------
void move_object_to_goal (object *objP, vms_vector *goal_point, short goal_seg)
{
	ai_static	*aip = &objP->ctype.ai_info;
	int			segnum;

	if (aip->path_length < 2)
		return;

	Assert (objP->segnum != -1);

#ifndef NDEBUG
	if (objP->segnum != goal_seg)
		if (FindConnectedSide (&gameData.segs.segments [objP->segnum], &gameData.segs.segments [goal_seg]) == -1) {
			fix	dist;
			dist = FindConnectedDistance (&objP->pos, objP->segnum, goal_point, goal_seg, 30, WID_FLY_FLAG);
			if (Connected_segment_distance > 2) {	//	This global is set in FindConnectedDistance
				// -- Int3 ();
#if TRACE
				con_printf (1, "Warning: Object %i hopped across %i segments, a distance of %7.3f.\n", OBJ_IDX (objP), Connected_segment_distance, f2fl (dist));
#endif
			}
		}
#endif

	Assert (aip->path_length >= 2);

	if (aip->cur_path_index <= 0) {
		if (aip->behavior == AIB_STATION) {
			create_path_to_station (objP, 15);
			return;
		}
		aip->cur_path_index = 1;
		aip->PATH_DIR = 1;
	} else if (aip->cur_path_index >= aip->path_length - 1) {
		if (aip->behavior == AIB_STATION) {
			create_path_to_station (objP, 15);
			if (aip->path_length == 0) {
				ai_local		*ailp = &gameData.ai.localInfo [OBJ_IDX (objP)];
				ailp->mode = AIM_STILL;
			}
			return;
		}
		Assert (aip->path_length != 0);
		aip->cur_path_index = aip->path_length-2;
		aip->PATH_DIR = -1;
	} else
		aip->cur_path_index += aip->PATH_DIR;

	//--Int3_if (( (aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length));

	objP->pos = *goal_point;
	segnum = FindObjectSeg (objP);
	if (segnum != goal_seg) {
#if TRACE
		con_printf (1, "Object #%i goal supposed to be in segment #%i, but in segment #%i\n", OBJ_IDX (objP), goal_seg, segnum);
#endif
	}
	if (segnum == -1) {
		Int3 ();	//	Oops, object is not in any segment.
					// Contact Mike: This is impossible.
		//	Hack, move object to center of segment it used to be in.
		COMPUTE_SEGMENT_CENTER_I (&objP->pos, objP->segnum);
	} else
		RelinkObject (OBJ_IDX (objP), segnum);
}

// -- too much work -- //	----------------------------------------------------------------------------------------------------------
// -- too much work -- //	Return true if the object the companion wants to kill is reachable.
// -- too much work -- int attack_kill_object (object *objP)
// -- too much work -- {
// -- too much work -- 	object		*kill_objp;
// -- too much work -- 	fvi_info		hit_data;
// -- too much work -- 	int			fate;
// -- too much work -- 	fvi_query	fq;
// -- too much work --
// -- too much work -- 	if (gameData.escort.nKillObject == -1)
// -- too much work -- 		return 0;
// -- too much work --
// -- too much work -- 	kill_objp = &gameData.objs.objects [gameData.escort.nKillObject];
// -- too much work --
// -- too much work -- 	fq.p0						= &objP->pos;
// -- too much work -- 	fq.startSeg				= objP->segnum;
// -- too much work -- 	fq.p1						= &kill_objP->pos;
// -- too much work -- 	fq.rad					= objP->size;
// -- too much work -- 	fq.thisObjNum			= OBJ_IDX (objP);
// -- too much work -- 	fq.ignoreObjList	= NULL;
// -- too much work -- 	fq.flags					= 0;
// -- too much work --
// -- too much work -- 	fate = FindVectorIntersection (&fq, &hit_data);
// -- too much work --
// -- too much work -- 	if (fate == HIT_NONE)
// -- too much work -- 		return 1;
// -- too much work -- 	else
// -- too much work -- 		return 0;
// -- too much work -- }

//	----------------------------------------------------------------------------------------------------------
//	Optimization: If current velocity will take robot near goal, don't change velocity
void ai_follow_path (object *objP, int player_visibility, int previous_visibility, vms_vector *vec_to_player)
{
	ai_static		*aip = &objP->ctype.ai_info;

	vms_vector	goal_point, new_goal_point;
	fix			dist_to_goal;
	robot_info	*robptr = &gameData.bots.pInfo [objP->id];
	int			forced_break, original_dir, original_index;
	fix			dist_to_player;
	short			goal_seg;
	ai_local		*ailp = gameData.ai.localInfo + OBJ_IDX (objP);
	fix			threshold_distance;


	if ((aip->hide_index == -1) || (aip->path_length == 0))
	{
		if (ailp->mode == AIM_RUN_FROM_OBJECT) {
			create_n_segment_path (objP, 5, -1);
			//--Int3_if ((aip->path_length != 0);
			ailp->mode = AIM_RUN_FROM_OBJECT;
		} else {
			create_n_segment_path (objP, 5, -1);
			//--Int3_if ((aip->path_length != 0);
		}
	}

	if ((aip->hide_index + aip->path_length > gameData.ai.freePointSegs - gameData.ai.pointSegs) && (aip->path_length>0)) {
		Int3 ();	//	Contact Mike: Bad.  Path goes into what is believed to be d_free space.
		//	This is debugging code.  Figure out why garbage collection
		//	didn't compress this object's path information.
		ai_path_garbage_collect ();
		//force_dump_ai_objects_all ("Error in ai_follow_path");
		AIResetAllPaths ();
	}

	if (aip->path_length < 2) {
		if ((aip->behavior == AIB_SNIPE) || (ailp->mode == AIM_RUN_FROM_OBJECT)) {
			if (gameData.objs.console->segnum == objP->segnum) {
				create_n_segment_path (objP, AVOID_SEG_LENGTH, -1);			//	Can't avoid segment player is in, robot is already in it!(That's what the -1 is for)
				//--Int3_if ((aip->path_length != 0);
			} else {
				create_n_segment_path (objP, AVOID_SEG_LENGTH, gameData.objs.console->segnum);
				//--Int3_if ((aip->path_length != 0);
			}
			if (aip->behavior == AIB_SNIPE) {
				if (robptr->thief)
					ailp->mode = AIM_THIEF_ATTACK;	//	It gets bashed in create_n_segment_path
				else
					ailp->mode = AIM_SNIPE_FIRE;	//	It gets bashed in create_n_segment_path
			} else {
				ailp->mode = AIM_RUN_FROM_OBJECT;	//	It gets bashed in create_n_segment_path
			}
		} else if (robptr->companion == 0) {
			ailp->mode = AIM_STILL;
			aip->path_length = 0;
			return;
		}
	}

	//--Int3_if (( (aip->PATH_DIR == -1) || (aip->PATH_DIR == 1));
	//--Int3_if (( (aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length));

	goal_point = gameData.ai.pointSegs [aip->hide_index + aip->cur_path_index].point;
	goal_seg = gameData.ai.pointSegs [aip->hide_index + aip->cur_path_index].segnum;
	dist_to_goal = VmVecDistQuick (&goal_point, &objP->pos);

	if (gameStates.app.bPlayerIsDead)
		dist_to_player = VmVecDistQuick (&objP->pos, &gameData.objs.viewer->pos);
	else
		dist_to_player = VmVecDistQuick (&objP->pos, &gameData.objs.console->pos);

	//	Efficiency hack: If far away from player, move in big quantized jumps.
	if (!(player_visibility || previous_visibility) && (dist_to_player > F1_0*200) && !(gameData.app.nGameMode & GM_MULTI)) {
		if (dist_to_goal < F1_0*2) {
			move_object_to_goal (objP, &goal_point, goal_seg);
			return;
		} else {
			robot_info	*robptr = &gameData.bots.pInfo [objP->id];
			fix	cur_speed = robptr->max_speed [gameStates.app.nDifficultyLevel]/2;
			fix	distance_travellable = FixMul (gameData.time.xFrame, cur_speed);

			// int	connect_side = FindConnectedSide (objP->segnum, goal_seg);
			//	Only move to goal if allowed to fly through the side.
			//	Buddy-bot can create paths he can't fly, waiting for player.
			// -- bah, this isn't good enough, buddy will fail to get through any door!if (WALL_IS_DOORWAY (&gameData.segs.segments]objP->segnum], connect_side) & WID_FLY_FLAG) {
			if (!gameData.bots.pInfo [objP->id].companion && !gameData.bots.pInfo [objP->id].thief) {
				if (distance_travellable >= dist_to_goal) {
					move_object_to_goal (objP, &goal_point, goal_seg);
				} else {
					fix	prob = FixDiv (distance_travellable, dist_to_goal);
	
					int	rand_num = d_rand ();
					if ((rand_num >> 1) < prob) {
						move_object_to_goal (objP, &goal_point, goal_seg);
					}
				}
				return;
			}
		}

	}

	//	If running from player, only run until can't be seen.
	if (ailp->mode == AIM_RUN_FROM_OBJECT) {
		if ((player_visibility == 0) && (ailp->player_awareness_type == 0)) {
			fix	vel_scale;

			vel_scale = F1_0 - gameData.time.xFrame/2;
			if (vel_scale < F1_0/2)
				vel_scale = F1_0/2;

			VmVecScale (&objP->mtype.phys_info.velocity, vel_scale);

			return;
		} else if (!(gameData.app.nFrameCount ^ ((OBJ_IDX (objP)) & 0x07))) {		//	Done 1/8 frames.
			//	If player on path (beyond point robot is now at), then create a new path.
			point_seg	*curpsp = &gameData.ai.pointSegs [aip->hide_index];
			short			player_segnum = gameData.objs.console->segnum;
			int			i;

			//	This is probably being done every frame, which is wasteful.
			for (i=aip->cur_path_index; i<aip->path_length; i++) {
				if (curpsp [i].segnum == player_segnum) {
					if (player_segnum != objP->segnum) {
						create_n_segment_path (objP, AVOID_SEG_LENGTH, player_segnum);
					} else {
						create_n_segment_path (objP, AVOID_SEG_LENGTH, -1);
					}
					Assert (aip->path_length != 0);
					ailp->mode = AIM_RUN_FROM_OBJECT;	//	It gets bashed in create_n_segment_path
					break;
				}
			}
			if (player_visibility) {
				ailp->player_awareness_type = 1;
				ailp->player_awareness_time = F1_0;
			}
		}
	}

	//--Int3_if (( (aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length));

	if (aip->cur_path_index < 0) {
		aip->cur_path_index = 0;
	} else if (aip->cur_path_index >= aip->path_length) {
		if (ailp->mode == AIM_RUN_FROM_OBJECT) {
			create_n_segment_path (objP, AVOID_SEG_LENGTH, gameData.objs.console->segnum);
			ailp->mode = AIM_RUN_FROM_OBJECT;	//	It gets bashed in create_n_segment_path
			Assert (aip->path_length != 0);
		} else {
			aip->cur_path_index = aip->path_length-1;
		}
	}

	goal_point = gameData.ai.pointSegs [aip->hide_index + aip->cur_path_index].point;

	//	If near goal, pick another goal point.
	forced_break = 0;		//	Gets set for short paths.
	original_dir = aip->PATH_DIR;
	original_index = aip->cur_path_index;
	threshold_distance = FixMul (VmVecMagQuick (&objP->mtype.phys_info.velocity), gameData.time.xFrame)*2 + F1_0*2;

	new_goal_point = gameData.ai.pointSegs [aip->hide_index + aip->cur_path_index].point;

	//--Int3_if (( (aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length));

	while ((dist_to_goal < threshold_distance) && !forced_break) {

		//	Advance to next point on path.
		aip->cur_path_index += aip->PATH_DIR;

		//	See if next point wraps past end of path (in either direction), and if so, deal with it based on mode.
		if ((aip->cur_path_index >= aip->path_length) || (aip->cur_path_index < 0)) {

			//	If mode = hiding, then stay here until get bonked or hit by player.
			// --	if (ailp->mode == AIM_BEHIND) {
			// --		ailp->mode = AIM_STILL;
			// --		return;		// Stay here until bonked or hit by player.
			// --	} else

			//	Buddy bot.  If he's in mode to get away from player and at end of line, 
			//	if player visible, then make a new path, else just return.
			if (robptr->companion) {
				if (gameData.escort.nSpecialGoal == ESCORT_GOAL_SCRAM)
				{
					if (player_visibility) {
						create_n_segment_path (objP, 16 + d_rand () * 16, -1);
						aip->path_length = polish_path (objP, &gameData.ai.pointSegs [aip->hide_index], aip->path_length);
						Assert (aip->path_length != 0);
						ailp->mode = AIM_WANDER;	//	Special buddy mode.
						//--Int3_if (( (aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length));
						return;
					} else {
						ailp->mode = AIM_WANDER;	//	Special buddy mode.
						VmVecZero (&objP->mtype.phys_info.velocity);
						VmVecZero (&objP->mtype.phys_info.rotvel);
						//!!Assert ((aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length);
						return;
					}
				}
			}

			if (aip->behavior == AIB_FOLLOW) {
				create_n_segment_path (objP, 10, gameData.objs.console->segnum);
				//--Int3_if (( (aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length));
			} else if (aip->behavior == AIB_STATION) {
				create_path_to_station (objP, 15);
				if ((aip->hide_segment != gameData.ai.pointSegs [aip->hide_index+aip->path_length-1].segnum) || (aip->path_length == 0)) {
					ailp->mode = AIM_STILL;
				} else {
					//--Int3_if (( (aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length));
				}
				return;
			} else if (ailp->mode == AIM_FOLLOW_PATH) {
				CreatePathToPlayer (objP, 10, 1);
				if (aip->hide_segment != gameData.ai.pointSegs [aip->hide_index+aip->path_length-1].segnum) {
					ailp->mode = AIM_STILL;
					return;
				} else {
					//--Int3_if (( (aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length));
				}
			} else if (ailp->mode == AIM_RUN_FROM_OBJECT) {
				create_n_segment_path (objP, AVOID_SEG_LENGTH, gameData.objs.console->segnum);
				ailp->mode = AIM_RUN_FROM_OBJECT;	//	It gets bashed in create_n_segment_path
				if (aip->path_length < 1) {
					create_n_segment_path (objP, AVOID_SEG_LENGTH, gameData.objs.console->segnum);
					ailp->mode = AIM_RUN_FROM_OBJECT;	//	It gets bashed in create_n_segment_path
					if (aip->path_length < 1) {
						aip->behavior = AIB_NORMAL;
						ailp->mode = AIM_STILL;
						return;
					}
				}
				//--Int3_if (( (aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length));
			} else {
				//	Reached end of the line.  First see if opposite end point is reachable, and if so, go there.
				//	If not, turn around.
				int			opposite_end_index;
				vms_vector	*opposite_end_point;
				fvi_info		hit_data;
				int			fate;
				fvi_query	fq;

				// See which end we're nearer and look at the opposite end point.
				if (abs (aip->cur_path_index - aip->path_length) < aip->cur_path_index) {
					//	Nearer to far end (ie, index not 0), so try to reach 0.
					opposite_end_index = 0;
				} else {
					//	Nearer to 0 end, so try to reach far end.
					opposite_end_index = aip->path_length-1;
				}

				//--Int3_if (( (opposite_end_index >= 0) && (opposite_end_index < aip->path_length));

				opposite_end_point = &gameData.ai.pointSegs [aip->hide_index + opposite_end_index].point;

				fq.p0						= &objP->pos;
				fq.startSeg				= objP->segnum;
				fq.p1						= opposite_end_point;
				fq.rad					= objP->size;
				fq.thisObjNum			= OBJ_IDX (objP);
				fq.ignoreObjList	= NULL;
				fq.flags					= 0; 				//what about trans walls???

				fate = FindVectorIntersection (&fq, &hit_data);

				if (fate != HIT_WALL) {
					//	We can be circular! Do it!
					//	Path direction is unchanged.
					aip->cur_path_index = opposite_end_index;
				} else {
					aip->PATH_DIR = -aip->PATH_DIR;
					aip->cur_path_index += aip->PATH_DIR;
				}
				//--Int3_if (( (aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length));
			}
			break;
		} else {
			new_goal_point = gameData.ai.pointSegs [aip->hide_index + aip->cur_path_index].point;
			goal_point = new_goal_point;
			dist_to_goal = VmVecDistQuick (&goal_point, &objP->pos);
			//--Int3_if (( (aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length));
		}

		//	If went all the way around to original point, in same direction, then get out of here!
		if ((aip->cur_path_index == original_index) && (aip->PATH_DIR == original_dir)) {
			CreatePathToPlayer (objP, 3, 1);
			//--Int3_if (( (aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length));
			forced_break = 1;
		}
		//--Int3_if (( (aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length));
	}	//	end while

	//	Set velocity (objP->mtype.phys_info.velocity) and orientation (objP->orient) for this object.
	//--Int3_if (( (aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length));
	ai_path_set_orient_and_vel (objP, &goal_point, player_visibility, vec_to_player);
	//--Int3_if (( (aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length));

}

typedef struct {
	short	path_start, objnum;
} obj_path;

int _CDECL_ path_index_compare (obj_path *i1, obj_path *i2)
{
	if (i1->path_start < i2->path_start)
		return -1;
	else if (i1->path_start == i2->path_start)
		return 0;
	else
		return 1;
}

//	----------------------------------------------------------------------------------------------------------
//	Set orientation matrix and velocity for objP based on its desire to get to a point.
void ai_path_set_orient_and_vel (object *objP, vms_vector *goal_point, int player_visibility, vms_vector *vec_to_player)
{
	vms_vector	cur_vel = objP->mtype.phys_info.velocity;
	vms_vector	norm_cur_vel;
	vms_vector	norm_vec_to_goal;
	vms_vector	cur_pos = objP->pos;
	vms_vector	norm_fvec;
	fix			speed_scale;
	fix			dot;
	robot_info	*robptr = &gameData.bots.pInfo [objP->id];
	fix			max_speed;

	//	If evading player, use highest difficulty level speed, plus something based on diff level
	max_speed = robptr->max_speed [gameStates.app.nDifficultyLevel];
	if ((gameData.ai.localInfo [OBJ_IDX (objP)].mode == AIM_RUN_FROM_OBJECT) || (objP->ctype.ai_info.behavior == AIB_SNIPE))
		max_speed = max_speed*3/2;

	VmVecSub (&norm_vec_to_goal, goal_point, &cur_pos);
	VmVecNormalizeQuick (&norm_vec_to_goal);

	norm_cur_vel = cur_vel;
	VmVecNormalizeQuick (&norm_cur_vel);

	norm_fvec = objP->orient.fvec;
	VmVecNormalizeQuick (&norm_fvec);

	dot = VmVecDot (&norm_vec_to_goal, &norm_fvec);

	//	If very close to facing opposite desired vector, perturb vector
	if (dot < -15*F1_0/16) {
		norm_cur_vel = norm_vec_to_goal;
	} else {
		norm_cur_vel.x += norm_vec_to_goal.x/2;
		norm_cur_vel.y += norm_vec_to_goal.y/2;
		norm_cur_vel.z += norm_vec_to_goal.z/2;
	}

	VmVecNormalizeQuick (&norm_cur_vel);

	//	Set speed based on this robot type's maximum allowed speed and how hard it is turning.
	//	How hard it is turning is based on the dot product of (vector to goal) and (current velocity vector)
	//	Note that since 3*F1_0/4 is added to dot product, it is possible for the robot to back up.

	//	Set speed and orientation.
	if (dot < 0)
		dot /= -4;

	//	If in snipe mode, can move fast even if not facing that direction.
	if (objP->ctype.ai_info.behavior == AIB_SNIPE)
		if (dot < F1_0/2)
			dot = (dot + F1_0)/2;

	speed_scale = FixMul (max_speed, dot);
	VmVecScale (&norm_cur_vel, speed_scale);
	objP->mtype.phys_info.velocity = norm_cur_vel;

	if ((gameData.ai.localInfo [OBJ_IDX (objP)].mode == AIM_RUN_FROM_OBJECT) || (robptr->companion == 1) || (objP->ctype.ai_info.behavior == AIB_SNIPE)) {
		if (gameData.ai.localInfo [OBJ_IDX (objP)].mode == AIM_SNIPE_RETREAT_BACKWARDS) {
			if ((player_visibility) && (vec_to_player != NULL))
				norm_vec_to_goal = *vec_to_player;
			else
				VmVecNegate (&norm_vec_to_goal);
		}
		ai_turn_towards_vector (&norm_vec_to_goal, objP, robptr->turn_time [NDL-1]/2);
	} else
		ai_turn_towards_vector (&norm_vec_to_goal, objP, robptr->turn_time [gameStates.app.nDifficultyLevel]);

}

int	Last_frame_garbage_collected = 0;

//	----------------------------------------------------------------------------------------------------------
//	Garbage colledion -- Free all unused records in gameData.ai.pointSegs and compress all paths.
void ai_path_garbage_collect (void)
{
	int	free_path_index = 0;
	int	num_path_objects = 0;
	int	objnum;
	int	objind;
	obj_path		object_list [MAX_OBJECTS];

#ifndef NDEBUG
	force_dump_ai_objects_all ("***** Start ai_path_garbage_collect *****");
#endif

	Last_frame_garbage_collected = gameData.app.nFrameCount;

#if PATH_VALIDATION
	validate_all_paths ();
#endif
	//	Create a list of gameData.objs.objects which have paths of length 1 or more.
	for (objnum=0; objnum <= gameData.objs.nLastObject; objnum++) {
		object	*objP = &gameData.objs.objects [objnum];

		if ((objP->type == OBJ_ROBOT) && ((objP->control_type == CT_AI) || (objP->control_type == CT_MORPH))) {
			ai_static	*aip = &objP->ctype.ai_info;

			if (aip->path_length) {
				object_list [num_path_objects].path_start = aip->hide_index;
				object_list [num_path_objects++].objnum = objnum;
			}
		}
	}

	qsort (object_list, num_path_objects, sizeof (object_list [0]), 
			 (int (_CDECL_ *) (void const *, void const *))path_index_compare);

	for (objind=0; objind < num_path_objects; objind++) {
		object		*objP;
		ai_static	*aip;
		int			i;
		int			old_index;

		objnum = object_list [objind].objnum;
		objP = &gameData.objs.objects [objnum];
		aip = &objP->ctype.ai_info;
		old_index = aip->hide_index;

		aip->hide_index = free_path_index;
		for (i=0; i<aip->path_length; i++)
			gameData.ai.pointSegs [free_path_index++] = gameData.ai.pointSegs [old_index++];
	}

	gameData.ai.freePointSegs = &gameData.ai.pointSegs [free_path_index];

////printf ("After garbage collection, d_free index = %i\n", gameData.ai.freePointSegs - gameData.ai.pointSegs);
#ifndef NDEBUG
	{
	int i;

	force_dump_ai_objects_all ("***** Finish ai_path_garbage_collect *****");

	for (i=0; i<=gameData.objs.nLastObject; i++) {
		ai_static	*aip = &gameData.objs.objects [i].ctype.ai_info;

		if ((gameData.objs.objects [i].type == OBJ_ROBOT) && (gameData.objs.objects [i].control_type == CT_AI))
			if ((aip->hide_index + aip->path_length > gameData.ai.freePointSegs - gameData.ai.pointSegs) && (aip->path_length>0))
				Int3 ();		//	Contact Mike: Debug trap for nasty, elusive bug.
	}

	validate_all_paths ();
	}
#endif

}

//	-----------------------------------------------------------------------------
//	Do garbage collection if not been done for awhile, or things getting really critical.
void maybe_ai_path_garbage_collect (void)
{
	if (gameData.ai.freePointSegs - gameData.ai.pointSegs > MAX_POINT_SEGS - MAX_PATH_LENGTH) {
		if (Last_frame_garbage_collected+1 >= gameData.app.nFrameCount) {
			//	This is kind of bad.  Garbage collected last frame or this frame.
			//	Just destroy all paths.  Too bad for the robots.  They are memory wasteful.
			AIResetAllPaths ();
#if TRACE
			con_printf (1, "Warning: Resetting all paths.  gameData.ai.pointSegs buffer nearly exhausted.\n");
#endif
		} else {
			//	We are really close to full, but didn't just garbage collect, so maybe this is recoverable.
#if TRACE
			con_printf (1, "Warning: Almost full garbage collection being performed: ");
#endif
			ai_path_garbage_collect ();
#if TRACE
			con_printf (1, "Free records = %i/%i\n", MAX_POINT_SEGS - (gameData.ai.freePointSegs - gameData.ai.pointSegs), MAX_POINT_SEGS);
#endif
		}
	} else if (gameData.ai.freePointSegs - gameData.ai.pointSegs > 3*MAX_POINT_SEGS/4) {
		if (Last_frame_garbage_collected + 16 < gameData.app.nFrameCount) {
			ai_path_garbage_collect ();
		}
	} else if (gameData.ai.freePointSegs - gameData.ai.pointSegs > MAX_POINT_SEGS/2) {
		if (Last_frame_garbage_collected + 256 < gameData.app.nFrameCount) {
			ai_path_garbage_collect ();
		}
	}
}

//	-----------------------------------------------------------------------------
//	Reset all paths.  Do garbage collection.
//	Should be called at the start of each level.
void AIResetAllPaths (void)
{
	int	i;

	for (i=0; i<=gameData.objs.nLastObject; i++)
		if (gameData.objs.objects [i].control_type == CT_AI) {
			gameData.objs.objects [i].ctype.ai_info.hide_index = -1;
			gameData.objs.objects [i].ctype.ai_info.path_length = 0;
		}

	ai_path_garbage_collect ();

}

//	---------------------------------------------------------------------------------------------------------
//	Probably called because a robot bashed a wall, getting a bunch of retries.
//	Try to resume path.
void attempt_to_resume_path (object *objP)
{
	//int				objnum = OBJ_IDX (objP);
	ai_static		*aip = &objP->ctype.ai_info;
//	int				goal_segnum, object_segnum, 
	int				abs_index, new_path_index;

	if ((aip->behavior == AIB_STATION) && (gameData.bots.pInfo [objP->id].companion != 1))
		if (d_rand () > 8192) {
			ai_local			*ailp = &gameData.ai.localInfo [OBJ_IDX (objP)];

			aip->hide_segment = objP->segnum;
//Int3 ();
			ailp->mode = AIM_STILL;
#if TRACE
			con_printf (1, "Note: Bashing hide segment of robot %i to current segment because he's lost.\n", OBJ_IDX (objP));
#endif
		}

//	object_segnum = objP->segnum;
	abs_index = aip->hide_index+aip->cur_path_index;
//	goal_segnum = gameData.ai.pointSegs [abs_index].segnum;

	new_path_index = aip->cur_path_index - aip->PATH_DIR;

	if ((new_path_index >= 0) && (new_path_index < aip->path_length)) {
		aip->cur_path_index = new_path_index;
	} else {
		//	At end of line and have nowhere to go.
		move_towards_segment_center (objP);
		create_path_to_station (objP, 15);
	}

}

//	----------------------------------------------------------------------------------------------------------
//					DEBUG FUNCTIONS FOLLOW
//	----------------------------------------------------------------------------------------------------------

#ifdef EDITOR
int	Test_size = 1000;

void test_create_path_many (void)
{
	point_seg	point_segs [200];
	short			num_points;

	int			i;

	for (i=0; i<Test_size; i++) {
		Cursegp = &gameData.segs.segments [ (d_rand () * (gameData.segs.nLastSegment + 1)) / D_RAND_MAX];
		Markedsegp = &gameData.segs.segments [ (d_rand () * (gameData.segs.nLastSegment + 1)) / D_RAND_MAX];
		CreatePathPoints (&gameData.objs.objects [0], CurSEG_IDX (segp), MarkedSEG_IDX (segp), point_segs, &num_points, -1, 0, 0, -1);
	}

}

void test_create_path (void)
{
	point_seg	point_segs [200];
	short			num_points;

	CreatePathPoints (&gameData.objs.objects [0], CurSEG_IDX (segp), MarkedSEG_IDX (segp), point_segs, &num_points, -1, 0, 0, -1);

}

void show_path (int start_seg, int end_seg, point_seg *psp, short length)
{
	//printf (" [%3i:%3i (%3i):] ", start_seg, end_seg, length);

	while (length--)
		//printf ("%3i ", psp [length].segnum);

	//printf ("\n");
}

//	For all segments in mine, create paths to all segments in mine, print results.
void test_create_all_paths (void)
{
	int	start_seg, end_seg;
	short	resultant_length;

	gameData.ai.freePointSegs = gameData.ai.pointSegs;

	for (start_seg=0; start_seg<=gameData.segs.nLastSegment-1; start_seg++) {
		if (gameData.segs.segments [start_seg].segnum != -1) {
			for (end_seg=start_seg+1; end_seg<=gameData.segs.nLastSegment; end_seg++) {
				if (gameData.segs.segments [end_seg].segnum != -1) {
					CreatePathPoints (&gameData.objs.objects [0], start_seg, end_seg, gameData.ai.freePointSegs, &resultant_length, -1, 0, 0, -1);
					show_path (start_seg, end_seg, gameData.ai.freePointSegs, resultant_length);
				}
			}
		}
	}
}

//--anchor--int	Num_anchors;
//--anchor--int	Anchor_distance = 3;
//--anchor--int	End_distance = 1;
//--anchor--int	Anchors [MAX_SEGMENTS];

//--anchor--int get_nearest_anchor_distance (int segnum)
//--anchor--{
//--anchor--	short	resultant_length, minimum_length;
//--anchor--	int	anchor_index;
//--anchor--
//--anchor--	minimum_length = 16383;
//--anchor--
//--anchor--	for (anchor_index=0; anchor_index<Num_anchors; anchor_index++) {
//--anchor--		CreatePathPoints (&gameData.objs.objects [0], segnum, Anchors [anchor_index], gameData.ai.freePointSegs, &resultant_length, -1, 0, 0, -1);
//--anchor--		if (resultant_length != 0)
//--anchor--			if (resultant_length < minimum_length)
//--anchor--				minimum_length = resultant_length;
//--anchor--	}
//--anchor--
//--anchor--	return minimum_length;
//--anchor--
//--anchor--}
//--anchor--
//--anchor--void create_new_anchor (int segnum)
//--anchor--{
//--anchor--	Anchors [Num_anchors++] = segnum;
//--anchor--}
//--anchor--
//--anchor--//	A set of anchors is within N units of all segments in the graph.
//--anchor--//	Anchor_distance = how close anchors can be.
//--anchor--//	End_distance = how close you can be to the end.
//--anchor--void test_create_all_anchors (void)
//--anchor--{
//--anchor--	int	nearest_anchor_distance;
//--anchor--	int	segnum, i;
//--anchor--
//--anchor--	Num_anchors = 0;
//--anchor--
//--anchor--	for (segnum=0; segnum<=gameData.segs.nLastSegment; segnum++) {
//--anchor--		if (gameData.segs.segments [segnum].segnum != -1) {
//--anchor--			nearest_anchor_distance = get_nearest_anchor_distance (segnum);
//--anchor--			if (nearest_anchor_distance > Anchor_distance)
//--anchor--				create_new_anchor (segnum);
//--anchor--		}
//--anchor--	}
//--anchor--
//--anchor--	//	Set selected segs.
//--anchor--	for (i=0; i<Num_anchors; i++)
//--anchor--		Selected_segs [i] = Anchors [i];
//--anchor--	N_selected_segs = Num_anchors;
//--anchor--
//--anchor--}
//--anchor--
//--anchor--int	Test_path_length = 5;
//--anchor--
//--anchor--void test_create_n_segment_path (void)
//--anchor--{
//--anchor--	point_seg	point_segs [200];
//--anchor--	short			num_points;
//--anchor--
//--anchor--	CreatePathPoints (&gameData.objs.objects [0], CurSEG_IDX (segp), -2, point_segs, &num_points, Test_path_length, 0, 0, -1);
//--anchor--}

short	Player_path_length=0;
int	Player_hide_index=-1;
int	Player_cur_path_index=0;
int	Player_following_path_flag=0;

//	------------------------------------------------------------------------------------------------------------------
//	Set orientation matrix and velocity for objP based on its desire to get to a point.
void player_path_set_orient_and_vel (object *objP, vms_vector *goal_point)
{
	vms_vector	cur_vel = objP->mtype.phys_info.velocity;
	vms_vector	norm_cur_vel;
	vms_vector	norm_vec_to_goal;
	vms_vector	cur_pos = objP->pos;
	vms_vector	norm_fvec;
	fix			speed_scale;
	fix			dot;
	fix			max_speed;

	max_speed = gameData.bots.pInfo [objP->id].max_speed [gameStates.app.nDifficultyLevel];

	VmVecSub (&norm_vec_to_goal, goal_point, &cur_pos);
	VmVecNormalizeQuick (&norm_vec_to_goal);

	norm_cur_vel = cur_vel;
	VmVecNormalizeQuick (&norm_cur_vel);

	norm_fvec = objP->orient.fvec;
	VmVecNormalizeQuick (&norm_fvec);

	dot = VmVecDot (&norm_vec_to_goal, &norm_fvec);
	if (gameData.ai.localInfo [OBJ_IDX (objP)].mode == AIM_SNIPE_RETREAT_BACKWARDS) {
		dot = -dot;
	}

	//	If very close to facing opposite desired vector, perturb vector
	if (dot < -15*F1_0/16) {
		norm_cur_vel = norm_vec_to_goal;
	} else {
		norm_cur_vel.x += norm_vec_to_goal.x/2;
		norm_cur_vel.y += norm_vec_to_goal.y/2;
		norm_cur_vel.z += norm_vec_to_goal.z/2;
	}

	VmVecNormalizeQuick (&norm_cur_vel);

	//	Set speed based on this robot type's maximum allowed speed and how hard it is turning.
	//	How hard it is turning is based on the dot product of (vector to goal) and (current velocity vector)
	//	Note that since 3*F1_0/4 is added to dot product, it is possible for the robot to back up.

	//	Set speed and orientation.
	if (dot < 0)
		dot /= 4;

	speed_scale = FixMul (max_speed, dot);
	VmVecScale (&norm_cur_vel, speed_scale);
	objP->mtype.phys_info.velocity = norm_cur_vel;
	ai_turn_towards_vector (&norm_vec_to_goal, objP, F1_0);

}

//	----------------------------------------------------------------------------------------------------------
//	Optimization: If current velocity will take robot near goal, don't change velocity
void player_follow_path (object *objP)
{
	vms_vector	goal_point;
	fix			dist_to_goal;
	int			count, forced_break, original_index;
	int			goal_seg;
	fix			threshold_distance;

	if (!Player_following_path_flag)
		return;

	if (Player_hide_index == -1)
		return;

	if (Player_path_length < 2)
		return;

	goal_point = gameData.ai.pointSegs [Player_hide_index + Player_cur_path_index].point;
	goal_seg = gameData.ai.pointSegs [Player_hide_index + Player_cur_path_index].segnum;
	Assert ((goal_seg >= 0) && (goal_seg <= gameData.segs.nLastSegment);
	dist_to_goal = VmVecDistQuick (&goal_point, &objP->pos);

	if (Player_cur_path_index < 0)
		Player_cur_path_index = 0;
	else if (Player_cur_path_index >= Player_path_length)
		Player_cur_path_index = Player_path_length-1;

	goal_point = gameData.ai.pointSegs [Player_hide_index + Player_cur_path_index].point;

	count=0;

	//	If near goal, pick another goal point.
	forced_break = 0;		//	Gets set for short paths.
	//original_dir = 1;
	original_index = Player_cur_path_index;
	threshold_distance = FixMul (VmVecMagQuick (&objP->mtype.phys_info.velocity), gameData.time.xFrame)*2 + F1_0*2;

	while ((dist_to_goal < threshold_distance) && !forced_break) {

		//	----- Debug stuff -----
		if (count++ > 20) {
#if TRACE
			con_printf (1, "Problem following path for player.  Aborting.\n");
#endif
			break;
		}

		//	Advance to next point on path.
		Player_cur_path_index += 1;

		//	See if next point wraps past end of path (in either direction), and if so, deal with it based on mode.
		if ((Player_cur_path_index >= Player_path_length) || (Player_cur_path_index < 0)) {
			Player_following_path_flag = 0;
			forced_break = 1;
		}

		//	If went all the way around to original point, in same direction, then get out of here!
		if (Player_cur_path_index == original_index) {
#if TRACE
			con_printf (CON_DEBUG, "Forcing break because player path wrapped, count = %i.\n", count);
#endif
			Player_following_path_flag = 0;
			forced_break = 1;
		}

		goal_point = gameData.ai.pointSegs [Player_hide_index + Player_cur_path_index].point;
		dist_to_goal = VmVecDistQuick (&goal_point, &objP->pos);

	}	//	end while

	//	Set velocity (objP->mtype.phys_info.velocity) and orientation (objP->orient) for this object.
	player_path_set_orient_and_vel (objP, &goal_point);

}


//	------------------------------------------------------------------------------------------------------------------
//	Create path for player from current segment to goal segment.
void create_player_path_to_segment (int segnum)
{
	object		*objP = gameData.objs.console;

	Player_path_length=0;
	Player_hide_index=-1;
	Player_cur_path_index=0;
	Player_following_path_flag=0;

	if (CreatePathPoints (objP, objP->segnum, segnum, gameData.ai.freePointSegs, &Player_path_length, 100, 0, 0, -1) == -1) {
#if TRACE
		con_printf (CON_DEBUG, "Unable to form path of length %i for myself\n", 100);
#endif
		}

	Player_following_path_flag = 1;

	Player_hide_index = gameData.ai.freePointSegs - gameData.ai.pointSegs;
	Player_cur_path_index = 0;
	gameData.ai.freePointSegs += Player_path_length;
	if (gameData.ai.freePointSegs - gameData.ai.pointSegs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
		//Int3 ();	//	Contact Mike: This is curious, though not deadly. /eip++;g
		AIResetAllPaths ();
	}

}

int	Player_goal_segment = -1;

void check_create_player_path (void)
{
	if (Player_goal_segment != -1)
		create_player_path_to_segment (Player_goal_segment);

	Player_goal_segment = -1;
}

#endif

//	----------------------------------------------------------------------------------------------------------
//					DEBUG FUNCTIONS ENDED
//	----------------------------------------------------------------------------------------------------------

