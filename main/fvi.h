/* $Id: fvi.h,v 1.2 2003/10/10 09:36:35 btb Exp $ */
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

#ifndef _FVI_H
#define _FVI_H

#include "vecmat.h"
#include "segment.h"
#include "object.h"

//return values for FindVectorIntersection() - what did we hit?
#define HIT_NONE		0		//we hit nothing
#define HIT_WALL		1		//we hit - guess - a wall
#define HIT_OBJECT	2		//we hit an tObject - which one?  no way to tell...
#define HIT_BAD_P0	3		//start point is not in specified tSegment

#define MAX_FVI_SEGS 200

typedef struct fvi_hit_info {
	int 			nType;						//what sort of intersection
	short 		nSegment;					//what tSegment hit_pnt is in
	short			nSegment2;
	short 		nSide;						//if hit wall, which tSide
	short 		nSideSegment;				//what tSegment the hit tSide is in
	short 		nObject;						//if tObject hit, which tObject
	vmsVector	vPoint;						//where we hit
	vmsVector 	vNormal;						//if hit wall, ptr to its surface normal
	int			nNestCount;
} fvi_hit_info;

//this data structure gets filled in by FindVectorIntersection()
typedef struct tFVIData {
	fvi_hit_info	hit;
	short 			nSegments;					//how many segs we went through
	short 			segList [MAX_FVI_SEGS];	//list of segs vector went through
} tFVIData;

//flags for fvi query
#define FQ_CHECK_OBJS	1		//check against objects?
#define FQ_TRANSWALL		2		//go through transparent walls
#define FQ_TRANSPOINT	4		//go through trans wall if hit point is transparent
#define FQ_GET_SEGLIST	8		//build a list of segments
#define FQ_IGNORE_POWERUPS	16		//ignore powerups
#define FQ_SEE_OBJS		32

//this data contains the parms to fvi()
typedef struct tVFIQuery {
	vmsVector	*p0, *p1;
	short			startSeg;
	fix			radP0, radP1;
	short			thisObjNum;
	short			*ignoreObjList;
	int			flags;
} tVFIQuery;

//Find out if a vector intersects with anything.
//Fills in hit_data, an tFVIData structure (see above).
//Parms:
//  p0 & startseg 	describe the start of the vector
//  p1 					the end of the vector
//  rad 					the radius of the cylinder
//  thisobjnum 		used to prevent an tObject with colliding with itself
//  ingore_obj_list	NULL, or ptr to a list of objnums to ignore, terminated with -1
//  check_objFlag	determines whether collisions with objects are checked
//Returns the hit_data->hitType
int FindVectorIntersection(tVFIQuery *fq,tFVIData *hit_data);

//finds the uv coords of the given point on the given seg & tSide
//fills in u & v. if l is non-NULL fills it in also
void FindHitPointUV(fix *u,fix *v,fix *l, vmsVector *pnt,tSegment *seg,int nSide,int facenum);

//Returns true if the tObject is through any walls
int ObjectIntersectsWall (tObject *objP);

int PixelTranspType (short nTexture, short nOrient, fix u, fix v);	//-1: supertransp., 0: opaque, 1: transparent

int CheckLineToFace (vmsVector *newP, vmsVector *p0, vmsVector *p1, 
							short nSegment, short nSide, short iFace, int nv, fix rad);

int CanSeePoint (tObject *objP, vmsVector *vSource, vmsVector *vDest, short nSegment);

#endif

