/* $Id: fvi.c, v 1.3 2003/10/10 09:36:35 btb Exp $ */
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
 * New home for FindVectorIntersection()
 *
 * Old Log:
 * Revision 1.7  1995/10/21  23:52:18  allender
 * #ifdef'ed out stack debug stuff
 *
 * Revision 1.6  1995/10/10  12:07:42  allender
 * add forgotten ;
 *
 * Revision 1.5  1995/10/10  11:47:27  allender
 * put in stack space check
 *
 * Revision 1.4  1995/08/23  21:34:08  allender
 * fix mcc compiler warning
 *
 * Revision 1.3  1995/08/14  14:35:18  allender
 * changed transparency to 0
 *
 * Revision 1.2  1995/07/05  16:50:51  allender
 * transparency/kitchen change
 *
 * Revision 1.1  1995/05/16  15:24:59  allender
 * Initial revision
 *
 * Revision 2.3  1995/03/24  14:49:04  john
 * Added cheat for player to go thru walls.
 *
 * Revision 2.2  1995/03/21  17:58:32  john
 * Fixed bug with normals..
 *
 *
 * Revision 2.1  1995/03/20  18:15:37  john
 * Added code to not store the normals in the segment structure.
 *
 * Revision 2.0  1995/02/27  11:27:41  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.49  1995/02/22  14:45:47  allender
 * remove anonymous unions from object structure
 *
 * Revision 1.48  1995/02/22  13:24:50  john
 * Removed the vecmat anonymous unions.
 *
 * Revision 1.47  1995/02/07  16:17:26  matt
 * Disabled all robot-robot collisions except those involving two green
 * guys.  Used to do collisions if either robot was green guy.
 *
 * Revision 1.46  1995/02/02  14:07:53  matt
 * Fixed confusion about which segment you are touching when you're
 * touching a wall.  This manifested itself in spurious lava burns.
 *
 * Revision 1.45  1995/02/02  13:45:53  matt
 * Made a bunch of lint-inspired changes
 *
 * Revision 1.44  1995/01/24  12:10:17  matt
 * Fudged collisions for player/player, and player weapon/other player in
 * coop games.
 *
 * Revision 1.43  1995/01/14  19:16:45  john
 * First version of new bitmap paging code.
 *
 * Revision 1.42  1994/12/15  12:22:40  matt
 * Small change which may or may not help
 *
 * Revision 1.41  1994/12/14  11:45:51  matt
 * Fixed (hopefully) little bug with invalid segnum
 *
 * Revision 1.40  1994/12/13  17:12:01  matt
 * Increased edge tolerance a bunch more
 *
 * Revision 1.39  1994/12/13  14:37:59  matt
 * Fixed another stupid little bug
 *
 * Revision 1.38  1994/12/13  13:25:44  matt
 * Increased tolerance massively to avoid catching on corners
 *
 * Revision 1.37  1994/12/13  12:02:20  matt
 * Fixed small bug
 *
 * Revision 1.36  1994/12/13  11:17:35  matt
 * Lots of changes to hopefully fix gameData.objs.objects leaving the mine.  Note that
 * this code should be considered somewhat experimental - one problem I
 * know about is that you can get stuck on edges more easily than before.
 * There may be other problems I don't know about yet.
 *
 * Revision 1.35  1994/12/12  01:20:57  matt
 * Added hack in object-object collisions that treats claw guys as
 * if they have 3/4 of their actual radius.
 *
 * Revision 1.34  1994/12/04  22:48:39  matt
 * Physics & FVI now only build segList for player gameData.objs.objects, and they
 * responsilby deal with buffer full conditions
 *
 * Revision 1.33  1994/12/04  22:07:05  matt
 * Added better handing of buffer full condition
 *
 * Revision 1.32  1994/12/01  21:06:33  matt
 * Several important changes:
 *  (1) Checking against triangulated sides has been standardized a bit
 *  (2) Code has been added to de-triangulate some sides
 *  (3) BIG ONE: the tolerance for checking a point against a plane has
 *      been drastically relaxed
 *
 *
 * Revision 1.31  1994/11/27  23:15:03  matt
 * Made changes for new con_printf calling convention
 *
 * Revision 1.30  1994/11/19  15:20:30  mike
 * rip out unused code and data
 *
 * Revision 1.29  1994/11/16  12:18:17  mike
 * hack for green_guy:green_guy collision detection.
 *
 * Revision 1.28  1994/11/10  13:08:54  matt
 * Added support for new run-length-encoded bitmaps
 *
 * Revision 1.27  1994/10/31  12:27:51  matt
 * Added new function ObjectIntersectsWall()
 *
 * Revision 1.26  1994/10/20  13:59:27  matt
 * Added assert
 *
 * Revision 1.25  1994/10/09  23:51:09  matt
 * Made FindHitPointUV() work with triangulated sides
 *
 * Revision 1.24  1994/09/25  00:39:29  matt
 * Took out con_printf's
 *
 * Revision 1.23  1994/09/25  00:37:53  matt
 * Made the 'find the point in the bitmap where something hit' system
 * publicly accessible.
 *
 * Revision 1.22  1994/09/21  16:58:22  matt
 * Fixed bug in trans wall check that was checking against verically
 * flipped bitmap (i.e., the y coord was negative when checking).
 *
 * Revision 1.21  1994/09/02  11:31:40  matt
 * Fixed object/object collisions, so you can't fly through robots anymore.
 * Cleaned up object damage system.
 *
 * Revision 1.20  1994/08/26  09:42:03  matt
 * Increased the size of a buffer
 *
 * Revision 1.19  1994/08/11  18:57:53  mike
 * Convert shorts to ints for optimization.
 *
 * Revision 1.18  1994/08/08  21:38:24  matt
 * Put in small optimization
 *
 * Revision 1.17  1994/08/08  12:21:52  yuan
 * Fixed assert
 *
 * Revision 1.16  1994/08/08  11:47:04  matt
 * Cleaned up fvi and physics a little
 *
 * Revision 1.15  1994/08/04  00:21:04  matt
 * Cleaned up fvi & physics error handling; put in code to make sure gameData.objs.objects
 * are in correct segment; simplified segment finding for gameData.objs.objects and points
 *
 * Revision 1.14  1994/08/02  19:04:26  matt
 * Cleaned up vertex list functions
 *
 * Revision 1.13  1994/08/02  09:56:28  matt
 * Put in check for bad value FindPlaneLineIntersection()
 *
 * Revision 1.12  1994/08/01  17:27:26  matt
 * Added support for triangulated walls in trans point check
 *
 * Revision 1.11  1994/08/01  13:30:40  matt
 * Made fvi() check holes in transparent walls, and changed fvi() calling
 * parms to take all input data in query structure.
 *
 * Revision 1.10  1994/07/13  21:47:17  matt
 * FVI() and physics now keep lists of segments passed through which the
 * trigger code uses.
 *
 * Revision 1.9  1994/07/09  21:21:40  matt
 * Fixed, hopefull, bugs in sphere-to-vector intersection code
 *
 * Revision 1.8  1994/07/08  14:26:42  matt
 * Non-needed powerups don't get picked up now; this required changing FVI to
 * take a list of ingore gameData.objs.objects rather than just one ignore object.
 *
 * Revision 1.7  1994/07/06  20:02:37  matt
 * Made change to match gameseg that uses lowest point number as reference
 * point when checking against a plane
 *
 * Revision 1.6  1994/06/29  15:43:58  matt
 * When computing intersection of vector and sphere, use the radii of both
 * gameData.objs.objects.
 *
 * Revision 1.5  1994/06/14  15:57:58  matt
 * Took out asserts, and added other hacks, pending real bug fixes
 *
 * Revision 1.4  1994/06/13  23:10:08  matt
 * Fixed problems with triangulated sides
 *
 * Revision 1.3  1994/06/09  12:11:14  matt
 * Fixed confusing use of two variables, hit_objnum & fviHitData.nObject, to
 * keep the same information in different ways.
 *
 * Revision 1.2  1994/06/09  09:58:38  matt
 * Moved FindVectorIntersection() from physics.c to new file fvi.c
 *
 * Revision 1.1  1994/06/09  09:25:57  matt
 * Initial revision
 *
 *
 */

