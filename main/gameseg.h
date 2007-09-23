/* $Id: gameseg.h,v 1.2 2003/10/04 03:14:47 btb Exp $ */
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
 * Header file for stuff moved from tSegment.c to gameseg.c.
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:57:18  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:31:20  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.24  1995/02/01  16:34:03  john
 * Linted.
 *
 * Revision 1.23  1995/01/16  21:06:36  mike
 * Move function pick_random_point_in_segment from fireball.c to gameseg.c.
 *
 * Revision 1.22  1994/11/23  12:18:59  mike
 * prototype for level names.
 *
 * Revision 1.21  1994/11/17  14:56:59  mike
 * moved tSegment validation functions from editor to main.
 *
 * Revision 1.20  1994/11/16  23:38:46  mike
 * new improved boss teleportation behavior.
 *
 * Revision 1.19  1994/10/30  14:12:14  mike
 * rip out local segments stuff.
 *
 * Revision 1.18  1994/10/09  23:51:07  matt
 * Made FindHitPointUV() work with triangulated sides
 *
 * Revision 1.17  1994/10/06  14:08:22  matt
 * Added new function, ExtractOrientFromSegment()
 *
 * Revision 1.16  1994/09/19  21:05:52  mike
 * Prototype for FindConnectedDistance.
 *
 * Revision 1.15  1994/08/11  18:58:45  mike
 * Change shorts to ints.
 *
 * Revision 1.14  1994/08/04  00:21:09  matt
 * Cleaned up fvi & physics error handling; put in code to make sure objects
 * are in correct tSegment; simplified tSegment finding for objects and points
 *
 * Revision 1.13  1994/08/02  19:04:25  matt
 * Cleaned up vertex list functions
 *
 * Revision 1.12  1994/07/21  19:01:53  mike
 * lsegment stuff.
 *
 * Revision 1.11  1994/07/07  09:31:13  matt
 * Added comments
 *
 * Revision 1.10  1994/06/14  12:21:20  matt
 * Added new function, FindSegByPoint()
 *
 * Revision 1.9  1994/05/29  23:17:38  matt
 * Move FindObjectSeg() from physics.c to gameseg.c
 * Killed unused FindSegByPoint()
 *
 * Revision 1.8  1994/05/20  11:56:57  matt
 * Cleaned up FindVectorIntersection() interface
 * Killed check_point_in_seg(), check_player_seg(), checkObject_seg()
 *
 * Revision 1.7  1994/03/17  18:07:38  yuan
 * Removed switch code... Now we just have Walls, Triggers, and Links...
 *
 * Revision 1.6  1994/02/22  18:14:44  yuan
 * Added new tWall system
 *
 * Revision 1.5  1994/02/17  11:33:22  matt
 * Changes in tObject system
 *
 * Revision 1.4  1994/02/16  13:48:33  mike
 * enable editor to compile out.
 *
 * Revision 1.3  1994/02/14  12:05:07  mike
 * change tSegment data structure.
 *
 * Revision 1.2  1994/02/10  16:07:20  mike
 * separate editor from game based on EDITOR flag.
 *
 * Revision 1.1  1994/02/09  15:45:38  mike
 * Initial revision
 *
 *
 */


#ifndef _GAMESEG_H
#define _GAMESEG_H

#include "pstypes.h"
#include "fix.h"
#include "vecmat.h"
#include "segment.h"

#define PLANE_DIST_TOLERANCE	250

//figure out what seg the given point is in, tracing through segments
int get_new_seg(vmsVector *p0,int startseg);

typedef struct segmasks {
   short faceMask;     //which faces sphere pokes through (12 bits)
   sbyte sideMask;     //which sides sphere pokes through (6 bits)
   sbyte centerMask;   //which sides center point is on back of (6 bits)
	sbyte valid;
} segmasks;


void ComputeSideCenter(vmsVector *vp,tSegment *sp,int tSide);
void ComputeSideRads (short nSegment, short tSide, fix *prMin, fix *prMax);
void ComputeSegmentCenter(vmsVector *vp,tSegment *sp);
int FindConnectedSide(tSegment *base_seg, tSegment *con_seg);

#define	SEGMENT_CENTER_I(_nSeg)	(gameData.segs.segCenters [0] + (_nSeg))

#define	COMPUTE_SEGMENT_CENTER_I(_pc,_nSeg) *(_pc) = (*SEGMENT_CENTER_I (_nSeg))

#define	COMPUTE_SIDE_CENTER_I(_pc,_nSeg,_nSide) \
			*(_pc) = gameData.segs.sideCenters [(_nSeg) * 6 + (_nSide)]

#define	COMPUTE_SEGMENT_RAD_I(_pc,_nSeg,_nSide) \
			*(_pc) = gameData.segs.segRads [_nSeg]

#define	COMPUTE_SEGMENT_CENTER(_pc,_segP) \
			COMPUTE_SEGMENT_CENTER_I (_pc, SEG_IDX (_segP))

