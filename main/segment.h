/* $Id: segment.h,v 1.4 2003/10/04 03:14:47 btb Exp $ */
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

#ifndef _SEGMENT_H
#define _SEGMENT_H

#include "pstypes.h"
#include "fix.h"
#include "vecmat.h"
#include "3d.h"
//#include "inferno.h"
#include "cfile.h"
#include "gr.h"

// Version 1 - Initial version
// Version 2 - Mike changed some shorts to bytes in segments, so incompatible!

#define SIDE_IS_QUAD    1   // render tSide as quadrilateral
#define SIDE_IS_TRI_02  2   // render tSide as two triangles, triangulated along edge from 0 to 2
#define SIDE_IS_TRI_13  3   // render tSide as two triangles, triangulated along edge from 1 to 3

// Set maximum values for tSegment and face data structures.
#define MAX_VERTICES_PER_SEGMENT    8
#define MAX_SIDES_PER_SEGMENT       6
#define MAX_VERTICES_PER_POLY       4
#define WLEFT                       0
#define WTOP                        1
#define WRIGHT                      2
#define WBOTTOM                     3
#define WBACK                       4
#define WFRONT                      5

#define MAX_SEGMENTS_D2        900
#define MAX_SEGMENTS           5000
#define MAX_SEGMENT_VERTICES   (MAX_SEGMENTS * 4 + 8)

//normal everyday vertices

#define DEFAULT_LIGHTING        0   // (F1_0/2)

#ifdef EDITOR   //verts for the new tSegment
# define NUM_NEW_SEG_VERTICES   8
# define NEW_SEGMENT_VERTICES   (MAX_SEGMENT_VERTICES)
# define MAX_VERTICES           (MAX_SEGMENT_VERTICES+NUM_NEW_SEG_VERTICES)
#else           //No editor
# define MAX_VERTICES           (MAX_SEGMENT_VERTICES)
#endif

// Returns true if nSegment references a child, else returns false.
// Note that -1 means no connection, -2 means a connection to the outside world.
#define IS_CHILD(nSegment) (nSegment > -1)

#if 0
//Structure for storing u,v,light values.
//NOTE: this structure should be the same as the one in 3d.h
typedef struct uvl {
	fix u, v, l;
} uvl;
#endif

#ifdef COMPACT_SEGS
typedef struct tSide {
	sbyte   nType;           // replaces num_faces and tri_edge, 1 = quad, 2 = 0:2 triangulation, 3 = 1:3 triangulation
	ubyte   pad;            //keep us longword alligned
	short   nWall;
	short   nTexture [2];
	uvl     uvls [4];
	//vmsVector normals[2];  // 2 normals, if quadrilateral, both the same.
} tSide;
#else
typedef struct tSide {
	sbyte   		nType;           // replaces num_faces and tri_edge, 1 = quad, 2 = 0:2 triangulation, 3 = 1:3 triangulation
	sbyte   		nFrame;      //keep us longword aligned
	ushort  		nWall;
	short   		nBaseTex;
	ushort		nOvlTex : 14;
	ushort		nOvlOrient : 2;
	uvl     		uvls [4];
	vmsVector	normals [2];  // 2 normals, if quadrilateral, both the same.
} tSide;
#endif

typedef struct tSegment {
#ifdef EDITOR
	short   nSegment;     // tSegment number, not sure what it means
#endif
	tSide   sides [MAX_SIDES_PER_SEGMENT];       // 6 sides
	short   children [MAX_SIDES_PER_SEGMENT];    // indices of 6 children segments, front, left, top, right, bottom, back
	short   verts [MAX_VERTICES_PER_SEGMENT];    // vertex ids of 4 front and 4 back vertices
#ifdef EDITOR
	short   group;      // group number to which the tSegment belongs.
	short   objects;    // pointer to objects in this tSegment
#else
	int     objects;    // pointer to objects in this tSegment
#endif
} tSegment;

typedef struct xsegment {
	char		owner;		  // team owning that tSegment (-1: always neutral, 0: neutral, 1: blue team, 2: red team)
	char		group;
} xsegment;


#define S2F_AMBIENT_WATER   0x01
#define S2F_AMBIENT_LAVA    0x02

typedef struct segment2 {
	ubyte   special;
	sbyte   nMatCen;
	sbyte   value;
	ubyte   s2Flags;
	fix     static_light;
} segment2;

//values for special field
#define SEGMENT_IS_NOTHING      0
#define SEGMENT_IS_FUELCEN      1
#define SEGMENT_IS_REPAIRCEN    2
#define SEGMENT_IS_CONTROLCEN   3
#define SEGMENT_IS_ROBOTMAKER   4
#define SEGMENT_IS_GOAL_BLUE    5
#define SEGMENT_IS_GOAL_RED     6
#define SEGMENT_IS_WATER        7
#define SEGMENT_IS_LAVA         8
#define SEGMENT_IS_TEAM_BLUE    9
#define SEGMENT_IS_TEAM_RED     10
#define SEGMENT_IS_SPEEDBOOST	  11
#define SEGMENT_IS_BLOCKED		  12
#define SEGMENT_IS_NODAMAGE	  13
#define SEGMENT_IS_SKYBOX		  14
#define MAX_CENTER_TYPES        15