#define NEW_FVI_STUFF 1

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inferno.h"
#include "u_mem.h"
#include "error.h"
#include "mono.h"

#include "inferno.h"
#include "fvi.h"
#include "segment.h"
#include "object.h"
#include "wall.h"
#include "laser.h"
#include "rle.h"
#include "robot.h"
#include "piggy.h"
#include "player.h"
#include "gameseg.h"

#define face_type_num(nfaces, face_num, tri_edge) ((nfaces==1)?0:(tri_edge*2 + face_num))

#include "fvi_a.h"

//	-----------------------------------------------------------------------------
//find the point on the specified plane where the line intersects
//returns true if point found, false if line parallel to plane
//new_pnt is the found point on the plane
//plane_pnt & plane_norm describe the plane
//p0 & p1 are the ends of the line
int FindPlaneLineIntersection(vms_vector *new_pnt, vms_vector *plane_pnt, vms_vector *plane_norm, 
										vms_vector *p0, vms_vector *p1, fix rad)
{
	vms_vector d, w;
	fix num, den, k;

VmVecSub (&d, p1, p0);
VmVecSub (&w, p0, plane_pnt);
num =  VmVecDot (plane_norm, &w);
den = -VmVecDot (plane_norm, &d);
num -= rad;			//move point out by rad
if (!den)
	return 0;
else if (den > 0) {
	if ((num > den) || (-num >> 15 >= den)) //frac greater than one
		return 0;
	}
else {
	if (num < den)
		return 0;
	}
//do check for potenial overflow
if (labs (num) / (f1_0 / 2) >= labs (den))	
	return 0;
k = FixDiv (num, den);
Assert (k <= f1_0);		//should be trapped above
VmVecScaleFrac (&d, num, den);
VmVecAdd (new_pnt, p0, &d);
return 1;
}

//	-----------------------------------------------------------------------------

typedef struct vec2d {
	fix i, j;
} vec2d;

//given largest componant of normal, return i & j
//if largest componant is negative, swap i & j
int ij_table [3][2] =        {
							{2, 1},          //pos x biggest
							{0, 2},          //pos y biggest
							{1, 0},          //pos z biggest
						};

//intersection types
#define IT_NONE 0       //doesn't touch face at all
#define IT_FACE 1       //touches face
#define IT_EDGE 2       //touches edge of face
#define IT_POINT        3       //touches vertex

//	-----------------------------------------------------------------------------
//see if a point in inside a face by projecting into 2d
uint CheckPointToFace(vms_vector *checkp, side *s, int facenum, int nv, int *vertex_list)
{
	vms_vector_array *checkp_array;
	vms_vector_array norm;
	vms_vector t;
	int biggest;
///
	int i, j, edge;
	uint edgemask;
	fix check_i, check_j;
	vms_vector_array *v0, *v1;
	vec2d edgevec, checkvec;
	fix d;

#ifdef COMPACT_SEGS
GetSideNormal (sp, s-sp->sides, facenum, (vms_vector *)&norm);
#else
memcpy (&norm, &s->normals [facenum], sizeof(vms_vector_array));
#endif
checkp_array = (vms_vector_array *)checkp;

//now do 2d check to see if point is in side
//project polygon onto plane by finding largest component of normal
t.x = labs (norm.xyz [0]); 
t.y = labs (norm.xyz [1]); 
t.z = labs (norm.xyz [2]);

if (t.x > t.y) 
	if (t.x > t.z) 
		biggest=0; 
	else 
		biggest=2;
	else if (t.y > t.z) 
		biggest = 1; 
	else 
		biggest=2;
if (norm.xyz [biggest] > 0) {
	i = ij_table [biggest][0];
	j = ij_table [biggest][1];
	}
else {
	i = ij_table [biggest][1];
	j = ij_table [biggest][0];
	}
//now do the 2d problem in the i, j plane
check_i = checkp_array->xyz [i];
check_j = checkp_array->xyz [j];
for (edge = edgemask = 0; edge < nv; edge++) {
	v0 = (vms_vector_array *) &gameData.segs.vertices [vertex_list [facenum * 3 + edge]];
	v1 = (vms_vector_array *) &gameData.segs.vertices [vertex_list [facenum * 3 + ((edge + 1) % nv)]];
	edgevec.i = v1->xyz [i] - v0->xyz [i];
	edgevec.j = v1->xyz [j] - v0->xyz [j];
	checkvec.i = check_i - v0->xyz [i];
	checkvec.j = check_j - v0->xyz [j];
	d = FixMul (checkvec.i, edgevec.j) - FixMul (checkvec.j, edgevec.i);
	if (d < 0)              		//we are outside of triangle
		edgemask |= (1 << edge);
	}
return edgemask;
}