#define	COMPUTE_SIDE_CENTER(_pc,_segP,_nSide) \
			COMPUTE_SIDE_CENTER_I (_pc, SEG_IDX (_segP), _nSide)

#define	COMPUTE_SEGMENT_RAD(_pc,_segP) \
			COMPUTE_SEGMENT_RAD_I (_pc, SEG_IDX (_segP))

// Fill in array with four absolute point numbers for a given tSide
void GetSideVertIndex (short *vertIndex, int nSegment, int nSide);
void GetSideVerts (vmsVector *vertices, int nSegment, int nSide);
ubyte GetSideDists (vmsVector *checkp, int nSegment, fix *xSideDists, int bBehind);
ubyte GetSideDistsAll (vmsVector *checkp, int nSegment, fix *xSideDists);

//      Create all vertex lists (1 or 2) for faces on a tSide.
//      Sets:
//              num_faces               number of lists
//              vertices                        vertices in all (1 or 2) faces
//      If there is one face, it has 4 vertices.
//      If there are two faces, they both have three vertices, so face #0 is stored in vertices 0,1,2,
//      face #1 is stored in vertices 3,4,5.
// Note: these are not absolute vertex numbers, but are relative to the tSegment
// Note:  for triagulated sides, the middle vertex of each trianle is the one NOT
//   adjacent on the diagonal edge
void CreateAllVertexLists (int *num_faces, int *vertices, int nSegment, int nSide);

//like create_all_vertex_lists(), but generate absolute point numbers
int CreateAbsVertexLists (int *vertices, int nSegment, int nSide);

// -----------------------------------------------------------------------------------
// Like create all vertex lists, but returns the vertnums (relative to
// the tSide) for each of the faces that make up the tSide.
//      If there is one face, it has 4 vertices.
//      If there are two faces, they both have three vertices, so face #0 is stored in vertices 0,1,2,
//      face #1 is stored in vertices 3,4,5.
void CreateAllVertNumLists(int *num_faces, int *vertnums, int nSegment, int nSide);

//      Given a tSide, return the number of faces
extern int GetNumFaces(tSide *sidep);

//returns 3 different bitmasks with info telling if this sphere is in
//this tSegment.  See segmasks structure for info on fields
segmasks GetSegMasks(vmsVector *checkp,int nSegment,fix rad);

//this macro returns true if the nSegment for an tObject is correct
#define check_obj_seg(obj) (GetSegMasks(&(obj)->pos,(obj)->nSegment,0).centermask == 0)

//Tries to find a tSegment for a point, in the following way:
// 1. Check the given tSegment
// 2. Recursively trace through attached segments
// 3. Check all the segmentns
//Returns nSegment if found, or -1
int FindSegByPoint(vmsVector *vPos, int nSegment, int bExhaustive);

//--repair-- // Create data specific to segments which does not need to get written to disk.
//--repair-- extern void create_local_segment_data(void);

//      Sort of makes sure create_local_segment_data has been called for the currently executing mine.
//      Returns 1 if Lsegments appears valid, 0 if not.
int check_lsegments_validity(void);

//      ----------------------------------------------------------------------------------------------------------
//      Determine whether seg0 and seg1 are reachable using widFlag to go through walls.
//      For example, set to WID_RENDPAST_FLAG to see if sound can get from one tSegment to the other.
//      set to WID_FLY_FLAG to see if a robot could fly from one to the other.
//      Search up to a maximum depth of max_depth.
//      Return the distance.
fix FindConnectedDistance(vmsVector *p0, short seg0, vmsVector *p1, short seg1, int max_depth, int widFlag);

//create a matrix that describes the orientation of the given tSegment
void ExtractOrientFromSegment(vmsMatrix *m,tSegment *seg);

//      In tSegment.c
//      Make a just-modified tSegment valid.
//              check all sides to see how many faces they each should have (0,1,2)
//              create new vector normals
void ValidateSegment(tSegment *sp);

void ValidateSegmentAll(void);

//      Extract the forward vector from tSegment *sp, return in *vp.
//      The forward vector is defined to be the vector from the the center of the front face of the tSegment
// to the center of the back face of the tSegment.
void extract_forward_vector_from_segment(tSegment *sp,vmsVector *vp);

//      Extract the right vector from tSegment *sp, return in *vp.
//      The forward vector is defined to be the vector from the the center of the left face of the tSegment
// to the center of the right face of the tSegment.
void extract_right_vector_from_segment(tSegment *sp,vmsVector *vp);

//      Extract the up vector from tSegment *sp, return in *vp.
//      The forward vector is defined to be the vector from the the center of the bottom face of the tSegment
// to the center of the top face of the tSegment.
void extract_up_vector_from_segment(tSegment *sp,vmsVector *vp);

void CreateWallsOnSide(tSegment *sp, int nSide);

void PickRandomPointInSeg(vmsVector *new_pos, int nSegment);

int GetVertsForNormal (int v0, int v1, int v2, int v3, int *pv0, int *pv1, int *pv2, int *pv3);
int GetVertsForNormalTri (int v0, int v1, int v2, int *pv0, int *pv1, int *pv2);

#endif