#ifdef COMPACT_SEGS
extern void GetSideNormal(tSegment *sp, int nSide, int normal_num, vmsVector * vm );
extern void GetSideNormals(tSegment *sp, int nSide, vmsVector * vm1, vmsVector *vm2 );
#endif

// Local tSegment data.
// This is stuff specific to a tSegment that does not need to get
// written to disk.  This is a handy separation because we can add to
// this structure without obsoleting existing data on disk.

#define SS_REPAIR_CENTER    0x01    // Bitmask for this tSegment being part of repair center.

//--repair-- typedef struct {
//--repair-- 	int     specialType;
//--repair-- 	short   special_segment; // if specialType indicates repair center, this is the base of the repair center
//--repair-- } lsegment;

typedef struct {
	int     num_segments;
	int     num_vertices;
	short   segments[MAX_SEGMENTS];
	short   vertices[MAX_VERTICES];
} group;

// Globals from mglobal.c
extern sbyte sideToVerts[MAX_SIDES_PER_SEGMENT][4];       // sideToVerts[my_side] is list of vertices forming tSide my_side.
extern int  sideToVertsInt[MAX_SIDES_PER_SEGMENT][4];    // sideToVerts[my_side] is list of vertices forming tSide my_side.
extern char sideOpposite[];                                // sideOpposite[my_side] returns tSide opposite cube from my_side.

// New stuff, 10/14/95: For shooting out lights and monitors.
// Light cast upon vert_light vertices in nSegment:nSide by some light
typedef struct {
	short   nSegment;
	sbyte   nSide;
	sbyte   dummy;
	ubyte   vert_light[4];
} delta_light;

// Light at nSegment:nSide casts light on count sides beginning at index (in array gameData.render.lightDeltas)
typedef struct {
	short   nSegment;
	ubyte   nSide;
	ubyte   count;
	unsigned short   index;
} dl_index_d2;

typedef struct {
	short   nSegment;
	unsigned short nSide :3;
	unsigned short count :13;
	unsigned short   index;
} dl_index_d2x;

typedef union {
	dl_index_d2x	d2x;
	dl_index_d2		d2;
} dl_index;

#define MAX_DL_INDICES_D2    500
#define MAX_DELTA_LIGHTS_D2  10000

#define MAX_DL_INDICES       2500
#define MAX_DELTA_LIGHTS     50000

#define DL_SCALE            2048    // Divide light to allow 3 bits integer, 5 bits fraction.

int SubtractLight(short nSegment, short nSide);
int AddLight(short nSegment, short nSide);
void restore_all_lights_in_mine(void);
void ClearLightSubtracted(void);

// ----------------------------------------------------------------------------
// --------------------- Segment interrogation functions ----------------------
// Do NOT read the tSegment data structure directly.  Use these
// functions instead.  The tSegment data structure is GUARANTEED to
// change MANY TIMES.  If you read the tSegment data structure
// directly, your code will break, I PROMISE IT!

// Return a pointer to the list of vertex indices for the current
// tSegment in vp and the number of vertices in *nv.
extern void med_get_vertex_list(tSegment *s,int *nv,short **vp);

// Return a pointer to the list of vertex indices for face facenum in
// vp and the number of vertices in *nv.
extern void med_get_face_vertex_list(tSegment *s,int tSide, int facenum,int *nv,short **vp);

// Set *nf = number of faces in tSegment s.
extern void med_get_num_faces(tSegment *s,int *nf);

void med_validate_segment_side(tSegment *sp, short tSide);

// Delete tSegment function added for curves.c
extern int med_delete_segment(tSegment *sp);

// Delete tSegment from group
extern void delete_segment_from_group(int segment_num, int group_num);

// Add tSegment to group
extern void add_segment_to_group(int segment_num, int group_num);

// Verify that all vertices are legal.
extern void med_check_all_vertices();

#ifdef FAST_FILE_IO
#define segment2_read(s2, fp) CFRead(s2, sizeof(segment2), 1, fp)
#define delta_light_read(dl, fp) CFRead(dl, sizeof(delta_light), 1, fp)
#define dl_index_read(di, fp) CFRead(di, sizeof(dl_index), 1, fp)
#else
/*
 * reads a segment2 structure from a CFILE
 */
void segment2_read(segment2 *s2, CFILE *fp);

/*
 * reads a delta_light structure from a CFILE
 */
void delta_light_read(delta_light *dl, CFILE *fp);

/*
 * reads a dl_index structure from a CFILE
 */
void dl_index_read(dl_index *di, CFILE *fp);
#endif

#endif