//	-----------------------------------------------------------------------------
//check if a sphere intersects a face
int CheckSphereToFace (vms_vector *pnt, side *s, int facenum, int nv, fix rad, int *vertex_list)
{
	vms_vector checkp = *pnt;
	vms_vector edgevec, checkvec;            //this time, real 3d vectors
	vms_vector closest_point;
	fix edgelen, d, dist;
	vms_vector *v0, *v1;
	int itype;
	int edgenum;
	uint edgemask;

//now do 2d check to see if point is in side
edgemask = CheckPointToFace (pnt, s, facenum, nv, vertex_list);
//we've gone through all the sides, are we inside?
if (edgemask == 0)
	return IT_FACE;
//get verts for edge we're behind
for (edgenum = 0; !(edgemask & 1); (edgemask >>= 1), edgenum++)
	;
v0 = &gameData.segs.vertices [vertex_list [facenum * 3 + edgenum]];
v1 = &gameData.segs.vertices [vertex_list [facenum * 3 + ((edgenum + 1) % nv)]];
//check if we are touching an edge or point
VmVecSub (&checkvec, &checkp, v0);
edgelen = VmVecNormalizedDir (&edgevec, v1, v0);
//find point dist from planes of ends of edge
d = VmVecDot (&edgevec, &checkvec);
if (d + rad < 0) 
	return IT_NONE;                  //too far behind start point
if (d - rad > edgelen) 
	return IT_NONE;    //too far part end point
//find closest point on edge to check point
itype = IT_POINT;
if (d < 0) 
	closest_point = *v0;
else if (d > edgelen) 
	closest_point = *v1;
else {
	itype = IT_EDGE;
	VmVecScaleAdd (&closest_point, v0, &edgevec, d);
	}
dist = VmVecDist (&checkp, &closest_point);
if (dist <= rad)
	return (itype == IT_POINT) ? IT_NONE : itype;
return IT_NONE;
}

//	-----------------------------------------------------------------------------
//returns true if line intersects with face. fills in newp with intersection
//point on plane, whether or not line intersects side
//facenum determines which of four possible faces we have
//note: the seg parm is temporary, until the face itself has a point field
int CheckLineToFace(vms_vector *newp, vms_vector *p0, vms_vector *p1, segment *segP, int side, int facenum, int nv, fix rad)
{
	vms_vector	checkp, vNormal, v1;
	struct side *sideP = segP->sides + side;
	int			vertexList [6];
	int			pli, nFaces, nVertex;

#ifdef COMPACT_SEGS
GetSideNormal (segP, side, facenum, &vNormal);
#else
vNormal = sideP->normals [facenum];
#endif
if ((SEG_IDX (segP))==-1)
	Error ("segnum == -1 in CheckLineToFace()");
CreateAbsVertexLists (&nFaces, vertexList, SEG_IDX (segP), side);
//use lowest point number
nVertex = vertexList [0];
if (nVertex > vertexList [2])
	nVertex = vertexList [2];
if (nFaces == 1) {
	if (nVertex > vertexList [1])
		nVertex = vertexList [1];
	if (nVertex > vertexList [3])
		nVertex = vertexList [3];
	}
#if 1
if (p1 == p0) {
	v1 = vNormal;
	VmVecNegate (&v1);
	VmVecScale (&v1, rad);
	VmVecInc (&v1, p0);
	rad = 0;
	p1 = &v1;
	}
#endif
pli = FindPlaneLineIntersection (newp, gameData.segs.vertices + nVertex, &vNormal, p0, p1, rad);
if (!pli) 
	return IT_NONE;
checkp = *newp;
//if rad != 0, project the point down onto the plane of the polygon
if (rad)
	VmVecScaleInc (&checkp, &vNormal, -rad);
return CheckSphereToFace (&checkp, sideP, facenum, nv, rad, vertexList);
}

//	-----------------------------------------------------------------------------
//returns the value of a determinant
fix CalcDetValue (vms_matrix *det)
{
return FixMul (det->rvec.x, FixMul (det->uvec.y, det->fvec.z)) -
		 FixMul (det->rvec.x, FixMul (det->uvec.z, det->fvec.y)) -
		 FixMul (det->rvec.y, FixMul (det->uvec.x, det->fvec.z)) +
		 FixMul (det->rvec.y, FixMul (det->uvec.z, det->fvec.x)) +
	 	 FixMul (det->rvec.z, FixMul (det->uvec.x, det->fvec.y)) -
		 FixMul (det->rvec.z, FixMul (det->uvec.y, det->fvec.x));
}

//	-----------------------------------------------------------------------------
//computes the parameters of closest approach of two lines
//fill in two parameters, t0 & t1.  returns 0 if lines are parallel, else 1
int CheckLineToLine(fix *t1, fix *t2, vms_vector *p1, vms_vector *v1, vms_vector *p2, vms_vector *v2)
{
	vms_matrix det;
	fix d, cross_mag2;		//mag squared Cross product

VmVecSub(&det.rvec, p2, p1);
VmVecCross(&det.fvec, v1, v2);
cross_mag2 = VmVecDot(&det.fvec, &det.fvec);
if (!cross_mag2)
	return 0;			//lines are parallel
det.uvec = *v2;
d = CalcDetValue(&det);
if (oflow_check(d, cross_mag2))
	return 0;
*t1 = FixDiv(d, cross_mag2);
det.uvec = *v1;
d = CalcDetValue(&det);
if (oflow_check(d, cross_mag2))
	return 0;
*t2 = FixDiv(d, cross_mag2);
return 1;		//found point
}

#ifdef NEW_FVI_STUFF
int bSimpleFVI=0;
#else
#define bSimpleFVI 1
#endif

