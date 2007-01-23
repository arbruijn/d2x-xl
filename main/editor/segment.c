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

/*
 * $Source: /cvs/cvsroot/d2x/main/editor/tSegment.c,v $
 * $Revision: 1.1 $
 * $Author: bradleyb $
 * $Date: 2001/10/25 02:27:17 $
 *
 * Interrogation functions for tSegment data structure.
 *
 * $Log: tSegment.c,v $
 * Revision 1.1  2001/10/25 02:27:17  bradleyb
 * attempt at support for editor, makefile changes, etc
 *
 * Revision 1.2  1999/09/02 13:37:06  sekmu
 * remove warning in editor compile
 *
 * Revision 1.1.1.1  1999/06/14 22:04:21  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:35:21  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.191  1995/02/22  15:28:30  allender
 * remove anonymous unions from tObject structure
 * 
 * Revision 1.190  1995/02/02  02:59:40  yuan
 * Working on exterminating bogus matcen_nums... (harmless though)
 * 
 * Revision 1.189  1995/02/01  16:29:51  yuan
 * Stabilizing triggers and matcens.
 * 
 * Revision 1.188  1995/02/01  11:31:47  yuan
 * Trigger bug fixed.
 * 
 * Revision 1.187  1994/11/27  23:17:24  matt
 * Made changes for new con_printf calling convention
 * 
 * Revision 1.186  1994/11/17  14:48:12  mike
 * validation functions moved from editor to game.
 * 
 * Revision 1.185  1994/10/30  14:13:17  mike
 * rip out local tSegment stuff.
 * 
 * Revision 1.184  1994/10/27  10:04:24  matt
 * When triangulating, don't use WID() to see if connected, look at children
 * 
 * Revision 1.183  1994/10/26  13:40:23  mike
 * debug code for matt.
 * 
 * Revision 1.182  1994/10/24  16:34:00  mike
 * Force render after mine compress to prevent bugs in tSegment selection via clicking in 3d window.
 * 
 * Revision 1.181  1994/10/20  18:16:15  mike
 * Initialize gameData.reactor.triggers.nLinks in create_new_mine.
 * 
 * Revision 1.180  1994/10/18  16:29:14  mike
 * Write function to automatically fix bogus segnums in tSegment array.
 * 
 * Revision 1.179  1994/10/08  17:10:41  matt
 * Correctly set currentLevel_num when loading/creating mine in editor
 * 
 * Revision 1.178  1994/09/25  14:17:51  mike
 * Initialize (to 0) gameData.matCens.nMatCens and gameData.walls.nOpenDoors at mine creation.
 * 
 * Revision 1.177  1994/09/20  14:36:06  mike
 * Write function to find overlapping segments.
 * 
 * Revision 1.176  1994/08/25  21:55:57  mike
 * IS_CHILD stuff.
 * 
 * Revision 1.175  1994/08/23  15:28:03  mike
 * Fix peculiarity in med_combine_duplicate_vertices.
 * 
 * Revision 1.174  1994/08/09  16:06:17  john
 * Added the ability to place players.  Made old
 * Player variable be gameData.objs.console.
 * 
 * Revision 1.173  1994/08/05  21:18:10  matt
 * Allow two doors to be linked together
 * 
 * Revision 1.172  1994/08/04  19:13:16  matt
 * Changed a bunch of vecmat calls to use multiple-function routines, and to
 * allow the use of C macros for some functions
 * 
 * Revision 1.171  1994/07/22  12:37:00  matt
 * Cleaned up editor/game interactions some more.
 * 
 * Revision 1.170  1994/07/22  11:20:08  mike
 * Set Lsegments validity.
 * 
 * Revision 1.169  1994/07/21  19:02:49  mike
 * lsegment stuff.
 * 
 * Revision 1.168  1994/07/21  13:27:17  matt
 * Ripped out remants of old demo system, and added demo
 * disables tObject movement and game options from menu.
 * 
 * Revision 1.167  1994/07/19  20:15:48  matt
 * Name for each level now saved in the .SAV file & stored in gameData.missions.szCurrentLevel
 * 
 * Revision 1.166  1994/07/06  12:42:45  john
 * Made generic messages for hostages.
 * 
 * Revision 1.165  1994/06/24  17:04:29  john
 * *** empty log message ***
 * 
 * Revision 1.164  1994/06/15  15:42:40  mike
 * Initialize xAvgSegLight field in new segments.
 * 
 * Revision 1.163  1994/06/13  17:49:19  mike
 * Fix bug in med_validate_side which was toasting lighting for removable walls.
 * 
 * Revision 1.162  1994/06/13  10:52:20  mike
 * Fix bug in triangulation of sides between connected segments.
 * Was assigning SIDE_IS_02 regardless of how triangulated, was
 * causing physics bugs.
 * 
 * Revision 1.161  1994/06/08  18:14:16  mike
 * Fix triangulation of sides in hallways (ie, where there is no wall),
 * so they get triangulated the same way, so find_new_seg doesn't get
 * stuck in an infinite recursion.
 * 
 * Revision 1.160  1994/06/08  11:44:31  mike
 * Fix bug in normals not being opposite on opposite sides of a tSegment.
 * Problem occurred due to difference in handling of remainder in signed divide.
 * 
 * Revision 1.159  1994/05/31  19:00:15  yuan
 * Fixed gamestate restore.
 * 
 * Revision 1.158  1994/05/30  20:22:36  yuan
 * New triggers.
 * 
 * Revision 1.157  1994/05/26  19:32:51  mike
 * Add bfs_parse.
 * 
 * Revision 1.156  1994/05/23  14:56:46  mike
 * make current tSegment be add tSegment.,
 * 
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "mono.h"
#include "key.h"
#include "gr.h"

#include "inferno.h"
#include "segment.h"
// #include "tSegment2.h"
#include "editor.h"
#include "error.h"
#include "object.h"

#include "gameseg.h"
#include "render.h"
#include "game.h"

#include "wall.h"
#include "switch.h"
#include "fuelcen.h"
#include "cntrlcen.h"
#include "seguvs.h"
#include "gameseq.h"

#include "medwall.h"
#include "hostage.h"

int	Do_duplicate_vertex_check = 0;		// Gets set to 1 in med_create_duplicate_vertex, means to check for duplicate vertices in compress_mine

#define	BOTTOM_STUFF	0

//	Remap all vertices in polygons in a tSegment through translation table xlate_verts.
#if BOTTOM_STUFF
void remap_vertices(tSegment *segp, int *xlate_verts)
{
	int	nSide, facenum, polynum, v;

	for (nSide=0; nSide<MAX_SIDES_PER_SEGMENT; nSide++)
		for (facenum=0; facenum<segp->sides[nSide].num_faces; facenum++)
			for (polynum=0; polynum<segp->sides[nSide].faces[facenum].num_polys; polynum++) {
				poly *pp = &segp->sides[nSide].faces[facenum].polys[polynum];
				for (v=0; v<pp->num_vertices; v++)
					pp->verts[v] = xlate_verts[pp->verts[v]];
			}
}

//	Copy everything from sourceside to destside except sourceside->faces[xx].polys[xx].verts
void copy_side_except_vertex_ids(tSide *destside, tSide *sourceside)
{
	int	facenum, polynum, v;

	destside->num_faces = sourceside->num_faces;
	destside->tri_edge = sourceside->tri_edge;
	destside->nWall = sourceside->nWall;

	for (facenum=0; facenum<sourceside->num_faces; facenum++) {
		face *destface = &destside->faces[facenum];
		face *sourceface = &sourceside->faces[facenum];

		destface->num_polys = sourceface->num_polys;
		destface->normal = sourceface->normal;

		for (polynum=0; polynum<sourceface->num_polys; polynum++) {
			poly *destpoly = &destface->polys[polynum];
			poly *sourcepoly = &sourceface->polys[polynum];

			destpoly->num_vertices = sourcepoly->num_vertices;
			destpoly->faceType = sourcepoly->faceType;
			destpoly->nBaseTex = sourcepoly->nBaseTex;
			destpoly->nOvlTex = sourcepoly->nOvlTex;

			for (v=0; v<sourcepoly->num_vertices; v++)
				destpoly->uvls[v] = sourcepoly->uvls[v];
		}

	}
}

//	[tSide] [index] [cur:next]
//	To remap the vertices on a tSide after a forward rotation
byte xlate_previous[6][4][2] = {
{ {7, 3}, {3, 2}, {2, 6}, {6, 7} },		// remapping left to left
{ {5, 4}, {4, 0}, {7, 3}, {6, 7} },		// remapping back to top
{ {5, 4}, {1, 5}, {0, 1}, {4, 0} },		// remapping right to right
{ {0, 1}, {1, 5}, {2, 6}, {3, 2} },		//	remapping front to bottom
{ {1, 5}, {5, 4}, {6, 7}, {2, 6} },		// remapping bottom to back
{ {4, 0}, {0, 1}, {3, 2}, {7, 3} },		// remapping top to front
};

void remap_vertices_previous(tSegment *segp, int nSide)
{
	int	v, w, facenum, polynum;

	for (facenum=0; facenum<segp->sides[nSide].num_faces; facenum++) {
		for (polynum=0; polynum<segp->sides[nSide].faces[facenum].num_polys; polynum++) {
			poly *pp = &segp->sides[nSide].faces[facenum].polys[polynum];

			for (v=0; v<pp->num_vertices; v++) {
				for (w=0; w<4; w++) {
					if (pp->verts[v] == xlate_previous[nSide][w][0]) {
						pp->verts[v] = xlate_previous[nSide][w][1];
						break;
					}
				}
				Assert(w<4);	// If w == 4, then didn't find current vertex in list, which means xlate_previous table is bogus
			}
		}
	}
}

byte xlate_previous_right[6][4][2] = {
{ {5, 6}, {6, 7}, {2, 3}, {1, 2} },		// bottom to left
{ {6, 7}, {7, 4}, {3, 0}, {2, 3} },		// left to top
{ {7, 4}, {4, 5}, {0, 1}, {3, 0} },		// top to right
{ {4, 5}, {5, 6}, {1, 2}, {0, 1} },		// right to bottom
{ {6, 7}, {5, 6}, {4, 5}, {7, 4} },		// back to back
{ {3, 2}, {0, 3}, {1, 0}, {2, 1} },		// front to front
};

void remap_vertices_previous_right(tSegment *segp, int nSide)
{
	int	v, w, facenum, polynum;

	for (facenum=0; facenum<segp->sides[nSide].num_faces; facenum++) {
		for (polynum=0; polynum<segp->sides[nSide].faces[facenum].num_polys; polynum++) {
			poly *pp = &segp->sides[nSide].faces[facenum].polys[polynum];

			for (v=0; v<pp->num_vertices; v++) {
				for (w=0; w<4; w++) {
					if (pp->verts[v] == xlate_previous_right[nSide][w][0]) {
						pp->verts[v] = xlate_previous_right[nSide][w][1];
						break;
					}
				}
				Assert(w<4);	// If w == 4, then didn't find current vertex in list, which means xlate_previous table is bogus
			}
		}
	}
}


// -----------------------------------------------------------------------------------
//	Takes top to front
void med_rotate_segment_forward(tSegment *segp)
{
	tSegment	seg_copy;
	int		i;

	seg_copy = *segp;

	seg_copy.verts[0] = segp->verts[4];
	seg_copy.verts[1] = segp->verts[0];
	seg_copy.verts[2] = segp->verts[3];
	seg_copy.verts[3] = segp->verts[7];
	seg_copy.verts[4] = segp->verts[5];
	seg_copy.verts[5] = segp->verts[1];
	seg_copy.verts[6] = segp->verts[2];
	seg_copy.verts[7] = segp->verts[6];

	seg_copy.children[WFRONT] = segp->children[WTOP];
	seg_copy.children[WTOP] = segp->children[WBACK];
	seg_copy.children[WBACK] = segp->children[WBOTTOM];
	seg_copy.children[WBOTTOM] = segp->children[WFRONT];

	seg_copy.sides[WFRONT] = segp->sides[WTOP];
	seg_copy.sides[WTOP] = segp->sides[WBACK];
	seg_copy.sides[WBACK] = segp->sides[WBOTTOM];
	seg_copy.sides[WBOTTOM] = segp->sides[WFRONT];

	for (i=0; i<6; i++)
		remap_vertices_previous(&seg_copy, i);

	*segp = seg_copy;
}

// -----------------------------------------------------------------------------------
//	Takes top to right
void med_rotate_segment_right(tSegment *segp)
{
	tSegment	seg_copy;
	int		i;

	seg_copy = *segp;

	seg_copy.verts[4] = segp->verts[7];
	seg_copy.verts[5] = segp->verts[4];
	seg_copy.verts[1] = segp->verts[0];
	seg_copy.verts[0] = segp->verts[3];
	seg_copy.verts[3] = segp->verts[2];
	seg_copy.verts[2] = segp->verts[1];
	seg_copy.verts[6] = segp->verts[5];
	seg_copy.verts[7] = segp->verts[6];

	seg_copy.children[WRIGHT] = segp->children[WTOP];
	seg_copy.children[WBOTTOM] = segp->children[WRIGHT];
	seg_copy.children[WLEFT] = segp->children[WBOTTOM];
	seg_copy.children[WTOP] = segp->children[WLEFT];

	seg_copy.sides[WRIGHT] = segp->sides[WTOP];
	seg_copy.sides[WBOTTOM] = segp->sides[WRIGHT];
	seg_copy.sides[WLEFT] = segp->sides[WBOTTOM];
	seg_copy.sides[WTOP] = segp->sides[WLEFT];

	for (i=0; i<6; i++)
		remap_vertices_previous_right(&seg_copy, i);

	*segp = seg_copy;
}

void make_curside_bottom_side(void)
{
	switch (Curside) {
		case WRIGHT:	med_rotate_segment_right(Cursegp);		break;
		case WTOP:		med_rotate_segment_right(Cursegp);		med_rotate_segment_right(Cursegp);		break;
		case WLEFT:		med_rotate_segment_right(Cursegp);		med_rotate_segment_right(Cursegp);		med_rotate_segment_right(Cursegp);		break;
		case WBOTTOM:	break;
		case WFRONT:	med_rotate_segment_forward(Cursegp);	break;
		case WBACK:		med_rotate_segment_forward(Cursegp);	med_rotate_segment_forward(Cursegp);	med_rotate_segment_forward(Cursegp);	break;
	}
	UpdateFlags = UF_WORLD_CHANGED;
}
#endif

int ToggleBottom(void)
{
	Render_only_bottom = !Render_only_bottom;
	UpdateFlags = UF_WORLD_CHANGED;
	return 0;
}
		
// ---------------------------------------------------------------------------------------------
//           ---------- Segment interrogation functions ----------
// ----------------------------------------------------------------------------
//	Return a pointer to the list of vertex indices for the current tSegment in vp and
//	the number of vertices in *nv.
void med_get_vertex_list(tSegment *s,int *nv,short **vp)
{
	*vp = s->verts;
	*nv = MAX_VERTICES_PER_SEGMENT;
}

// -------------------------------------------------------------------------------
//	Return number of times vertex vi appears in all segments.
//	This function can be used to determine whether a vertex is used exactly once in
//	all segments, in which case it can be freely moved because it is not connected
//	to any other tSegment.
int med_vertexCount(int vi)
{
	int		s,v;
	tSegment	*sp;
	int		count;

	count = 0;

	for (s=0; s<MAX_SEGMENTS; s++) {
		sp = &gameData.segs.segments[s];
		if (sp->nSegment != -1)
			for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
				if (sp->verts[v] == vi)
					count++;
	}

	return count;
}

// -------------------------------------------------------------------------------
int is_free_vertex(int vi)
{
	return med_vertexCount(vi) == 1;
}


// -------------------------------------------------------------------------------
// Move a d_free vertex in the tSegment by adding the vector *vofs to its coordinates.
//	Error handling:
// 	If the point is not d_free then:
//		If the point is not valid (probably valid = in 0..7) then:
//		If adding *vofs will cause a degenerate tSegment then:
//	Note, pi is the point index relative to the tSegment, not an absolute point index.
// For example, 3 is always the front upper left vertex.
void med_move_vertex(tSegment *sp, int pi, vmsVector *vofs)
{
	int	abspi;

	Assert((pi >= 0) && (pi <= 7);		// check valid range of point indices.

	abspi = sp->verts[pi];

	// Make sure vertex abspi is d_free.  If it is d_free, it appears exactly once in gameData.segs.vertices
	Assert(med_vertexCount(abspi) == 1);

	Assert(abspi <= MAX_SEGMENT_VERTICES);			// Make sure vertex id is not bogus.

	VmVecAdd(&gameData.segs.vertices[abspi],&gameData.segs.vertices[abspi],vofs);

	// Here you need to validate the geometry of the tSegment, which will be quite tricky.
	// You need to make sure:
	//		The tSegment is not concave.
	//		None of the sides are concave.
	ValidateSegment(sp);

}

// -------------------------------------------------------------------------------
//	Move a d_free wall in the tSegment by adding the vector *vofs to its coordinates.
//	Wall indices: 0/1/2/3/4/5 = left/top/right/bottom/back/front
void med_move_wall(tSegment *sp,int wi, vmsVector *vofs)
{
	char *vp;
	int	i;

	Assert( (wi >= 0) && (wi <= 5) );

	vp = sideToVerts[wi];
	for (i=0; i<4; i++) {
		med_move_vertex(sp,*vp,vofs);
		vp++;
	}

	ValidateSegment(sp);
}

// -------------------------------------------------------------------------------
//	Return true if one fixed point number is very close to another, else return false.
int fnear(fix f1, fix f2)
{
	return (abs(f1 - f2) <= FIX_EPSILON);
}

// -------------------------------------------------------------------------------
int vnear(vmsVector *vp1, vmsVector *vp2)
{
	return fnear(vp1->x, vp2->x) && fnear(vp1->y, vp2->y) && fnear(vp1->z, vp2->z);
}

// -------------------------------------------------------------------------------
//	Add the vertex *vp to the global list of vertices, return its index.
//	Search until a matching vertex is found (has nearly the same coordinates) or until gameData.segs.nVertices
// vertices have been looked at without a match.  If no match, add a new vertex.
int med_add_vertex(vmsVector *vp)
{
	int	v,free_index;
	int	count;					// number of used vertices found, for loops exits when count == gameData.segs.nVertices

//	set_vertexCounts();

	Assert(gameData.segs.nVertices < MAX_SEGMENT_VERTICES);

	count = 0;
	free_index = -1;
	for (v=0; (v < MAX_SEGMENT_VERTICES) && (count < gameData.segs.nVertices); v++)
		if (Vertex_active[v]) {
			count++;
			if (vnear(vp,&gameData.segs.vertices[v])) {
				return v;
			}
		} else if (free_index == -1)
			free_index = v;					// we want free_index to be the first d_free slot to add a vertex

	if (free_index == -1)
		free_index = gameData.segs.nVertices;

	while (Vertex_active[free_index] && (free_index < MAX_VERTICES))
		free_index++;

	Assert(free_index < MAX_VERTICES);

	gameData.segs.vertices[free_index] = *vp;
	Vertex_active[free_index] = 1;

	gameData.segs.nVertices++;

	if (free_index > gameData.segs.nLastVertex)
		gameData.segs.nLastVertex = free_index;

	return free_index;
}

// ------------------------------------------------------------------------------------------
//	Returns the index of a d_free tSegment.
//	Scans the gameData.segs.segments array.
int get_free_segment_number(void)
{
	int	nSegment;

	for (nSegment=0; nSegment<MAX_SEGMENTS; nSegment++)
		if (gameData.segs.segments[nSegment].nSegment == -1) {
			gameData.segs.nSegments++;
			if (nSegment > gameData.segs.nLastSegment)
				gameData.segs.nLastSegment = nSegment;
			return nSegment;
		}

	Assert(0);

	return 0;
}

// -------------------------------------------------------------------------------
//	Create a new tSegment, duplicating exactly, including vertex ids and children, the passed tSegment.
int med_create_duplicate_segment(tSegment *sp)
{
	int	nSegment;

	nSegment = get_free_segment_number();

	gameData.segs.segments[nSegment] = *sp;	

	return nSegment;
}

// -------------------------------------------------------------------------------
//	Add the vertex *vp to the global list of vertices, return its index.
//	This is the same as med_add_vertex, except that it does not search for the presence of the vertex.
int med_create_duplicate_vertex(vmsVector *vp)
{
	int	free_index;

	Assert(gameData.segs.nVertices < MAX_SEGMENT_VERTICES);

	Do_duplicate_vertex_check = 1;

	free_index = gameData.segs.nVertices;

	while (Vertex_active[free_index] && (free_index < MAX_VERTICES))
		free_index++;

	Assert(free_index < MAX_VERTICES);

	gameData.segs.vertices[free_index] = *vp;
	Vertex_active[free_index] = 1;

	gameData.segs.nVertices++;

	if (free_index > gameData.segs.nLastVertex)
		gameData.segs.nLastVertex = free_index;

	return free_index;
}


// -------------------------------------------------------------------------------
//	Set the vertex *vp at index vnum in the global list of vertices, return its index (just for compatibility).
int med_set_vertex(int vnum,vmsVector *vp)
{
	Assert(vnum < MAX_VERTICES);

	gameData.segs.vertices[vnum] = *vp;

	// Just in case this vertex wasn't active, mark it as active.
	if (!Vertex_active[vnum]) {
		Vertex_active[vnum] = 1;
		gameData.segs.nVertices++;
		if ((vnum > gameData.segs.nLastVertex) && (vnum < NEW_SEGMENT_VERTICES)) {
#if TRACE
			con_printf (CON_DEBUG,"Warning -- setting a previously unset vertex, index = %i.\n",vnum);
#endif
			gameData.segs.nLastVertex = vnum;
		}
	}

	return vnum;
}



//	----
//	A tSide is determined to be degenerate if the cross products of 3 consecutive points does not point outward.
int check_for_degenerate_side(tSegment *sp, int nSide)
{
	char			*vp = sideToVerts[nSide];
	vmsVector	vec1, vec2, cross, vec_to_center;
	vmsVector	segc, sidec;
	fix			dot;
	int			degeneracyFlag = 0;

	COMPUTE_SEGMENT_CENTER(&segc, sp);
	COMPUTE_SIDE_CENTER(&sidec, sp, nSide);
	VmVecSub(&vec_to_center, &segc, &sidec);

	//VmVecSub(&vec1, &gameData.segs.vertices[sp->verts[vp[1]]], &gameData.segs.vertices[sp->verts[vp[0]]]);
	//VmVecSub(&vec2, &gameData.segs.vertices[sp->verts[vp[2]]], &gameData.segs.vertices[sp->verts[vp[1]]]);
	//VmVecNormalize(&vec1);
	//VmVecNormalize(&vec2);
        VmVecNormalizedDir(&vec1, &gameData.segs.vertices[sp->verts[(int) vp[1]]], &gameData.segs.vertices[sp->verts[(int) vp[0]]]);
        VmVecNormalizedDir(&vec2, &gameData.segs.vertices[sp->verts[(int) vp[2]]], &gameData.segs.vertices[sp->verts[(int) vp[1]]]);
	VmVecCross(&cross, &vec1, &vec2);

	dot = VmVecDot(&vec_to_center, &cross);
	if (dot <= 0)
		degeneracyFlag |= 1;

	//VmVecSub(&vec1, &gameData.segs.vertices[sp->verts[vp[2]]], &gameData.segs.vertices[sp->verts[vp[1]]]);
	//VmVecSub(&vec2, &gameData.segs.vertices[sp->verts[vp[3]]], &gameData.segs.vertices[sp->verts[vp[2]]]);
	//VmVecNormalize(&vec1);
	//VmVecNormalize(&vec2);
        VmVecNormalizedDir(&vec1, &gameData.segs.vertices[sp->verts[(int) vp[2]]], &gameData.segs.vertices[sp->verts[(int) vp[1]]]);
        VmVecNormalizedDir(&vec2, &gameData.segs.vertices[sp->verts[(int) vp[3]]], &gameData.segs.vertices[sp->verts[(int) vp[2]]]);
	VmVecCross(&cross, &vec1, &vec2);

	dot = VmVecDot(&vec_to_center, &cross);
	if (dot <= 0)
		degeneracyFlag |= 1;

	return degeneracyFlag;

}

// -------------------------------------------------------------------------------
void create_removable_wall(tSegment *sp, int nSide, int nBaseTex)
{
	CreateWallsOnSide(sp, nSide);

	sp->sides[nSide].nBaseTex = nBaseTex;

	assign_default_uvs_to_side(sp, nSide);
	assign_light_to_side(sp, nSide);
}

//	----
//	See if a tSegment has gotten turned inside out, or something.
//	If so, set global Degenerate_segment_found and return 1, else return 0.
int check_for_degenerate_segment(tSegment *sp)
{
	vmsVector	fVec, rVec, uVec, cross;
	fix			dot;
	int			i, degeneracyFlag = 0;				// degeneracy flag for current tSegment

	extract_forward_vector_from_segment(sp, &fVec);
	extract_right_vector_from_segment(sp, &rVec);
	extract_up_vector_from_segment(sp, &uVec);

	VmVecNormalize(&fVec);
	VmVecNormalize(&rVec);
	VmVecNormalize(&uVec);

	VmVecCross(&cross, &fVec, &rVec);
	dot = VmVecDot(&cross, &uVec);

	if (dot > 0)
		degeneracyFlag = 0;
	else {
#if TRACE
		con_printf (CON_DEBUG, "tSegment #%i is degenerate due to cross product check.\n", SEG_IDX (sp));
#endif
		degeneracyFlag = 1;
	}

	//	Now, see if degenerate because of any tSide.
	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++)
		degeneracyFlag |= check_for_degenerate_side(sp, i);

	Degenerate_segment_found |= degeneracyFlag;

	return degeneracyFlag;

}

#if 0

// ---------------------------------------------------------------------------------------------
//	Orthogonalize matrix smat, returning result in rmat.
//	Does not modify smat.
//	Uses Gram-Schmidt process.
//	See page 172 of Strang, Gilbert, Linear Algebra and its Applications
//	Matt -- This routine should be moved to the vector matrix library.
//	It IS legal for smat == rmat.
//	We should also have the functions:
//		mat_a = mat_b * scalar;				// we now have mat_a = mat_a * scalar;
//		mat_a = mat_b + mat_c * scalar;	// or maybe not, maybe this is not primitive
void make_orthogonal(vmsMatrix *rmat,vmsMatrix *smat)
{
	vmsMatrix		tmat;
	vmsVector		tvec1,tvec2;
	double				dot;

	// Copy source matrix to work area.
	tmat = *smat;

	// Normalize the three rows of the matrix tmat.
	VmVecNormalize(&tmat.xrow);
	VmVecNormalize(&tmat.yrow);
	VmVecNormalize(&tmat.zrow);

	//	Now, compute the first vector.
	// This is very easy -- just copy the (normalized) source vector.
	rmat->zrow = tmat.zrow;

	// Now, compute the second vector.
	// From page 172 of Strang, we use the equation:
	//		b' = b - [transpose(q1) * b] * q1
	//	where:	b  = the second row of tmat
	//				q1 = the first row of rmat
	//				b' = the second row of rmat

	// Compute: transpose(q1) * b
	dot = VmVecDotProd(&rmat->zrow,&tmat.yrow);

	// Compute: b - dot * q1
	rmat->yrow.x = tmat.yrow.x - FixMul(dot,rmat->zrow.x);
	rmat->yrow.y = tmat.yrow.y - FixMul(dot,rmat->zrow.y);
	rmat->yrow.z = tmat.yrow.z - FixMul(dot,rmat->zrow.z);

	// Now, compute the third vector.
	// From page 173 of Strang, we use the equation:
	//		c' = c - (q1*c)*q1 - (q2*c)*q2
	//	where:	c  = the third row of tmat
	//				q1 = the first row of rmat
	//				q2 = the second row of rmat
	//				c' = the third row of rmat

	// Compute: q1*c
	dot = VmVecDotProd(&rmat->zrow,&tmat.xrow);

	tvec1.x = FixMul(dot,rmat->zrow.x);
	tvec1.y = FixMul(dot,rmat->zrow.y);
	tvec1.z = FixMul(dot,rmat->zrow.z);

	// Compute: q2*c
	dot = VmVecDotProd(&rmat->yrow,&tmat.xrow);

	tvec2.x = FixMul(dot,rmat->yrow.x);
	tvec2.y = FixMul(dot,rmat->yrow.y);
	tvec2.z = FixMul(dot,rmat->yrow.z);

	VmVecSub(&rmat->xrow,VmVecSub(&rmat->xrow,&tmat.xrow,&tvec1),&tvec2);
}

#endif

// ------------------------------------------------------------------------------------------
// Given a tSegment, extract the rotation matrix which defines it.
// Do this by extracting the forward, right, up vectors and then making them orthogonal.
// In the process of making the vectors orthogonal, favor them in the order forward, up, right.
// This means that the forward vector will remain unchanged.
void med_extract_matrix_from_segment(tSegment *sp,vmsMatrix *rotmat)
{
	vmsVector	forwardvec,upvec;

	extract_forward_vector_from_segment(sp,&forwardvec);
	extract_up_vector_from_segment(sp,&upvec);

	if (((forwardvec.x == 0) && (forwardvec.y == 0) && (forwardvec.z == 0)) || ((upvec.x == 0) && (upvec.y == 0) && (upvec.z == 0))) {
#if TRACE
		con_printf (CON_DEBUG, "Trapped null vector in med_extract_matrix_from_segment, returning identity matrix.\n");
#endif
		*rotmat = vmdIdentityMatrix;
		return;
	}


	VmVector2Matrix(rotmat,&forwardvec,&upvec,NULL);

#if 0
	vmsMatrix	rm;

	extract_forward_vector_from_segment(sp,&rm.zrow);
	extract_right_vector_from_segment(sp,&rm.xrow);
	extract_up_vector_from_segment(sp,&rm.yrow);

	VmVecNormalize(&rm.xrow);
	VmVecNormalize(&rm.yrow);
	VmVecNormalize(&rm.zrow);

	make_orthogonal(rotmat,&rm);

	VmVecNormalize(&rotmat->xrow);
	VmVecNormalize(&rotmat->yrow);
	VmVecNormalize(&rotmat->zrow);

// *rotmat = rm; // include this line (and remove the call to make_orthogonal) if you don't want the matrix orthogonalized
#endif

}

// ------------------------------------------------------------------------------------------
//	Given a rotation matrix *rotmat which describes the orientation of a tSegment
//	and a tSide destside, return the rotation matrix which describes the orientation for the tSide.
void	set_matrix_based_on_side(vmsMatrix *rotmat,int destside)
{
        vmsAngVec      rotvec,*tmpvec;
        vmsMatrix      r1,rtemp;

	switch (destside) {
		case WLEFT:
                        tmpvec=VmAngVecMake(&rotvec,0,0,-16384);
			VmAngles2Matrix(&r1,&rotvec);
			VmMatMul(&rtemp,rotmat,&r1);
			*rotmat = rtemp;
			break;

		case WTOP:
                        tmpvec=VmAngVecMake(&rotvec,-16384,0,0);
			VmAngles2Matrix(&r1,&rotvec);
			VmMatMul(&rtemp,rotmat,&r1);
			*rotmat = rtemp;
			break;

		case WRIGHT:
                        tmpvec=VmAngVecMake(&rotvec,0,0,16384);
			VmAngles2Matrix(&r1,&rotvec);
			VmMatMul(&rtemp,rotmat,&r1);
			*rotmat = rtemp;
			break;

		case WBOTTOM:
                        tmpvec=VmAngVecMake(&rotvec,+16384,-32768,0);        // bank was -32768, but I think that was an erroneous compensation
			VmAngles2Matrix(&r1,&rotvec);
			VmMatMul(&rtemp,rotmat,&r1);
			*rotmat = rtemp;
			break;

		case WFRONT:
                        tmpvec=VmAngVecMake(&rotvec,0,0,-32768);
			VmAngles2Matrix(&r1,&rotvec);
			VmMatMul(&rtemp,rotmat,&r1);
			*rotmat = rtemp;
			break;

		case WBACK:
			break;
	}

}

//	-------------------------------------------------------------------------------------
void change_vertex_occurrences(int dest, int src)
{
	int	g,s,v;

	// Fix vertices in groups
	for (g=0;g<num_groups;g++) 
		for (v=0; v<GroupList[g].num_vertices; v++)
			if (GroupList[g].vertices[v] == src)
				GroupList[g].vertices[v] = dest;

	// now scan all segments, changing occurrences of src to dest
	for (s=0; s<=gameData.segs.nLastSegment; s++)
		if (gameData.segs.segments[s].nSegment != -1)
			for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
				if (gameData.segs.segments[s].verts[v] == src)
					gameData.segs.segments[s].verts[v] = dest;
}

// --------------------------------------------------------------------------------------------------
void compress_vertices(void)
{
	int		hole,vert;

	if (gameData.segs.nLastVertex == gameData.segs.nVertices - 1)
		return;

	vert = gameData.segs.nLastVertex;	//MAX_SEGMENT_VERTICES-1;

	for (hole=0; hole < vert; hole++)
		if (!Vertex_active[hole]) {
			// found an unused vertex which is a hole if a used vertex follows (not necessarily immediately) it.
			for ( ; (vert>hole) && (!Vertex_active[vert]); vert--)
				;

			if (vert > hole) {

				// Ok, hole is the index of a hole, vert is the index of a vertex which follows it.
				// Copy vert into hole, update pointers to it.
				gameData.segs.vertices[hole] = gameData.segs.vertices[vert];
				
				change_vertex_occurrences(hole, vert);

				vert--;
			}
		}

	gameData.segs.nLastVertex = gameData.segs.nVertices-1;
}

// --------------------------------------------------------------------------------------------------
void compress_segments(void)
{
	int		hole,seg;

	if (gameData.segs.nLastSegment == gameData.segs.nSegments - 1)
		return;

	seg = gameData.segs.nLastSegment;

	for (hole=0; hole < seg; hole++)
		if (gameData.segs.segments[hole].nSegment == -1) {
			// found an unused tSegment which is a hole if a used tSegment follows (not necessarily immediately) it.
			for ( ; (seg>hole) && (gameData.segs.segments[seg].nSegment == -1); seg--)
				;

			if (seg > hole) {
				int		f,g,l,s,t,w;
				tSegment	*sp;
				int nObject;

				// Ok, hole is the index of a hole, seg is the index of a tSegment which follows it.
				// Copy seg into hole, update pointers to it, update Cursegp, Markedsegp if necessary.
				gameData.segs.segments[hole] = gameData.segs.segments[seg];
				gameData.segs.segments[seg].nSegment = -1;

				if (Cursegp == &gameData.segs.segments[seg])
					Cursegp = &gameData.segs.segments[hole];

				if (Markedsegp == &gameData.segs.segments[seg])
					Markedsegp = &gameData.segs.segments[hole];

				// Fix segments in groups
				for (g=0;g<num_groups;g++) 
					for (s=0; s<GroupList[g].num_segments; s++)
						if (GroupList[g].segments[s] == seg)
							GroupList[g].segments[s] = hole;

				// Fix walls
				for (w=0;w<gameData.walls.nWalls;w++)
					if (gameData.walls.walls[w].nSegment == seg)
						gameData.walls.walls[w].nSegment = hole;

				// Fix fuelcenters, robotcens, and triggers... added 2/1/95 -Yuan
				for (f=0;f<gameData.matCens.nFuelCenters;f++)
					if (gameData.matCens.fuelCenters[f].nSegment == seg)
						gameData.matCens.fuelCenters[f].nSegment = hole;

				for (f=0;f<gameData.matCens.nMatCens;f++)
					if (gameData.matCens.botGens[f].nSegment == seg)
						gameData.matCens.botGens[f].nSegment = hole;

				for (t=0;t<gameData.trigs.nTriggers;t++)
					for (l=0;l<gameData.trigs.triggers[t].nLinks;l++)
						if (gameData.trigs.triggers[t].seg[l] == seg)
							gameData.trigs.triggers[t].seg[l] = hole;

				sp = &gameData.segs.segments[hole];
				for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
					if (IS_CHILD(sp->children[s])) {
						tSegment *csegp;
						csegp = &gameData.segs.segments[sp->children[s]];

						// Find out on what tSide the tSegment connection to the former seg is on in *csegp.
						for (t=0; t<MAX_SIDES_PER_SEGMENT; t++) {
							if (csegp->children[t] == seg) {
								csegp->children[t] = hole;					// It used to be connected to seg, so make it connected to hole
							}
						}	// end for t
					}	// end if
				}	// end for s

				//Update tObject tSegment pointers
				for (nObject = sp->objects; nObject != -1; nObject = gameData.objs.objects[nObject].next) {
					Assert(gameData.objs.objects[nObject].nSegment == seg);
					gameData.objs.objects[nObject].nSegment = hole;
				}

				seg--;

			}	// end if (seg > hole)
		}	// end if

	gameData.segs.nLastSegment = gameData.segs.nSegments-1;
	med_create_new_segment_from_cursegp();

}


// -------------------------------------------------------------------------------
//	Combine duplicate vertices.
//	If two vertices have the same coordinates, within some small tolerance, then assign
//	the same vertex number to the two vertices, freeing up one of the vertices.
void med_combine_duplicate_vertices(byte *vlp)
{
	int	v,w;

	for (v=0; v<gameData.segs.nLastVertex; v++)		// Note: ok to do to <, rather than <= because w for loop starts at v+1
		if (vlp[v]) {
			vmsVector *vvp = &gameData.segs.vertices[v];
			for (w=v+1; w<=gameData.segs.nLastVertex; w++)
				if (vlp[w]) {	//	used to be Vertex_active[w]
					if (vnear(vvp, &gameData.segs.vertices[w])) {
						change_vertex_occurrences(v, w);
					}
				}
		}

}

// ------------------------------------------------------------------------------
//	Compress mine at gameData.segs.segments and gameData.segs.vertices by squeezing out all holes.
//	If no holes (ie, an unused tSegment followed by a used tSegment), then no action.
//	If Cursegp or Markedsegp is a tSegment which gets moved to fill in a hole, then
//	they are properly updated.
void med_compress_mine(void)
{
	if (Do_duplicate_vertex_check) {
		med_combine_duplicate_vertices(Vertex_active);
		Do_duplicate_vertex_check = 0;
	}

	compress_segments();
	compress_vertices();
	set_vertexCounts();

	//--repair-- create_local_segment_data();

	//	This is necessary becuase a tSegment search (due to click in 3d window) uses the previous frame's
	//	tSegment information, which could get changed by this.
	UpdateFlags = UF_WORLD_CHANGED;
}


// ------------------------------------------------------------------------------------------
//	Copy texture map ids for each face in sseg to dseg.
void copy_tmap_ids(tSegment *dseg, tSegment *sseg)
{
	int	s;

	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
		dseg->sides[s].nBaseTex = sseg->sides[s].nBaseTex;
		dseg->sides[s].nOvlTex = 0;
	}
}

// ------------------------------------------------------------------------------------------
//	Attach a tSegment with a rotated orientation.
// Return value:
//  0 = successful attach
//  1 = No room in gameData.segs.segments[].
//  2 = No room in gameData.segs.vertices[].
//  3 = newside != WFRONT -- for now, the new tSegment must be attached at its (own) front tSide
//	 4 = already a face attached on destseg:destside
int med_attach_segment_rotated(tSegment *destseg, tSegment *newseg, int destside, int newside,vmsMatrix *attmat)
{
	char			*dvp;
	tSegment		*nsp;
	tSegment2	*nsp2;
	int			tSide,v;
	vmsMatrix	rotmat,rotmat1,rotmat2,rotmat3,rotmat4;
	vmsVector	vr,vc,tvs[4],xlate_vec;
	int			nSegment;
	vmsVector	forvec,upvec;

	// Return if already a face attached on this tSide.
	if (IS_CHILD(destseg->children[destside]))
		return 4;

	nSegment = get_free_segment_number();

	forvec = attmat->fVec;
	upvec = attmat->uVec;

	//	We are pretty confident we can add the tSegment.
	nsp = &gameData.segs.segments[nSegment];
	nsp2 = &gameData.segs.segment2s[nSegment];

	nsp->nSegment = nSegment;
	nsp->objects = -1;
	nsp2->nMatCen = -1;

	// Copy group value.
	nsp->group = destseg->group;

	// Add tSegment to proper group list.
	if (nsp->group > -1)
		add_segment_to_group(nSEG_IDX (sp), nsp->group);

	// Copy the texture map ids.
	copy_tmap_ids(nsp,newseg);

	// clear all connections
	for (tSide=0; tSide<MAX_SIDES_PER_SEGMENT; tSide++) {
		nsp->children[tSide] = -1;
		nsp->sides[tSide].nWall = NO_WALL;	
	}

	// Form the connection
	destseg->children[destside] = nSegment;
//	destseg->sides[destside].renderFlag = 0;
	nsp->children[newside] = destSEG_IDX (seg);

	// Copy vertex indices of the four vertices forming the joint
	dvp = sideToVerts[destside];

	// Set the vertex indices for the four vertices forming the front of the new tSide
	for (v=0; v<4; v++)
                nsp->verts[v] = destseg->verts[(int) dvp[v]];

	// The other 4 vertices must be created.
	// Their coordinates are determined by the 4 welded vertices and the vector from front
	// to back of the original *newseg.

	// Do lots of hideous matrix stuff, about 3/4 of which could probably be simplified out.
	med_extract_matrix_from_segment(destseg,&rotmat);		// get orientation matrix for destseg (orthogonal rotation matrix)
	set_matrix_based_on_side(&rotmat,destside);
	VmVector2Matrix(&rotmat1,&forvec,&upvec,NULL);
	VmMatMul(&rotmat4,&rotmat,&rotmat1);			// this is the desired orientation of the new tSegment
	med_extract_matrix_from_segment(newseg,&rotmat3);		// this is the current orientation of the new tSegment
	VmTransposeMatrix(&rotmat3);								// get the inverse of the current orientation matrix
	VmMatMul(&rotmat2,&rotmat4,&rotmat3);			// now rotmat2 takes the current tSegment to the desired orientation

	// Warning -- look at this line!
	VmTransposeMatrix(&rotmat2);	// added 12:33 pm, 10/01/93

	// Compute and rotate the center point of the attaching face.
	COMPUTE_SIDE_CENTER(&vc,newseg,newside);
	VmVecRotate(&vr,&vc,&rotmat2);

	// Now rotate the d_free vertices in the tSegment
	for (v=0; v<4; v++)
		VmVecRotate(&tvs[v],&gameData.segs.vertices[newseg->verts[v+4]],&rotmat2);

	// Now translate the new tSegment so that the center point of the attaching faces are the same.
	COMPUTE_SIDE_CENTER(&vc,destseg,destside);
	VmVecSub(&xlate_vec,&vc,&vr);

	// Create and add the 4 new vertices.
	for (v=0; v<4; v++) {
		VmVecInc(&tvs[v],&xlate_vec);
		nsp->verts[v+4] = med_add_vertex(&tvs[v]);
	}

	set_vertexCounts();

	// Now all the vertices are in place.  Create the faces.
	ValidateSegment(nsp);

	//	Say to not render at the joint.
//	destseg->sides[destside].renderFlag = 0;
//	nsp->sides[newside].renderFlag = 0;

	Cursegp = nsp;

	return	0;
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

// ------------------------------------------------------------------------------------------
void scale_free_vertices(tSegment *sp,vmsVector *vp,fix scale_factor,int min_side,int max_side)
{
	int	i;
	char	*verts;

	verts = sideToVerts[min_side];

	for (i=0; i<4; i++)
                if (is_free_vertex(sp->verts[(int) verts[i]])) {
                        gameData.segs.vertices[sp->verts[(int) verts[i]]].x = FixMul(vp->x,scale_factor)/2;
                        gameData.segs.vertices[sp->verts[(int) verts[i]]].y = FixMul(vp->y,scale_factor)/2;
                        gameData.segs.vertices[sp->verts[(int) verts[i]]].z = FixMul(vp->z,scale_factor)/2;
		}

	verts = sideToVerts[max_side];

	for (i=0; i<4; i++)
                if (is_free_vertex(sp->verts[(int) verts[i]])) {
                        gameData.segs.vertices[sp->verts[(int) verts[i]]].x = FixMul(vp->x,scale_factor)/2;
                        gameData.segs.vertices[sp->verts[(int) verts[i]]].y = FixMul(vp->y,scale_factor)/2;
                        gameData.segs.vertices[sp->verts[(int) verts[i]]].z = FixMul(vp->z,scale_factor)/2;
		}
}


// ------------------------------------------------------------------------------------------
// Attach tSide newside of newseg to tSide destside of destseg.
// Copies *newseg into global array gameData.segs.segments, increments gameData.segs.nSegments.
// Forms a weld between the two segments by making the new tSegment fit to the old tSegment.
// Updates number of faces per tSide if necessitated by new vertex coordinates.
//	Updates Cursegp.
// Return value:
//  0 = successful attach
//  1 = No room in gameData.segs.segments[].
//  2 = No room in gameData.segs.vertices[].
//  3 = newside != WFRONT -- for now, the new tSegment must be attached at its (own) front tSide
//	 4 = already a face attached on tSide newside
int med_attach_segment(tSegment *destseg, tSegment *newseg, int destside, int newside)
{
	int		rval;
	tSegment	*ocursegp = Cursegp;

	vmsAngVec	tang = {0,0,0};
	vmsMatrix	rotmat;

	VmAngles2Matrix(&rotmat,&tang);
	rval = med_attach_segment_rotated(destseg,newseg,destside,newside,&rotmat);
	med_propagate_tmaps_to_segments(ocursegp,Cursegp,0);
	med_propagate_tmaps_to_back_side(Cursegp, sideOpposite[newside],0);
	copy_uvs_seg_to_seg(&New_segment,Cursegp);

	return rval;
}

// -------------------------------------------------------------------------------
//	Delete a vertex, sort of.
//	Decrement the vertex count.  If the count goes to 0, then the vertex is d_free (has been deleted).
void delete_vertex(short v)
{
	Assert(v < MAX_VERTICES);			// abort if vertex is not in array gameData.segs.vertices
	Assert(Vertex_active[v] >= 1);	// abort if trying to delete a non-existent vertex

	Vertex_active[v]--;
}

// -------------------------------------------------------------------------------
//	Update gameData.segs.nVertices.
//	This routine should be called by anyone who calls delete_vertex.  It could be called in delete_vertex,
//	but then it would be called much more often than necessary, and it is a slow routine.
void update_num_vertices(void)
{
	int	v;

	// Now count the number of vertices.
	gameData.segs.nVertices = 0;
	for (v=0; v<=gameData.segs.nLastVertex; v++)
		if (Vertex_active[v])
			gameData.segs.nVertices++;
}

// -------------------------------------------------------------------------------
//	Set Vertex_active to number of occurrences of each vertex.
//	Set gameData.segs.nVertices.
void set_vertexCounts(void)
{
	int	s,v;

	gameData.segs.nVertices = 0;

	for (v=0; v<=gameData.segs.nLastVertex; v++)
		Vertex_active[v] = 0;

	// Count number of occurrences of each vertex.
	for (s=0; s<=gameData.segs.nLastSegment; s++)
		if (gameData.segs.segments[s].nSegment != -1)
			for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++) {
				if (!Vertex_active[gameData.segs.segments[s].verts[v]])
					gameData.segs.nVertices++;
				Vertex_active[gameData.segs.segments[s].verts[v]]++;
			}
}

// -------------------------------------------------------------------------------
//	Delete all vertices in tSegment *sp from the vertex list if they are not contained in another tSegment.
//	This is kind of a dangerous routine.  It modifies the global array Vertex_active, using the field as
//	a count.
void delete_vertices_in_segment(tSegment *sp)
{
	int	v;

//	init_vertices();

	set_vertexCounts();

	// Subtract one count for each appearance of vertex in deleted tSegment
	for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
		delete_vertex(sp->verts[v]);

	update_num_vertices();
}

extern void ValidateSegmentSide(tSegment *sp, int nSide);

// -------------------------------------------------------------------------------
//	Delete tSegment *sp in gameData.segs.segments array.
// Return value:
//		0	successfully deleted.
//		1	unable to delete.
int med_delete_segment(tSegment *sp)
{
	int		s,tSide,nSegment;
	int 		nObject;

	nSegment = SEG_IDX (sp);

	// Cannot delete tSegment if only tSegment.
	if (gameData.segs.nSegments == 1)
		return 1;

	// Don't try to delete if tSegment doesn't exist.
	if (sp->nSegment == -1) {
#if TRACE
		con_printf (CON_DEBUG,"Tried to delete a non-existent tSegment (nSegment == -1)\n");
#endif
		return 1;
	}

	// Delete its refueling center if it has one
	FuelCenDelete(sp);

	delete_vertices_in_segment(sp);

	gameData.segs.nSegments--;

	// If deleted tSegment has walls on any tSide, wipe out the wall.
	for (tSide=0; tSide < MAX_SIDES_PER_SEGMENT; tSide++)
		if (IS_WALL (WallNumP (sp, tSide))) 
			wall_remove_side(sp, tSide);

	// Find out what this tSegment was connected to and break those connections at the other end.
	for (tSide=0; tSide < MAX_SIDES_PER_SEGMENT; tSide++)
		if (IS_CHILD(sp->children[tSide])) {
			tSegment	*csp;									// the connecting tSegment
			int		s;

			csp = gameData.segs.segments + sp->children[tSide];
			for (s=0; s<MAX_SIDES_PER_SEGMENT; s++)
				if (csp->children[s] == nSegment) {
					csp->children[s] = -1;				// this is the tSide of connection, break it
					ValidateSegmentSide(csp,s);					// we have converted a connection to a tSide so validate the tSegment
					med_propagate_tmaps_to_back_side(csp,s,0);
				}
			Cursegp = csp;
			med_create_new_segment_from_cursegp();
			copy_uvs_seg_to_seg(&New_segment,Cursegp);
		}

	sp->nSegment = -1;										// Mark tSegment as inactive.

	// If deleted tSegment = marked tSegment, then say there is no marked tSegment
	if (sp == Markedsegp)
		Markedsegp = 0;
	
	//	If deleted tSegment = a Group tSegment ptr, then wipe it out.
	for (s=0;s<num_groups;s++) 
		if (sp == Groupsegp[s]) 
			Groupsegp[s] = 0;

	// If deleted tSegment = group tSegment, wipe it off the group list.
	if (sp->group > -1) 
			delete_segment_from_group(SEG_IDX (sp), sp->group);

	// If we deleted something which was not connected to anything, must now select a new current tSegment.
	if (Cursegp == sp)
		for (s=0; s<MAX_SEGMENTS; s++)
			if ((gameData.segs.segments[s].nSegment != -1) && (s!=nSegment) ) {
				Cursegp = &gameData.segs.segments[s];
				med_create_new_segment_from_cursegp();
		   	break;
			}

	// If deleted tSegment contains gameData.objs.objects, wipe out all gameData.objs.objects
	if (sp->objects != -1) 	{
//		if (nObject == gameData.objs.objects[nObject].next) {
//			gameData.objs.objects[nObject].next = -1;
//		}
		for (nObject=sp->objects;nObject!=-1;nObject=gameData.objs.objects[nObject].next) 	{

			//if an tObject is in the seg, delete it
			//if the tObject is the tPlayer, move to new curseg

			if (nObject == (OBJ_IDX (gameData.objs.console)))	{
				COMPUTE_SEGMENT_CENTER(&gameData.objs.console->position.vPos,Cursegp);
				RelinkObject(nObject,CurSEG_IDX (segp));
			} else
				ReleaseObject(nObject);
		}
	}

	// Make sure everything deleted ok...
	Assert( sp->objects==-1 );

	// If we are leaving many holes in gameData.segs.segments or gameData.segs.vertices, then compress mine, because it is inefficient to be that way
//	if ((gameData.segs.nLastSegment > gameData.segs.nSegments+4) || (gameData.segs.nLastVertex > gameData.segs.nVertices+4*8))
//		med_compress_mine();

	return 0;
}

// ------------------------------------------------------------------------------------------
//	Copy texture maps from sseg to dseg
void copy_tmaps_to_segment(tSegment *dseg, tSegment *sseg)
{
	int	s;

	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
		dseg->sides[s].nType = sseg->sides[s].nType;
		dseg->sides[s].nBaseTex = sseg->sides[s].nBaseTex;
		dseg->sides[s].nOvlTex = sseg->sides[s].nOvlTex;
	}

}

// ------------------------------------------------------------------------------------------
// Rotate the tSegment *seg by the pitch, bank, heading defined by *rot, destructively
// modifying its four d_free vertices in the global array gameData.segs.vertices.
// It is illegal to rotate a tSegment which has connectivity != 1.
// Pitch, bank, heading are about the point which is the average of the four points
// forming the tSide of connection.
// Return value:
//  0 = successful rotation
//  1 = Connectivity makes rotation illegal (connected to 0 or 2+ segments)
//  2 = Rotation causes degeneracy, such as self-intersecting tSegment.
//	 3 = Unable to rotate because not connected to exactly 1 tSegment.
int med_rotate_segment(tSegment *seg, vmsMatrix *rotmat)
{
	tSegment	*destseg;
        int             newside=0,destside,s;
	int		count;
	int		back_side,side_tmaps[MAX_SIDES_PER_SEGMENT];

	// Find tSide of attachment
	count = 0;
	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++)
		if (IS_CHILD(seg->children[s])) {
			count++;
			newside = s;
		}

	// Return if passed in tSegment is connected to other than 1 tSegment.
	if (count != 1)
		return 3;

	destseg = &gameData.segs.segments[seg->children[newside]];

	destside = 0;
	while ((destseg->children[destside] != SEG_IDX (seg)) && (destside < MAX_SIDES_PER_SEGMENT))
		destside++;
		
	// Before deleting the tSegment, copy its texture maps to New_segment
	copy_tmaps_to_segment(&New_segment,seg);

	if (med_delete_segment(seg)) {
#if TRACE
		con_printf (CON_DEBUG,"Error in rotation: Unable to delete tSegment %i\n",SEG_IDX (seg));
#endif
		}
	if (Curside == WFRONT)
		Curside = WBACK;

	med_attach_segment_rotated(destseg,&New_segment,destside,AttachSide,rotmat);

	//	Save nBaseTex on each tSide to restore after call to med_propagate_tmaps_to_segments and _back_side
	//	which will change the tmap nums.
	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++)
		side_tmaps[s] = seg->sides[s].nBaseTex;

	back_side = sideOpposite[FindConnectedSide(destseg, seg)];

	med_propagate_tmaps_to_segments(destseg, seg,0);
	med_propagate_tmaps_to_back_side(seg, back_side,0);

	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++)
		if (s != back_side)
			seg->sides[s].nBaseTex = side_tmaps[s];

	return	0;
}

// ----------------------------------------------------------------------------------------
int med_rotate_segment_ang(tSegment *seg, vmsAngVec *ang)
{
	vmsMatrix	rotmat;

	return med_rotate_segment(seg,VmAngles2Matrix(&rotmat,ang);
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

// ----------------------------------------------------------------------------
//	Compute the sum of the distances between the four pairs of points.
//	The connections are:
//		firstv1 : 0		(firstv1+1)%4 : 1		(firstv1+2)%4 : 2		(firstv1+3)%4 : 3
fix seg_seg_vertexDistsum(tSegment *seg1, int side1, tSegment *seg2, int side2, int firstv1)
{
	fix	distsum;
	int	secondv;

	distsum = 0;
	for (secondv=0; secondv<4; secondv++) {
		int			firstv;

		firstv = (4-secondv + (3 - firstv1)) % 4;
		distsum += VmVecDist(&gameData.segs.vertices[seg1->verts[sideToVerts[side1][firstv]]],&gameData.segs.vertices[seg2->verts[sideToVerts[side2][secondv]]]);
	}

	return distsum;

}

// ----------------------------------------------------------------------------
//	Determine how to connect two segments together with the least amount of twisting.
//	Returns vertex index in 0..3 on first tSegment.  Assumed ordering of vertices
//	on second tSegment is 0,1,2,3.
//	So, if return value is 2, connect 2:0 3:1 0:2 1:3.
//	Theory:
//		We select an ordering of vertices for connection.  For the first pair of vertices to be connected,
//		compute the vector.  For the three remaining pairs of vertices, compute the vectors from one vertex
//		to the other.  Compute the dot products of these vectors with the original vector.  Add them up.
//		The close we are to 3, the better fit we have.  Reason:  The largest value for the dot product is
//		1.0, and this occurs for a parallel set of vectors.
int get_index_of_best_fit(tSegment *seg1, int side1, tSegment *seg2, int side2)
{
	int	firstv;
	fix	minDistance;
	int	best_index=0;

	minDistance = F1_0*30000;

	for (firstv=0; firstv<4; firstv++) {
		fix t;
		t = seg_seg_vertexDistsum(seg1, side1, seg2, side2, firstv);
		if (t <= minDistance) {
			minDistance = t;
			best_index = firstv;
		}
	}

	return best_index;

}


#define MAX_VALIDATIONS 50

// ----------------------------------------------------------------------------
//	Remap uv coordinates in all sides in tSegment *sp which have a vertex in vp[4].
//	vp contains absolute vertex indices.
void remap_side_uvs(tSegment *sp,int *vp)
{
	int	s,i,v;

	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
		for (v=0; v<4; v++)
			for (i=0; i<4; i++)												// scan each vertex in vp[4]
				if (sideToVerts[s][v] == vp[i]) {
					assign_default_uvs_to_side(sp,s);					// Side s needs to be remapped
					goto next_side;
				}
next_side: ;
	}
}

// ----------------------------------------------------------------------------
//	Assign default uv coordinates to Curside.
void assign_default_uvs_to_curside(void)
{
	assign_default_uvs_to_side(Cursegp, Curside);
}

// ----------------------------------------------------------------------------
//	Assign default uv coordinates to all sides in Curside.
void assign_default_uvs_to_curseg(void)
{
	int	s;

	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++)
		assign_default_uvs_to_side(Cursegp,s);					// Side s needs to be remapped
}

// ----------------------------------------------------------------------------
//	Modify seg2 to share side2 with seg1:side1.  This forms a connection between
//	two segments without creating a new tSegment.  It modifies seg2 by sharing
//	vertices from seg1.  seg1 is not modified.  Four vertices from seg2 are
//	deleted.
//	Return code:
//		0			joint formed
//		1			-- no, this is legal! -- unable to form joint because one or more vertices of side2 is not d_free
//		2			unable to form joint because side1 is already used
int med_form_joint(tSegment *seg1, int side1, tSegment *seg2, int side2)
{
	char		*vp1,*vp2;
	int		bfi,v,s,sv,s1,nv;
	int		lost_vertices[4],remap_vertices[4];
	int		validation_list[MAX_VALIDATIONS];

	//	Make sure that neither tSide is connected.
	if (IS_CHILD(seg1->children[side1]) || IS_CHILD(seg2->children[side2]))
		return 2;

	// Make sure there is no wall there 
	if (IS_WALL (WallNumP (seg1, side1)) || IS_WALL (WallNumP (seg2, side2)))
		return 2;

	//	We can form the joint.  Find the best orientation of vertices.
	bfi = get_index_of_best_fit(seg1, side1, seg2, side2);

	vp1 = sideToVerts[side1];
	vp2 = sideToVerts[side2];

	//	Make a copy of the list of vertices in seg2 which will be deleted and set the
	//	associated vertex number, so that all occurrences of the vertices can be replaced.
	for (v=0; v<4; v++)
                lost_vertices[v] = seg2->verts[(int) vp2[v]];

	//	Now, for each vertex in lost_vertices, determine which vertex it maps to.
	for (v=0; v<4; v++)
                remap_vertices[3 - ((v + bfi) % 4)] = seg1->verts[(int) vp1[v]];

	// Now, in all segments, replace all occurrences of vertices in lost_vertices with remap_vertices

	// Put the one tSegment we know are being modified into the validation list.
	// Note: seg1 does not require a full validation, only a validation of the affected tSide.  Its vertices do not move.
	nv = 1;
	validation_list[0] = SEG_IDX (seg2);

	for (v=0; v<4; v++)
		for (s=0; s<=gameData.segs.nLastSegment; s++)
			if (gameData.segs.segments[s].nSegment != -1)
				for (sv=0; sv<MAX_VERTICES_PER_SEGMENT; sv++)
					if (gameData.segs.segments[s].verts[sv] == lost_vertices[v]) {
						gameData.segs.segments[s].verts[sv] = remap_vertices[v];
						// Add tSegment to list of segments to be validated.
						for (s1=0; s1<nv; s1++)
							if (validation_list[s1] == s)
								break;
						if (s1 == nv)
							validation_list[nv++] = s;
						Assert(nv < MAX_VALIDATIONS);
					}

	//	Form new connections.
	seg1->children[side1] = seg2 - gameData.segs.segments;
	seg2->children[side2] = SEG_IDX (seg1);

	// validate all segments
	ValidateSegmentSide(seg1,side1);
	for (s=0; s<nv; s++) {
		ValidateSegment(&gameData.segs.segments[validation_list[s]]);
		remap_side_uvs(&gameData.segs.segments[validation_list[s]],remap_vertices);	// remap uv coordinates on sides which were reshaped (ie, have a vertex in lost_vertices)
		warn_if_concave_segment(&gameData.segs.segments[validation_list[s]]);
	}

	set_vertexCounts();

	//	Make sure connection is open, ie renderable.
//	seg1->sides[side1].renderFlag = 0;
//	seg2->sides[side2].renderFlag = 0;

//--// debug -- check all segments, make sure if a children[s] == -1, then tSide[s].num_faces != 0
//--{
//--int seg,tSide;
//--for (seg=0; seg<MAX_SEGMENTS; seg++)
//--	if (gameData.segs.segments[seg].nSegment != -1)
//--		for (tSide=0; tSide<MAX_SIDES_PER_SEGMENT; tSide++)
//--			if (gameData.segs.segments[seg].children[tSide] == -1) {
//--				if (gameData.segs.segments[seg].sides[tSide].num_faces == 0) {
//--					Int3();
//--				}
//--			} else if (gameData.segs.segments[seg].sides[tSide].num_faces != 0) {
//--				Int3();
//--			}
//--}

	return 0;
}

// ----------------------------------------------------------------------------
//	Create a new tSegment and use it to form a bridge between two existing segments.
//	Specify two tSegment:tSide pairs.  If either tSegment:tSide is not open (ie, tSegment->children[tSide] != -1)
//	then it is not legal to form the brider.
//	Return:
//		0	bridge tSegment formed
//		1	unable to form bridge because one (or both) of the sides is not open.
//	Note that no new vertices are created by this process.
int med_form_bridge_segment(tSegment *seg1, int side1, tSegment *seg2, int side2)
{
	tSegment		*bs;
	char			*sv;
	int			v,bfi,i;

	if (IS_CHILD(seg1->children[side1]) || IS_CHILD(seg2->children[side2]))
		return 1;

	bs = &gameData.segs.segments[get_free_segment_number()];

	bs->nSegment = SEG_IDX (bs);
	bs->objects = -1;

	// Copy vertices from seg2 into last 4 vertices of bridge tSegment.
	sv = sideToVerts[side2];
	for (v=0; v<4; v++)
                bs->verts[(3-v)+4] = seg2->verts[(int) sv[v]];

	// Copy vertices from seg1 into first 4 vertices of bridge tSegment.
	bfi = get_index_of_best_fit(seg1, side1, seg2, side2);

	sv = sideToVerts[side1];
	for (v=0; v<4; v++)
                bs->verts[(v + bfi) % 4] = seg1->verts[(int) sv[v]];

	// Form connections to children, first initialize all to unconnected.
	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		bs->children[i] = -1;
		bs->sides[i].nWall = NO_WALL;
	}

	// Now form connections between segments.

	bs->children[AttachSide] = seg1 - gameData.segs.segments;
        bs->children[(int) sideOpposite[AttachSide]] = seg2 - gameData.segs.segments;

	seg1->children[side1] = SEG_IDX (bs); //seg2 - gameData.segs.segments;
	seg2->children[side2] = SEG_IDX (bs); //seg1 - gameData.segs.segments;

	//	Validate bridge tSegment, and if degenerate, clean up mess.
	Degenerate_segment_found = 0;

	ValidateSegment(bs);

	if (Degenerate_segment_found) {
		seg1->children[side1] = -1;
		seg2->children[side2] = -1;
		bs->children[AttachSide] = -1;
                bs->children[(int) sideOpposite[AttachSide]] = -1;
		if (med_delete_segment(bs)) {
#if TRACE
			con_printf (CON_DEBUG, "Oops, tried to delete bridge tSegment (because it's degenerate), but couldn't.\n");
#endif
			Int3();
		}
		editor_status("Bridge tSegment would be degenerate, not created.\n");
		return 1;
	} else {
		ValidateSegment(seg1);	// used to only validate tSide, but tSegment does more error checking: ,side1);
		ValidateSegment(seg2);	// ,side2);
		med_propagate_tmaps_to_segments(seg1,bs,0);

		editor_status("Bridge tSegment formed.");
		warn_if_concave_segment(bs);
		return 0;
	}
}

// -------------------------------------------------------------------------------
//	Create a tSegment given center, dimensions, rotation matrix.
//	Note that the created tSegment will always have planar sides and rectangular cross sections.
//	It will be created with walls on all sides, ie not connected to anything.
void med_create_segment(tSegment *sp,fix cx, fix cy, fix cz, fix length, fix width, fix height, vmsMatrix *mp)
{
	int			i,f;
	vmsVector	v0,v1,cv;
	tSegment2 *sp2;

	gameData.segs.nSegments++;

	sp->nSegment = 1;						// What to put here?  I don't know.
	sp2 = &gameData.segs.segment2s[sp->nSegment];

	// Form connections to children, of which it has none.
	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		sp->children[i] = -1;
//		sp->sides[i].renderFlag = 0;
		sp->sides[i].nWall = NO_WALL;
	}

	sp->group = -1;
	sp2->nMatCen = -1;

	//	Create relative-to-center vertices, which are the rotated points on the box defined by length, width, height
	sp->verts[0] = med_add_vertex(VmVecRotate(&v1,VmVecMake(&v0,+width/2,+height/2,-length/2),mp);
	sp->verts[1] = med_add_vertex(VmVecRotate(&v1,VmVecMake(&v0,+width/2,-height/2,-length/2),mp);
	sp->verts[2] = med_add_vertex(VmVecRotate(&v1,VmVecMake(&v0,-width/2,-height/2,-length/2),mp);
	sp->verts[3] = med_add_vertex(VmVecRotate(&v1,VmVecMake(&v0,-width/2,+height/2,-length/2),mp);
	sp->verts[4] = med_add_vertex(VmVecRotate(&v1,VmVecMake(&v0,+width/2,+height/2,+length/2),mp);
	sp->verts[5] = med_add_vertex(VmVecRotate(&v1,VmVecMake(&v0,+width/2,-height/2,+length/2),mp);
	sp->verts[6] = med_add_vertex(VmVecRotate(&v1,VmVecMake(&v0,-width/2,-height/2,+length/2),mp);
	sp->verts[7] = med_add_vertex(VmVecRotate(&v1,VmVecMake(&v0,-width/2,+height/2,+length/2),mp);

	// Now create the vector which is the center of the tSegment and add that to all vertices.
	while (!VmVecMake(&cv,cx,cy,cz);

	//	Now, add the center to all vertices, placing the tSegment in 3 space.
	for (i=0; i<MAX_VERTICES_PER_SEGMENT; i++)
		VmVecAdd(&gameData.segs.vertices[sp->verts[i]],&gameData.segs.vertices[sp->verts[i]],&cv);

	//	Set scale vector.
//	sp->scale.x = width;
//	sp->scale.y = height;
//	sp->scale.z = length;

	//	Add faces to all sides.
	for (f=0; f<MAX_SIDES_PER_SEGMENT; f++)
		CreateWallsOnSide(sp,f);

	sp->objects = -1;		//no gameData.objs.objects in this tSegment

	// Assume nothing special about this tSegment
	sp2->special = 0;
	sp2->value = 0;
	sp2->xAvgSegLight = 0;
	sp2->nMatCen = -1;

	copy_tmaps_to_segment(sp, &New_segment);

	assign_default_uvs_to_segment(sp);
}

// ----------------------------------------------------------------------------------------------
//	Create New_segment using a specified scale factor.
void med_create_new_segment(vmsVector *scale)
{
	int			s,t;
	vmsVector	v0;
	tSegment		*sp = &New_segment;
	tSegment2 *sp2;

	fix			length,width,height;

	length = scale->z;
	width = scale->x;
	height = scale->y;

	sp->nSegment = 1;						// What to put here?  I don't know.
	sp2 = &gameData.segs.segment2s[sp->nSegment];

	//	Create relative-to-center vertices, which are the points on the box defined by length, width, height
	t = gameData.segs.nVertices;
	sp->verts[0] = med_set_vertex(NEW_SEGMENT_VERTICES+0,VmVecMake(&v0,+width/2,+height/2,-length/2);
	sp->verts[1] = med_set_vertex(NEW_SEGMENT_VERTICES+1,VmVecMake(&v0,+width/2,-height/2,-length/2);
	sp->verts[2] = med_set_vertex(NEW_SEGMENT_VERTICES+2,VmVecMake(&v0,-width/2,-height/2,-length/2);
	sp->verts[3] = med_set_vertex(NEW_SEGMENT_VERTICES+3,VmVecMake(&v0,-width/2,+height/2,-length/2);
	sp->verts[4] = med_set_vertex(NEW_SEGMENT_VERTICES+4,VmVecMake(&v0,+width/2,+height/2,+length/2);
	sp->verts[5] = med_set_vertex(NEW_SEGMENT_VERTICES+5,VmVecMake(&v0,+width/2,-height/2,+length/2);
	sp->verts[6] = med_set_vertex(NEW_SEGMENT_VERTICES+6,VmVecMake(&v0,-width/2,-height/2,+length/2);
	sp->verts[7] = med_set_vertex(NEW_SEGMENT_VERTICES+7,VmVecMake(&v0,-width/2,+height/2,+length/2);
	gameData.segs.nVertices = t;

//	sp->scale = *scale;

	// Form connections to children, of which it has none, init faces and tmaps.
	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
		sp->children[s] = -1;
//		sp->sides[s].renderFlag = 0;
		sp->sides[s].nWall = NO_WALL;
		CreateWallsOnSide(sp,s);
		sp->sides[s].nBaseTex = s;					// assign some stupid old tmap to this tSide.
		sp->sides[s].nOvlTex = 0;
	}

	Seg_orientation.p = 0;	Seg_orientation.b = 0;	Seg_orientation.h = 0;

	sp->objects = -1;		//no gameData.objs.objects in this tSegment

	assign_default_uvs_to_segment(sp);

	// Assume nothing special about this tSegment
	sp2->special = 0;
	sp2->value = 0;
	sp2->xAvgSegLight = 0;
	sp2->nMatCen = -1;
}

// -------------------------------------------------------------------------------
void med_create_new_segment_from_cursegp(void)
{
	vmsVector	scalevec;
	vmsVector	uVec, rVec, fVec;

	med_extract_up_vector_from_segment_side(Cursegp, Curside, &uVec);
	med_extract_right_vector_from_segment_side(Cursegp, Curside, &rVec);
	extract_forward_vector_from_segment(Cursegp, &fVec);

	scalevec.x = VmVecMag(&rVec);
	scalevec.y = VmVecMag(&uVec);
	scalevec.z = VmVecMag(&fVec);

	med_create_new_segment(&scalevec);
}

// -------------------------------------------------------------------------------
//	Initialize all vertices to inactive status.
void init_all_vertices(void)
{
	int		v;
	int	s;

	for (v=0; v<MAX_SEGMENT_VERTICES; v++)
		Vertex_active[v] = 0;

	for (s=0; s<MAX_SEGMENTS; s++)
		gameData.segs.segments[s].nSegment = -1;

}

// --------------------------------------------------------------------------------------
//	Create a new mine, set global variables.
int create_new_mine(void)
{
	int	s;
	vmsVector	sizevec;
	vmsMatrix	m1 = IDENTITY_MATRIX;

	// initialize_mine_arrays();

//	gamestate_not_restored = 1;

	// Clear refueling center code
	FuelCenReset();
	hostage_init_all();

	init_all_vertices();

	gameData.missions.nCurrentLevel = 0;		//0 means not a real level
	gameData.missions.szCurrentLevel[0] = 0;

	CurObject_index = -1;
	ResetObjects(1);		//just one tObject, the tPlayer

	num_groups = 0;
	current_group = -1;

	gameData.segs.nVertices = 0;		// Number of vertices in global array.
	gameData.segs.nSegments = 0;		// Number of segments in global array, will get increased in med_create_segment
	Cursegp = gameData.segs.segments;	// Say current tSegment is the only tSegment.
	Curside = WBACK;		// The active tSide is the back tSide
	Markedsegp = 0;		// Say there is no marked tSegment.
	Markedside = WBACK;	//	Shouldn't matter since Markedsegp == 0, but just in case...
	for (s=0;s<MAX_GROUPS+1;s++) {
		GroupList[s].num_segments = 0;		
		GroupList[s].num_vertices = 0;		
		Groupsegp[s] = NULL;
		Groupside[s] = 0;
	}

	gameData.matCens.nMatCens = 0;
	gameData.walls.nOpenDoors = 0;
	WallInit();
	trigger_init();

	// Create New_segment, which is the tSegment we will be adding at each instance.
	med_create_new_segment(VmVecMake(&sizevec,DEFAULT_X_SIZE,DEFAULT_Y_SIZE,DEFAULT_Z_SIZE);		// New_segment = gameData.segs.segments[0];
//	med_create_segment(gameData.segs.segments,0,0,0,DEFAULT_X_SIZE,DEFAULT_Y_SIZE,DEFAULT_Z_SIZE,vm_mat_make(&m1,F1_0,0,0,0,F1_0,0,0,0,F1_0);
	med_create_segment(gameData.segs.segments,0,0,0,DEFAULT_X_SIZE,DEFAULT_Y_SIZE,DEFAULT_Z_SIZE,&m1);

	N_found_segs = 0;
	N_selected_segs = 0;
	N_warning_segs = 0;

	//--repair-- create_local_segment_data();

	gameData.reactor.triggers.nLinks = 0;

    //editor_status("New mine created.");
	return	0;			// say no error
}

// --------------------------------------------------------------------------------------------------
// Copy a tSegment from *ssp to *dsp.  Do not simply copy the struct.  Use *dsp's vertices, copying in
//	just the values, not the indices.
void med_copy_segment(tSegment *dsp,tSegment *ssp)
{
	int	v;
	int	verts_copy[MAX_VERTICES_PER_SEGMENT];

	//	First make a copy of the vertex list.
	for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
		verts_copy[v] = dsp->verts[v];

	// Now copy the whole struct.
	*dsp = *ssp;

	// Now restore the vertex indices.
	for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
		dsp->verts[v] = verts_copy[v];

	// Now destructively modify the vertex values for all vertex indices.
	for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
		gameData.segs.vertices[dsp->verts[v]] = gameData.segs.vertices[ssp->verts[v]];
}

// -----------------------------------------------------------------------------
//	Create coordinate axes in orientation of specified tSegment, stores vertices at *vp.
void create_coordinate_axes_from_segment(tSegment *sp,short *vertnums)
{
	vmsMatrix	rotmat;
	vmsVector t;

	med_extract_matrix_from_segment(sp,&rotmat);

	COMPUTE_SEGMENT_CENTER(&gameData.segs.vertices[vertnums[0]],sp);

	t = rotmat.rVec;
	VmVecScale(&t,i2f(32);
	VmVecAdd(&gameData.segs.vertices[vertnums[1]],&gameData.segs.vertices[vertnums[0]],&t);

	t = rotmat.uVec;
	VmVecScale(&t,i2f(32);
	VmVecAdd(&gameData.segs.vertices[vertnums[2]],&gameData.segs.vertices[vertnums[0]],&t);

	t = rotmat.fVec;
	VmVecScale(&t,i2f(32);
	VmVecAdd(&gameData.segs.vertices[vertnums[3]],&gameData.segs.vertices[vertnums[0]],&t);
}

// -----------------------------------------------------------------------------
//	Determine if a tSegment is concave. Returns true if concave
int check_seg_concavity(tSegment *s)
{
	int sn,vn;
	vmsVector n0,n1;

	for (sn=0;sn<MAX_SIDES_PER_SEGMENT;sn++)
		for (vn=0;vn<=4;vn++) {

			VmVecNormal(&n1,
				&gameData.segs.vertices[s->verts[sideToVerts[sn][vn%4]]],
				&gameData.segs.vertices[s->verts[sideToVerts[sn][(vn+1)%4]]],
				&gameData.segs.vertices[s->verts[sideToVerts[sn][(vn+2)%4]]]);

			//VmVecNormalize(&n1);

			if (vn>0) if (VmVecDotProd(&n0,&n1) < f0_5) return 1;

			n0 = n1;
		}

	return 0;
}


// -----------------------------------------------------------------------------
//	Find all concave segments and add to list
void find_concave_segs()
{
	int i;
	tSegment *s;

	N_warning_segs = 0;

	for (s=gameData.segs.segments,i=gameData.segs.nLastSegment;i>=0;s++,i--)
		if (s->nSegment != -1)
			if (check_seg_concavity(s)) Warning_segs[N_warning_segs++]=SEG_PTR_2_NUM(s);


}


// -----------------------------------------------------------------------------
void warn_if_concave_segments(void)
{
	char temp[1];

	find_concave_segs();

	if (N_warning_segs) {
		editor_status("*** WARNING *** %d concave segments in mine! *** WARNING ***",N_warning_segs);
		sprintf( temp, "%d", N_warning_segs );
    }
}

// -----------------------------------------------------------------------------
//	Check tSegment s, if concave, warn
void warn_if_concave_segment(tSegment *s)
{
    char temp[1];
	int	result;

	result = check_seg_concavity(s);

	if (result) {
		Warning_segs[N_warning_segs++] = SEG_IDX (s);

        if (N_warning_segs) {
			editor_status("*** WARNING *** New tSegment is concave! *** WARNING ***");
            sprintf( temp, "%d", N_warning_segs );
        }
        //else
           // editor_status("");
	} //else
        //editor_status("");
}


// -------------------------------------------------------------------------------
//	Find tSegment adjacent to sp:tSide.
//	Adjacent means a tSegment which shares all four vertices.
//	Return true if tSegment found and fill in tSegment in adj_sp and tSide in adj_side.
//	Return false if unable to find, in which case adj_sp and adj_side are undefined.
int med_find_adjacent_segment_side(tSegment *sp, int tSide, tSegment **adj_sp, int *adj_side)
{
	int			seg,s,v,vv;
	int			abs_verts[4];

	//	Stuff abs_verts[4] array with absolute vertex indices
	for (v=0; v<4; v++)
		abs_verts[v] = sp->verts[sideToVerts[tSide][v]];

	//	Scan all segments, looking for a tSegment which contains the four abs_verts
	for (seg=0; seg<=gameData.segs.nLastSegment; seg++) {
		if (seg != SEG_IDX (sp)) {
			for (v=0; v<4; v++) {												// do for each vertex in abs_verts
				for (vv=0; vv<MAX_VERTICES_PER_SEGMENT; vv++)			// do for each vertex in tSegment
					if (abs_verts[v] == gameData.segs.segments[seg].verts[vv])
						goto fass_found1;											// Current vertex (indexed by v) is present in tSegment, try next
				goto fass_next_seg;												// This tSegment doesn't contain the vertex indexed by v
			fass_found1: ;
			}		// end for v

			//	All four vertices in sp:tSide are present in tSegment seg.
			//	Determine tSide and return
			for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
				for (v=0; v<4; v++) {
					for (vv=0; vv<4; vv++) {
						if (gameData.segs.segments[seg].verts[sideToVerts[s][v]] == abs_verts[vv])
							goto fass_found2;
					}
					goto fass_next_side;											// Couldn't find vertex v in current tSide, so try next tSide.
				fass_found2: ;
				}
				// Found all four vertices in current tSide.  We are done!
				*adj_sp = &gameData.segs.segments[seg];
				*adj_side = s;
				return 1;
			fass_next_side: ;
			}
			Assert(0);	// Impossible -- we identified this tSegment as containing all 4 vertices of tSide "tSide", but we couldn't find them.
			return 0;
		fass_next_seg: ;
		}
	}

	return 0;
}


#define JOINT_THRESHOLD	10000*F1_0 		// (Huge threshold)

// -------------------------------------------------------------------------------
//	Find tSegment closest to sp:tSide.
//	Return true if tSegment found and fill in tSegment in adj_sp and tSide in adj_side.
//	Return false if unable to find, in which case adj_sp and adj_side are undefined.
int med_find_closest_threshold_segment_side(tSegment *sp, int tSide, tSegment **adj_sp, int *adj_side, fix threshold)
{
	int			seg,s;
	vmsVector  vsc, vtc; 		// original tSegment center, test tSegment center
	fix			currentDist, closest_segDist;

	if (IS_CHILD(sp->children[tSide]))
		return 0;

	COMPUTE_SIDE_CENTER(&vsc, sp, tSide); 

	closest_segDist = JOINT_THRESHOLD;

	//	Scan all segments, looking for a tSegment which contains the four abs_verts
	for (seg=0; seg<=gameData.segs.nLastSegment; seg++) 
		if (seg != SEG_IDX (sp)) 
			for (s=0;s<MAX_SIDES_PER_SEGMENT;s++) {
				if (!IS_CHILD(gameData.segs.segments[seg].children[s])) {
					COMPUTE_SIDE_CENTER(&vtc, &gameData.segs.segments[seg], s); 
					currentDist = VmVecDist( &vsc, &vtc );
					if (currentDist < closest_segDist) {
						*adj_sp = &gameData.segs.segments[seg];
						*adj_side = s;
						closest_segDist = currentDist;
					}
				}
			}	

	if (closest_segDist < threshold)
		return 1;
	else
		return 0;
}



void med_check_all_vertices()
{
	int		s,v;
	tSegment	*sp;
	int		count;

	count = 0;

	for (s=0; s<gameData.segs.nSegments; s++) {
		sp = &gameData.segs.segments[s];
		if (sp->nSegment != -1)
			for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
				Assert(sp->verts[v] <= gameData.segs.nLastVertex);
					
	}

}

//	-----------------------------------------------------------------------------------------------------
void check_for_overlapping_segment(int nSegment)
{
	int	i, v;
	segmasks	masks;
	vmsVector	segcenter;

	COMPUTE_SEGMENT_CENTER(&segcenter, &gameData.segs.segments[nSegment]);

	for (i=0;i<=gameData.segs.nLastSegment; i++) {
		if (i != nSegment) {
			masks = GetSegMasks(&segcenter, i, 0);
			if (masks.centerMask == 0) {
#if TRACE
				con_printf (CON_DEBUG, "Segment %i center is contained in tSegment %i\n", nSegment, i);
#endif
				continue;
			}

			for (v=0; v<8; v++) {
				vmsVector	pdel, presult;

				VmVecSub(&pdel, &gameData.segs.vertices[gameData.segs.segments[nSegment].verts[v]], &segcenter);
				VmVecScaleAdd(&presult, &segcenter, &pdel, (F1_0*15)/16);
				masks = GetSegMasks(&presult, i, 0);
				if (masks.centerMask == 0) {
#if TRACE
					con_printf (CON_DEBUG, "Segment %i near vertex %i is contained in tSegment %i\n", nSegment, v, i);
#endif
					break;
				}
			}
		}
	}

}

//	-----------------------------------------------------------------------------------------------------
//	Check for overlapping segments.
void check_for_overlapping_segments(void)
{
	int	i;

	med_compress_mine();

	for (i=0; i<=gameData.segs.nLastSegment; i++) {
#if TRACE
		con_printf (CON_DEBUG, "+");
#endif
		check_for_overlapping_segment(i);
	}

	con_printf (CON_DEBUG, "\nDone!\n");
}