//	-----------------------------------------------------------------------------
//this version is for when the start and end positions both poke through
//the plane of a side.  In this case, we must do checks against the edge
//of faces
int SpecialCheckLineToFace(vms_vector *newp, vms_vector *p0, vms_vector *p1, segment *segP, int side, int facenum, int nv, fix rad)
{
	vms_vector move_vec;
	fix edge_t, move_t, edge_t2, move_t2, closest_dist;
	fix edge_len, move_len;
	int vertex_list [6];
	int num_faces, edgenum;
	uint edgemask;
	vms_vector *edge_v0, *edge_v1, edge_vec;
	struct side *sideP=segP->sides + side;
	vms_vector closest_point_edge, closest_point_move;

if (bSimpleFVI)
	return CheckLineToFace (newp, p0, p1, segP, side, facenum, nv, rad);
//calc some basic stuff
if ((SEG_IDX (segP)) == -1)
	Error("segnum == -1 in SpecialCheckLineToFace()");
CreateAbsVertexLists (&num_faces, vertex_list, SEG_IDX (segP), side);
VmVecSub (&move_vec, p1, p0);
//figure out which edge(sideP) to check against
if (!(edgemask = CheckPointToFace (p0, sideP, facenum, nv, vertex_list)))
	return CheckLineToFace (newp, p0, p1, segP, side, facenum, nv, rad);
for (edgenum = 0; !(edgemask & 1); edgemask >>= 1, edgenum++)
	;
edge_v0 = gameData.segs.vertices + vertex_list [facenum * 3 + edgenum];
edge_v1 = gameData.segs.vertices + vertex_list [facenum * 3 + ((edgenum + 1) % nv)];
VmVecSub (&edge_vec, edge_v1, edge_v0);
//is the start point already touching the edge?
//first, find point of closest approach of vec & edge
edge_len = VmVecNormalize (&edge_vec);
move_len = VmVecNormalize (&move_vec);
CheckLineToLine (&edge_t, &move_t, edge_v0, &edge_vec, p0, &move_vec);
//make sure t values are in valid range
if ((move_t < 0) || (move_t > move_len + rad))
	return IT_NONE;
if (move_t > move_len)
	move_t2 = move_len;
else
	move_t2 = move_t;
if (edge_t < 0)		//clamp at points
	edge_t2 = 0;
else
	edge_t2 = edge_t;
if (edge_t2 > edge_len)		//clamp at points
	edge_t2 = edge_len;
//now, edge_t & move_t determine closest points.  calculate the points.
VmVecScaleAdd(&closest_point_edge, edge_v0, &edge_vec, edge_t2);
VmVecScaleAdd(&closest_point_move, p0, &move_vec, move_t2);
//find dist between closest points
closest_dist = VmVecDist(&closest_point_edge, &closest_point_move);
//could we hit with this dist?
//note massive tolerance here
if (closest_dist < (rad*15)/20) {		//we hit.  figure out where
	//now figure out where we hit
	VmVecScaleAdd(newp, p0, &move_vec, move_t-rad);
	return IT_EDGE;
	}
return IT_NONE;			//no hit
}

//	-----------------------------------------------------------------------------
//maybe this routine should just return the distance and let the caller
//decide it it's close enough to hit
//determine if and where a vector intersects with a sphere
//vector defined by p0, p1
//returns dist if intersects, and fills in intp
//else returns 0
int CheckVectorToSphere1(vms_vector *intp, vms_vector *p0, vms_vector *p1, vms_vector *sphere_pos, 
									  fix sphere_rad)
{
	vms_vector d, dn, w, closest_point;
	fix mag_d, dist, w_dist, int_dist;

//this routine could be optimized if it's taking too much time!

VmVecSub(&d, p1, p0);
VmVecSub(&w, sphere_pos, p0);
mag_d = VmVecCopyNormalize(&dn, &d);
if (mag_d == 0) {
	int_dist = VmVecMag(&w);
	*intp = *p0;
	return ((sphere_rad < 0) || (int_dist<sphere_rad))?int_dist:0;
	}
w_dist = VmVecDot(&dn, &w);
if (w_dist < 0)		//moving away from object
	return 0;
if (w_dist > mag_d+sphere_rad)
	return 0;		//cannot hit
VmVecScaleAdd(&closest_point, p0, &dn, w_dist);
dist = VmVecDist(&closest_point, sphere_pos);
if (dist < sphere_rad) {
	fix	dist2, radius2, nShorten;

	dist2 = FixMul(dist, dist);
	radius2 = FixMul(sphere_rad, sphere_rad);
	nShorten = fix_sqrt(radius2 - dist2);
	int_dist = w_dist-nShorten;
	if (int_dist > mag_d || int_dist < 0) {
		//past one or the other end of vector, which means we're inside
		*intp = *p0;		//don't move at all
		return 1;
		}
	VmVecScaleAdd(intp, p0, &dn, int_dist);         //calc intersection point
	return int_dist;
	}
return 0;
}

//	-----------------------------------------------------------------------------
//determine if a vector intersects with an object
//if no intersects, returns 0, else fills in intp and returns dist
fix CheckVectorToObject (vms_vector *intp, vms_vector *p0, vms_vector *p1, fix rad, 
								 object *objP, object *otherObjP)
{
	fix size;
	
if (rad < 0)
	size = 0;
else {
	size = objP->size;
	if (objP->type == OBJ_ROBOT && gameData.bots.pInfo [objP->id].attack_type)
		size = (size*3)/4;
	//if obj is player, and bumping into other player or a weapon of another coop player, reduce radius
	if (objP->type == OBJ_PLAYER &&
			((otherObjP->type == OBJ_PLAYER) ||
	 		((gameData.app.nGameMode&GM_MULTI_COOP) && otherObjP->type == OBJ_WEAPON && otherObjP->ctype.laser_info.parent_type == OBJ_PLAYER)))
		size = size/2;
	}
return CheckVectorToSphere1 (intp, p0, p1, &objP->pos, size+rad);
}


#define MAX_SEGS_VISITED 100
int nSegsVisited;
short segsVisited [MAX_SEGS_VISITED];

fvi_hit_info fviHitData;

//	-----------------------------------------------------------------------------

int FVICompute (vms_vector *intP, short *intS, vms_vector *p0, short nStartSeg, vms_vector *p1, 
				 fix rad, short nThisObject, short *ignoreObjList, int flags, short *segList, 
				 short *nSegments, int entrySegP);

//What the hell is fviHitData.nSegment for???

//Find out if a vector intersects with anything.
//Fills in hitData, an fvi_info structure (see header file).
//Parms:
//  p0 & startseg 	describe the start of the vector
//  p1 					the end of the vector
//  rad 					the radius of the cylinder
//  thisObjNum 		used to prevent an object with colliding with itself
//  ingore_obj			ignore collisions with this object
//  check_obj_flag	determines whether collisions with gameData.objs.objects are checked
//Returns the hitData->nHitType
int FindVectorIntersection (fvi_query *fq, fvi_info *hitData)
{
	int			nHitType, nNewHitType;
	short			nHitSegment, nHitSegment2;
	vms_vector	vHitPoint;
	int			i;
	segmasks		masks;

Assert(fq->ignoreObjList != (short *)(-1));
Assert((fq->startSeg <= gameData.segs.nLastSegment) && (fq->startSeg >= 0));

fviHitData.nSegment = -1;
fviHitData.nSide = -1;
fviHitData.nObject = -1;

//check to make sure start point is in seg its supposed to be in
//Assert(check_point_in_seg(p0, startseg, 0).centerMask==0);	//start point not in seg

// gameData.objs.viewer is not in segment as claimed, so say there is no hit.
masks = GetSegMasks (fq->p0, fq->startSeg, 0);
if (masks.centerMask) {
	hitData->hit.nType = HIT_BAD_P0;
	hitData->hit.vPoint = *fq->p0;
	hitData->hit.nSegment = fq->startSeg;
	hitData->hit.nSide = 0;
	hitData->hit.nObject = 0;
	hitData->hit.nSideSegment = -1;
	return hitData->hit.nType;
	}
segsVisited [0] = fq->startSeg;
nSegsVisited = 1;
fviHitData.nNestCount = 0;
nHitSegment2 = fviHitData.nSegment2 = -1;
nHitType = FVICompute (&vHitPoint, &nHitSegment2, fq->p0, (short) fq->startSeg, fq->p1, fq->rad, 
						  (short) fq->thisObjNum, fq->ignoreObjList, fq->flags, 
						  hitData->segList, &hitData->nSegments, -2);
//!!nHitSegment = FindSegByPoint(&vHitPoint, fq->startSeg);
if ((nHitSegment2 != -1) && !GetSegMasks (&vHitPoint, nHitSegment2, 0).centerMask)
	nHitSegment = nHitSegment2;
else {
	nHitSegment = FindSegByPoint (&vHitPoint, fq->startSeg);
	}
//MATT: TAKE OUT THIS HACK AND FIX THE BUGS!
if ((nHitType == HIT_WALL) && (nHitSegment == -1))
	if ((fviHitData.nSegment2 != -1) && !GetSegMasks (&vHitPoint, fviHitData.nSegment2, 0).centerMask)
		nHitSegment = fviHitData.nSegment2;

if (nHitSegment == -1) {
	//int nNewHitType;
	short nNewHitSeg2=-1;
	vms_vector vNewHitPoint;

	//because of code that deal with object with non-zero radius has
	//problems, try using zero radius and see if we hit a wall
	nNewHitType = FVICompute (&vNewHitPoint, &nNewHitSeg2, fq->p0, (short) fq->startSeg, fq->p1, 0, 
								  (short) fq->thisObjNum, fq->ignoreObjList, fq->flags, hitData->segList, 
								  &hitData->nSegments, -2);
	if (nNewHitSeg2 != -1) {
		nHitType = nNewHitType;
		nHitSegment = nNewHitSeg2;
		vHitPoint = vNewHitPoint;
		}
	}

if (nHitSegment!=-1 && fq->flags&FQ_GET_SEGLIST)
	if ((nHitSegment != hitData->segList [hitData->nSegments-1]) && 
		 (hitData->nSegments < MAX_FVI_SEGS - 1))
		hitData->segList [hitData->nSegments++] = nHitSegment;

if (nHitSegment!=-1 && fq->flags&FQ_GET_SEGLIST)
	for (i=0;i<hitData->nSegments && i<MAX_FVI_SEGS-1;i++)
		if (hitData->segList [i] == nHitSegment) {
			hitData->nSegments = i+1;
			break;
		}
Assert(!(nHitType==HIT_OBJECT && fviHitData.nObject==-1));
hitData->hit = fviHitData;
hitData->hit.nType		= nHitType;
hitData->hit.vPoint		= vHitPoint;
hitData->hit.nSegment	= nHitSegment;
return nHitType;
}


//	-----------------------------------------------------------------------------

int ObjectInList(short objnum, short *obj_list)
{
	short t;

	while ((t=*obj_list)!=-1 && t!=objnum) 
		obj_list++;

	return (t==objnum);

}

//	-----------------------------------------------------------------------------

int CheckTransWall (vms_vector *pnt, segment *seg, short sidenum, short facenum);

int FVICompute (vms_vector *vIntP, short *intS, vms_vector *p0, short nStartSeg, vms_vector *p1, 
				 fix rad, short nThisObject, short *ignoreObjList, int flags, short *segList, 
				 short *nSegments, int entrySegP)
{
	segment		*segP;				//the segment we're looking at
	int			startMask, endMask, centerMask;	//mask of faces
	//@@int sideMask;				//mask of sides - can be on back of face but not side
	short			objnum;
	segmasks		masks;
	vms_vector	vHitPoint, vClosestHitPoint; 	//where we hit
	fix			d, dMin = 0x7fffffff;					//distance to hit point
	int			nHitType = HIT_NONE;							//what sort of hit
	int			nHitSegment = -1;
	int			nHitNoneSegment = -1;
	int			nHitNoneSegs = 0;
	int			hitNoneSegList [MAX_FVI_SEGS];
	int			nCurNestLevel = fviHitData.nNestCount;
#if 1
	int			nFudgedRad;
	int			nThisType, nOtherType;
	object		*otherObjP,
					*thisObjP = (nThisObject < 0) ? NULL : gameData.objs.objects + nThisObject;
#endif

if (flags & FQ_GET_SEGLIST)
	*segList = nStartSeg;
*nSegments = 1;
segP = gameData.segs.segments + nStartSeg;
fviHitData.nNestCount++;
//first, see if vector hit any objects in this segment
#if 0
if (flags & FQ_CHECK_OBJS)
	for (objnum = segP->objects; objnum != -1; objnum = gameData.objs.objects [objnum].next)
		if (!(gameData.objs.objects [objnum].flags & OF_SHOULD_BE_DEAD) &&
				!(nThisObject == objnum) &&
				(ignoreObjList==NULL || !ObjectInList(objnum, ignoreObjList)) &&
				!LasersAreRelated (objnum, nThisObject) &&
				!((nThisObject > -1) &&
				(CollisionResult [gameData.objs.objects [nThisObject].type][gameData.objs.objects [objnum].type] == RESULT_NOTHING) &&
			 	(CollisionResult [gameData.objs.objects [objnum].type][gameData.objs.objects [nThisObject].type] == RESULT_NOTHING))) {
			int nFudgedRad = rad;

			//	If this is a powerup, don't do collision if flag FQ_IGNORE_POWERUPS is set
			if (gameData.objs.objects [objnum].type == OBJ_POWERUP)
				if (flags & FQ_IGNORE_POWERUPS)
					continue;
			//	If this is a robot:robot collision, only do it if both of them have attack_type != 0 (eg, green guy)
			if (gameData.objs.objects [nThisObject].type == OBJ_ROBOT) {
				if (gameData.objs.objects [objnum].type == OBJ_ROBOT)
					// -- MK: 11/18/95, 4claws glomming together...this is easy.  -- if (!(gameData.bots.pInfo [gameData.objs.objects [objnum].id].attack_type && gameData.bots.pInfo [gameData.objs.objects [nThisObject].id].attack_type))
						continue;
				if (gameData.bots.pInfo [gameData.objs.objects [nThisObject].id].attack_type)
					nFudgedRad = (rad*3)/4;
					}
			//if obj is player, and bumping into other player or a weapon of another coop player, reduce radius
			if (gameData.objs.objects [nThisObject].type == OBJ_PLAYER &&
					((gameData.objs.objects [objnum].type == OBJ_PLAYER) ||
					((gameData.app.nGameMode&GM_MULTI_COOP) &&  gameData.objs.objects [objnum].type == OBJ_WEAPON && gameData.objs.objects [objnum].ctype.laser_info.parent_type == OBJ_PLAYER)))
				nFudgedRad = rad/2;	//(rad*3)/4;

			d = CheckVectorToObject (&vHitPoint, p0, p1, nFudgedRad, gameData.objs.objects + objnum, 
												&gameData.objs.objects [nThisObject]);

			if (d)          //we have intersection
				if (d < dMin) {
					fviHitData.nObject = objnum;
					Assert(fviHitData.nObject!=-1);
					dMin = d;
					vClosestHitPoint = vHitPoint;
					nHitType = HIT_OBJECT;
				}
		}

if ((nThisObject > -1) && (CollisionResult [gameData.objs.objects [nThisObject].type][OBJ_WALL] == RESULT_NOTHING))
	rad = 0;		//HACK - ignore when edges hit walls
#else
nThisType = (nThisObject < 0) ? -1 : gameData.objs.objects [nThisObject].type;
if (flags & FQ_CHECK_OBJS) {
	for (objnum = segP->objects; objnum != -1; objnum = otherObjP->next) {
		otherObjP = gameData.objs.objects + objnum;
		nOtherType = otherObjP->type;
		if (otherObjP->flags & OF_SHOULD_BE_DEAD)
			continue;
		if (nThisObject == objnum)
			continue;
		if (ignoreObjList && ObjectInList (objnum, ignoreObjList))
			continue;
		if (LasersAreRelated (objnum, nThisObject))
			continue;
		if (nThisObject > -1) {
			if ((CollisionResult [nThisType][nOtherType] == RESULT_NOTHING) &&
				 (CollisionResult [nOtherType][nThisType] == RESULT_NOTHING))
				continue;
			}
		nFudgedRad = rad;

		//	If this is a powerup, don't do collision if flag FQ_IGNORE_POWERUPS is set
		if ((nOtherType == OBJ_POWERUP) && (flags & FQ_IGNORE_POWERUPS))
			continue;
		//	If this is a robot:robot collision, only do it if both of them have attack_type != 0 (eg, green guy)
		if (nThisType == OBJ_ROBOT) {
			if (nOtherType == OBJ_ROBOT)
				continue;
			if (gameData.bots.pInfo [thisObjP->id].attack_type)
				nFudgedRad = (rad * 3) / 4;
			}
		//if obj is player, and bumping into other player or a weapon of another coop player, reduce radius
		if ((nThisType == OBJ_PLAYER )&&
			 ((nOtherType == OBJ_PLAYER) ||
			  (IsCoopGame && (nOtherType == OBJ_WEAPON) && (otherObjP->ctype.laser_info.parent_type == OBJ_PLAYER))))
			nFudgedRad = rad / 2;

		d = CheckVectorToObject (&vHitPoint, p0, p1, nFudgedRad, otherObjP, thisObjP);

		if (d && (d < dMin)) {
			fviHitData.nObject = objnum;
			Assert(fviHitData.nObject!=-1);
			dMin = d;
			vClosestHitPoint = vHitPoint;
			nHitType = HIT_OBJECT;
			}
		}
	}

if ((nThisObject > -1) && (CollisionResult [nThisType][OBJ_WALL] == RESULT_NOTHING))
	rad = 0;		//HACK - ignore when edges hit walls
#endif
//now, check segment walls
startMask = GetSegMasks (p0, nStartSeg, (p1 == p0) ? 0 : rad).faceMask;
masks = GetSegMasks (p1, nStartSeg, rad);    //on back of which faces?
if (!(centerMask = masks.centerMask))
	nHitNoneSegment = nStartSeg;
if (endMask = masks.faceMask) { //on the back of at least one face
	short side, face, bit;

	//for each face we are on the back of, check if intersected
	for (side = 0, bit = 1; (side < 6) && (endMask >= bit); side++) {
		int nFaces = GetNumFaces (segP->sides + side);
		if (!nFaces)
			nFaces = 1;
		// commented out by mk on 02/13/94:: if ((nFaces=segP->sides [side].nFaces)==0) nFaces=1;
		for (face = 0; face < 2; face++, bit <<= 1) {
			if (endMask & bit) {            //on the back of this face
				int nFaceHitType;      //in what way did we hit the face?
				if (segP->children [side] == entrySegP)
					continue;		//don't go back through entry side
				//did we go through this wall/door?
				if (startMask & bit)		//start was also though.  Do extra check
					nFaceHitType = SpecialCheckLineToFace (&vHitPoint, p0, p1, segP, side, face, 5 - nFaces, rad);
				else
					//NOTE LINK TO ABOVE!!
					nFaceHitType = CheckLineToFace (&vHitPoint, p0, p1, segP, side, face, 5 - nFaces, rad);
				if (nFaceHitType) {            //through this wall/door
					int wid_flag;
					//if what we have hit is a door, check the adjoining segP
					if ((nThisObject == gameData.multi.players [gameData.multi.nLocalPlayer].objnum) && 
							(gameStates.app.cheats.bPhysics==0xBADA55)) {
						int childSide = segP->children [side];
						int special = gameData.segs.segment2s [childSide].special;
						wid_flag = WALL_IS_DOORWAY(segP, side, gameData.objs.objects + nThisObject);
						if ((childSide >= 0) && 
								(((special != SEGMENT_IS_BLOCKED) && (special != SEGMENT_IS_SKYBOX)) ||
								  (gameStates.gameplay.bSpeedBoost &&
								   ((gameData.segs.segment2s [nStartSeg].special != SEGMENT_IS_SPEEDBOOST) ||
								    (special == SEGMENT_IS_SPEEDBOOST)))))
 							wid_flag |= WID_FLY_FLAG;
						}
					else
						wid_flag = WALL_IS_DOORWAY (segP, side, gameData.objs.objects + nThisObject);

					if ((wid_flag & WID_FLY_FLAG) ||
						(((wid_flag & WID_RENDER_FLAG) && (wid_flag & WID_RENDPAST_FLAG)) &&
							((flags & FQ_TRANSWALL) || (flags & FQ_TRANSPOINT && CheckTransWall(&vHitPoint, segP, side, face))))) {

						int nNewSeg;
						vms_vector subHitPoint;
						int subHitType;
						short subHitSeg;
						vms_vector vSaveWallNorm = fviHitData.vNormal;
						short nSaveHitObj = fviHitData.nObject;
						int i;

						//do the check recursively on the next segment.

						nNewSeg = segP->children [side];
						for (i = 0; i < nSegsVisited && (nNewSeg != segsVisited[i]); i++)
							;
						if (i == nSegsVisited) {                //haven't visited here yet
							short tempSegList [MAX_FVI_SEGS], nTempSegs;
							segsVisited [nSegsVisited++] = nNewSeg;
							if (nSegsVisited >= MAX_SEGS_VISITED)
								goto quit_looking;		//we've looked a long time, so give up
							subHitType = FVICompute (&subHitPoint, &subHitSeg, p0, (short) nNewSeg, 
															p1, rad, nThisObject, ignoreObjList, flags, 
															tempSegList, &nTempSegs, nStartSeg);
							if (subHitType != HIT_NONE) {
								d = VmVecDist(&subHitPoint, p0);
								if (d < dMin) {
									dMin = d;
									vClosestHitPoint = subHitPoint;
									nHitType = subHitType;
									if (subHitSeg!=-1) 
										nHitSegment = subHitSeg;
									//copy segList
									if (flags & FQ_GET_SEGLIST) {
										int i = MAX_FVI_SEGS - 1 - *nSegments;
										if (i > nTempSegs)
											i = nTempSegs;
										memcpy (segList + *nSegments, tempSegList, i * sizeof (*segList));
										*nSegments += i;
										}
									Assert (*nSegments < MAX_FVI_SEGS);
									}
								else {
									fviHitData.vNormal = vSaveWallNorm;     //global could be trashed
									fviHitData.nObject = nSaveHitObj;
 									}
								}
							else {
								fviHitData.vNormal = vSaveWallNorm;     //global could be trashed
								if (subHitSeg != -1) 
									nHitNoneSegment = subHitSeg;
								//copy segList
								if (flags&FQ_GET_SEGLIST) {
									int i = MAX_FVI_SEGS - 1;
									if (i > nTempSegs)
										i = nTempSegs;
									memcpy (hitNoneSegList, tempSegList, i * sizeof (*hitNoneSegList));
									}
								nHitNoneSegs = nTempSegs;
								}
							}
						}
					else {          //a wall
						//is this the closest hit?
						d = VmVecDist(&vHitPoint, p0);
						if (d < dMin) {
							dMin = d;
							vClosestHitPoint = vHitPoint;
							nHitType = HIT_WALL;
#ifdef COMPACT_SEGS
							GetSideNormal(segP, side, face, &fviHitData.vNormal);
#else
							fviHitData.vNormal = segP->sides [side].normals [face];	
#endif
							if (!GetSegMasks (&vHitPoint, nStartSeg, rad).centerMask)
								nHitSegment = nStartSeg;             //hit in this segment
							else
								fviHitData.nSegment2 = nStartSeg;
							fviHitData.nSegment = nHitSegment;
							fviHitData.nSide = side;
							fviHitData.nSideSegment = nStartSeg;
							}
						}
					}
				}
			}
		}
	}
quit_looking:
	;

if (nHitType == HIT_NONE) {     //didn't hit anything, return end point
	int i;

	*vIntP = *p1;
	*intS = nHitNoneSegment;
	if (nHitNoneSegment!=-1) {			//(centerMask == 0)
		if (flags & FQ_GET_SEGLIST) {
			i = MAX_FVI_SEGS - 1 - *nSegments;
			if (i > nHitNoneSegs)
				i = nHitNoneSegs;
			memcpy (segList + *nSegments, hitNoneSegList, i * sizeof (*segList));
			*nSegments += i;
			}
		}
	else
		if (nCurNestLevel!=0)
			*nSegments=0;
	}
else {
	*vIntP = vClosestHitPoint;
	if (nHitSegment==-1)
		if (fviHitData.nSegment2 != -1)
			*intS = fviHitData.nSegment2;
		else
			*intS = nHitNoneSegment;
	else
		*intS = nHitSegment;
	}
Assert(!(nHitType==HIT_OBJECT && fviHitData.nObject==-1));
return nHitType;
}

#include "textures.h"
#include "texmerge.h"

#define Cross(v0, v1) (FixMul((v0)->i, (v1)->j) - FixMul((v0)->j, (v1)->i))

//	-----------------------------------------------------------------------------
//finds the uv coords of the given point on the given seg & side
//fills in u & v. if l is non-NULL fills it in also
void FindHitPointUV (fix *u, fix *v, fix *l, vms_vector *pnt, segment *seg, int sidenum, int facenum)
{
	vms_vector_array *pnt_array;
	vms_vector_array normal_array;
	int segnum = SEG_IDX (seg);
	int num_faces;
	int biggest, ii, jj;
	side *side = &seg->sides [sidenum];
	int vertex_list [6], vertnum_list [6];
 	vec2d p1, vec0, vec1, checkp;	//@@, checkv;
	uvl uvls [3];
	fix k0, k1;
	int i;

//do lasers pass through illusory walls?
//when do I return 0 & 1 for non-transparent walls?
if (segnum < 0 || segnum > gameData.segs.nLastSegment) {
#if TRACE
	con_printf (CON_DEBUG, "Bad segnum (%d) in FindHitPointUV()\n", segnum);
#endif
	*u = *v = 0;
	return;
	}
if (segnum==-1)
	Error("segnum == -1 in FindHitPointUV()");
CreateAbsVertexLists(&num_faces, vertex_list, segnum, sidenum);
CreateAllVertNumLists(&num_faces, vertnum_list, segnum, sidenum);
//now the hard work.
//1. find what plane to project this wall onto to make it a 2d case
#ifdef COMPACT_SEGS
	GetSideNormal(seg, sidenum, facenum, (vms_vector *)&normal_array);
#else
	memcpy(&normal_array, &side->normals [facenum], sizeof(vms_vector_array));
#endif
biggest = 0;

if (abs(normal_array.xyz [1]) > abs(normal_array.xyz [biggest])) 
	biggest = 1;
if (abs(normal_array.xyz [2]) > abs(normal_array.xyz [biggest])) 
	biggest = 2;
ii = (biggest == 0);
jj = (biggest == 2) ? 1 : 2;
//2. compute u, v of intersection point
//vec from 1 -> 0
pnt_array = (vms_vector_array *)&gameData.segs.vertices [vertex_list [facenum*3+1]];
p1.i = pnt_array->xyz [ii];
p1.j = pnt_array->xyz [jj];

pnt_array = (vms_vector_array *)&gameData.segs.vertices [vertex_list [facenum*3+0]];
vec0.i = pnt_array->xyz [ii] - p1.i;
vec0.j = pnt_array->xyz [jj] - p1.j;

//vec from 1 -> 2
pnt_array = (vms_vector_array *)&gameData.segs.vertices [vertex_list [facenum*3+2]];
vec1.i = pnt_array->xyz [ii] - p1.i;
vec1.j = pnt_array->xyz [jj] - p1.j;

//vec from 1 -> checkpoint
pnt_array = (vms_vector_array *)pnt;
checkp.i = pnt_array->xyz [ii];
checkp.j = pnt_array->xyz [jj];

//@@checkv.i = checkp.i - p1.i;
//@@checkv.j = checkp.j - p1.j;

k1 = -FixDiv(Cross(&checkp, &vec0) + Cross(&vec0, &p1), Cross(&vec0, &vec1));
if (abs(vec0.i) > abs(vec0.j))
	k0 = FixDiv(FixMul(-k1, vec1.i) + checkp.i - p1.i, vec0.i);
else
	k0 = FixDiv(FixMul(-k1, vec1.j) + checkp.j - p1.j, vec0.j);

for (i=0;i<3;i++)
	uvls [i] = side->uvls [vertnum_list [facenum*3+i]];

*u = uvls [1].u + FixMul(k0, uvls [0].u - uvls [1].u) + FixMul(k1, uvls [2].u - uvls [1].u);
*v = uvls [1].v + FixMul(k0, uvls [0].v - uvls [1].v) + FixMul(k1, uvls [2].v - uvls [1].v);
if (l)
	*l = uvls [1].l + FixMul(k0, uvls [0].l - uvls [1].l) + FixMul(k1, uvls [2].l - uvls [1].l);
}

//	-----------------------------------------------------------------------------

int PixelTranspType (short nTexture, fix u, fix v)
{
	grs_bitmap *bmP;
	int bmx, bmy, w, h, offs, orient;
	unsigned char	c;
	bitmap_index *bmiP;

//	Assert(WALL_IS_DOORWAY(seg, sidenum) == WID_TRANSPARENT_WALL);

bmiP = gameData.pig.tex.pBmIndex + (nTexture & 0x3fff);
PIGGY_PAGE_IN (*bmiP, gameStates.app.bD1Data);
bmP = BmOverride (gameData.pig.tex.pBitmaps + bmiP->index);
if (bmP->bm_props.flags & BM_FLAG_RLE)
	bmP = rle_expand_texture (bmP);
orient = (nTexture >> 14) & 3;
w = bmP->bm_props.w;
h = ((bmP->bm_type == BM_TYPE_ALT) && BM_FRAMES (bmP)) ? w : bmP->bm_props.h; 
if (orient == 0) {
	bmx = ((unsigned) f2i (u * w)) % w;
	bmy = ((unsigned) f2i (v * h)) % h;
	}
else if (orient == 1) {
	bmx = ((unsigned) f2i ((F1_0 - v) * w)) % w;
	bmy = ((unsigned) f2i (u * h)) % h;
	}
else if (orient == 2) {
	bmx = ((unsigned) f2i ((F1_0 - u) * w)) % w;
	bmy = ((unsigned) f2i ((F1_0 - v) * h)) % h;
	}
else {
	bmx = ((unsigned) f2i (v * w)) % w;
	bmy = ((unsigned) f2i ((F1_0 - u) * h)) % h;
	}
offs = bmy * w + bmx;
if (bmP->bm_props.flags & BM_FLAG_TGA) {
	ubyte *p = bmP->bm_texBuf + offs * 4;
	// check super transparency color
	if ((gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk) ?
	    (p[3] == 1) : ((p[0] == 120) && (p[1] == 88) && (p[2] == 128)))
		return -1;
	// check alpha
	if (!p[3])
		return 1;
	}
else {
	c = bmP->bm_texBuf[offs];
	if (c == SUPER_TRANSP_COLOR) 
		return -1;
	if (c == TRANSPARENCY_COLOR) 
		return 1;
	}
return 0;
}

//	-----------------------------------------------------------------------------
//check if a particular point on a wall is a transparent pixel
//returns 1 if can pass though the wall, else 0
int CheckTransWall (vms_vector *pnt, segment *seg, short sidenum, short facenum)
{
	side *side = seg->sides + sidenum;
	fix	u, v;
	int	nTranspType;

//	Assert(WALL_IS_DOORWAY(seg, sidenum) == WID_TRANSPARENT_WALL);

FindHitPointUV (&u, &v, NULL, pnt, seg, sidenum, facenum);	//	Don't compute light value.
if (side->tmap_num2)	{
	nTranspType = PixelTranspType (side->tmap_num2, u, v);
	if (nTranspType < 0)
		return 1;
	if (!nTranspType)
		return 0;
	}
return PixelTranspType (side->tmap_num, u, v) != 0;
}

//	-----------------------------------------------------------------------------
//new function for Mike
//note: nSegsVisited must be set to zero before this is called
int SphereIntersectsWall(vms_vector *pnt, int segnum, fix rad)
{
	int faceMask;
	segment *seg;

segsVisited [nSegsVisited++] = segnum;

faceMask = GetSegMasks(pnt, segnum, rad).faceMask;

seg = gameData.segs.segments + segnum;

if (faceMask != 0) {				//on the back of at least one face
	int side, bit, face, child, i;
	int nFaceHitType;      //in what way did we hit the face?
	int num_faces, vertex_list [6];

//for each face we are on the back of, check if intersected
	for (side=0, bit=1;side<6 && faceMask>=bit;side++) {
		for (face=0;face<2;face++, bit<<=1) {
			if (faceMask & bit) {            //on the back of this face
				//did we go through this wall/door?
				if ((SEG_IDX (seg))==-1)
					Error("segnum == -1 in SphereIntersectsWall()");
				CreateAbsVertexLists(&num_faces, vertex_list, SEG_IDX (seg), side);
				nFaceHitType = CheckSphereToFace(pnt, &seg->sides [side], 
									face, ((num_faces==1)?4:3), rad, vertex_list);
				if (nFaceHitType) {            //through this wall/door
					//if what we have hit is a door, check the adjoining seg
					child = seg->children [side];
					for (i = 0; (i < nSegsVisited) && (child != segsVisited [i]); i++)
						;
					if (i == nSegsVisited) {                //haven't visited here yet
						if (!IS_CHILD(child))
							return 1;
						else {
							if (SphereIntersectsWall(pnt, child, rad))
								return 1;
							}
						}
					}
				}
			}
		}
	}
return 0;
}

//	-----------------------------------------------------------------------------
//Returns true if the object is through any walls
int ObjectIntersectsWall(object *objP)
{
return SphereIntersectsWall(&objP->pos, objP->segnum, objP->size);
}

//	-----------------------------------------------------------------------------
//eof
