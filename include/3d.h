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

#ifndef _3D_H
#define _3D_H

#include "fix.h"
#include "vecmat.h" //the vector/matrix library
#include "globvars.h"
#include "gr.h"

extern int g3d_interp_outline;      //if on, polygon models outlined in white

extern CFixVector Matrix_scale;     //how the matrix is currently scaled

extern short highest_texture_num;

//Structure for storing u,v,light values.  This structure doesn't have a
//prefix because it was defined somewhere else before it was moved here
typedef struct tUVL {
	fix u,v,l;
} tUVL;

//Stucture to store clipping codes in a word
typedef struct g3sCodes {
	ubyte ccOr, ccAnd;   //or is low byte, and is high byte
} g3sCodes;

//flags for point structure
#define PF_PROJECTED    1   //has been projected, so sx,sy valid
#define PF_OVERFLOW     2   //can't project
#define PF_TEMP_POINT   4   //created during clip
#define PF_UVS          8   //has uv values set
#define PF_LS           16  //has lighting values set

//clipping codes flags

#define CC_OFF_LEFT     1
#define CC_OFF_RIGHT    2
#define CC_OFF_BOT      4
#define CC_OFF_TOP      8
#define CC_BEHIND       0x80

//Used to store rotated points for mines.  Has frame count to indicate
//if rotated, and flag to indicate if projected.
typedef struct g3sNormal {
	CFloatVector		vNormal;
	ubyte			nFaces;	// # of faces that use this vertex
} g3sNormal;

typedef struct tScreenPos {
	fix			x, y;
} tScreenPos;

typedef struct g3sPoint {
	CFixVector	p3_src;			//untransformed point
	CFixVector	p3_vec;			//x,y,z of rotated point
	tUVL			p3_uvl;			//u,v,l coords
	tScreenPos	p3_screen;		//screen x&y
	ubyte			p3_codes;		//clipping codes
	ubyte			p3_flags;		//projected?
	short			p3_key;
	int			p3_index;		//keep structure longword aligned
	g3sNormal	p3_normal;
} g3sPoint;

//An CObject, such as a robot
typedef struct g3sObject {
	CFixVector o3_pos;       //location of this CObject
	CAngleVector o3_orient;    //orientation of this CObject
	int o3_nverts;          //number of points in the CObject
	int o3_nfaces;          //number of faces in the CObject

	//this will be filled in later

} g3sObject;

typedef void tmap_drawer_func (CBitmap *, int, g3sPoint **);
typedef void flat_drawer_func (int, int *);
typedef int line_drawer_func (fix, fix, fix, fix);
typedef tmap_drawer_func *tmap_drawer_fp;
typedef flat_drawer_func *flat_drawer_fp;
typedef line_drawer_func *line_drawer_fp;

//Functions in library

//3d system startup and shutdown:

//initialize the 3d system
void g3_init (void);

//close down the 3d system
void _CDECL_ g3_close (void);


//Frame setup functions:

//start the frame
void G3StartFrame (int bFlat, int bResetColorBuf);

//set view from x,y,z & p,b,h, zoom.  Must call one of g3_setView_* ()
void G3SetViewAngles (const CFixVector& view_pos, const CAngleVector& view_orient,fix zoom);

//set view from x,y,z, viewer matrix, and zoom.  Must call one of g3_setView_* ()
void G3SetViewMatrix (const CFixVector& view_pos, const CFixMatrix& view_matrix,fix zoom, int bOglScale);

//end the frame
void G3EndFrame (void);

//draw a horizon
void g3_draw_horizon (int sky_color,int ground_color);

//get vectors that are edge of horizon
int g3_compute_sky_polygon (fix *points_2d,CFixVector *vecs);

//Instancing

//instance at specified point with specified orientation
//void G3StartInstanceMatrix (const CFixVector& pos);
void G3StartInstanceMatrix (const CFixVector& pos,
                           const CFixMatrix& orient=CFixMatrix::IDENTITY);

//instance at specified point with specified orientation
void G3StartInstanceAngles (const CFixVector& pos,
                           const CAngleVector& angles = CAngleVector::ZERO);

//pops the old context
void G3DoneInstance ();
int G3PushMatrix (void);
int G3PopMatrix (void);

//Misc utility functions:

//get current field of view.  Fills in angle for x & y
void g3_get_FOV (fixang *fov_x,fixang *fov_y);

//get zoom.  For a given window size, return the zoom which will achieve
//the given FOV along the given axis.
fix g3_get_zoom (char axis,fixang fov,short window_width,short window_height);

//returns the normalized, unscaled view vectors
// \unused
void g3_getView_vectors (CFixVector& forward, CFixVector& up, CFixVector& right);

//returns true if a plane is facing the viewer. takes the unrotated surface
//normal of the plane, and a point on it.  The normal need not be normalized
int G3CheckNormalFacing (const CFixVector& v, const CFixVector& norm);

//Point definition and rotation functions:

//specify the arrays refered to by the 'pointlist' parms in the following
//functions.  I'm not sure if we will keep this function, but I need
//it now.
//void g3_set_points (g3sPoint *points,CFixVector *vecs);

//returns codes_and & codes_or of a list of points numbers
g3sCodes g3_check_codes (int nv,g3sPoint **pointlist);

//projects a point
void G3ProjectPoint (g3sPoint* point);

//code a point.  fills in the p3_codes field of the point, and returns the codes
#if 1
static inline ubyte G3EncodePoint (g3sPoint* p) 
{
	ubyte cc = 0;
	fix z = p->p3_vec [Z];
	fix x = FixMulDiv (p->p3_vec [X], viewInfo.scale [X], viewInfo.zoom);

if (x > z)
	cc |= CC_OFF_RIGHT;
if (x < -z)
	cc |= CC_OFF_LEFT;
if (p->p3_vec [Y] > z)
	cc |= CC_OFF_TOP;
if (p->p3_vec [Y] < -z)
	cc |= CC_OFF_BOT;
if (z < 0)
	cc |= CC_BEHIND;
return p->p3_codes = cc;
}
#else
ubyte G3EncodePoint (g3sPoint *point);
#endif

static inline CFixVector& G3TranslatePoint (CFixVector& dest, const CFixVector& src) 
{
return dest = src - viewInfo.pos;
}

static inline CFixVector& G3RotatePoint (CFixVector& dest, const CFixVector& src, int bUnscaled) 
{
return dest = viewInfo.view [bUnscaled] * src;
}

static inline CFixVector& G3TransformPoint (CFixVector& dest, const CFixVector& src, int bUnscaled) 
{
CFixVector vTrans = src - viewInfo.pos;
return dest = viewInfo.view [bUnscaled] * vTrans;
}

static inline CFloatVector& G3TranslatePoint (CFloatVector& dest, const CFloatVector& src) 
{
return dest = src - viewInfo.posf [0];
}

static inline CFloatVector& G3RotatePoint (CFloatVector& dest, const CFloatVector& src, int bUnscaled) 
{
return dest = viewInfo.viewf [bUnscaled] * src;
}

static inline CFloatVector3& G3RotatePoint (CFloatVector3& dest, const CFloatVector3& src, int bUnscaled) 
{
return dest = viewInfo.viewf [bUnscaled] * src;
}

static inline CFloatVector& G3TransformPoint (CFloatVector& dest, const CFloatVector& src, int bUnscaled) 
{
CFloatVector vTrans = src - viewInfo.posf [0];
return dest = viewInfo.viewf [bUnscaled] * vTrans;
}

static inline ubyte G3TransformAndEncodePoint (g3sPoint* dest, const CFixVector& src) 
{
dest->p3_src = src;
G3TransformPoint (dest->p3_vec, src, 0);
dest->p3_flags = 0;
return G3EncodePoint (dest);
}

//calculate the depth of a point - returns the z coord of the rotated point
fix G3CalcPointDepth (const CFixVector& pnt);

//from a 2d point, compute the vector through that point
void G3Point2Vec (CFixVector& v, short sx, short sy);

//delta rotation functions
const CFixVector& G3RotateDeltaX (CFixVector& dest, fix dx);
const CFixVector& G3RotateDeltaY (CFixVector& dest, fix dy);
const CFixVector& G3RotateDeltaZ (CFixVector& dest, fix dz);
const CFixVector& G3RotateDeltaVec (CFixVector& dest, const CFixVector& src);
ubyte G3AddDeltaVec (g3sPoint *dest,g3sPoint *src,CFixVector *deltav);

//Drawing functions:

//draw a flat-shaded face.
//returns 1 if off screen, 0 if drew
int G3DrawPoly (int nv,g3sPoint **pointlist);

//draw a texture-mapped face.
//returns 1 if off screen, 0 if drew
int G3DrawTMap (int nv,g3sPoint **pointlist,tUVL *uvl_list,CBitmap *bm, int bBlend);

//draw a sortof sphere - i.e., the 2d radius is proportional to the 3d
//radius, but not to the distance from the eye
int G3DrawSphere (g3sPoint *pnt,fix rad, int bBigSphere);

//@@//return ligting value for a point
//@@fix g3_compute_lightingValue (g3sPoint *rotated_point,fix normval);


//like G3DrawPoly (), but checks to see if facing.  If surface normal is
//NULL, this routine must compute it, which will be slow.  It is better to
//pre-compute the normal, and pass it to this function.  When the normal
//is passed, this function works like G3CheckNormalFacing () plus
//G3DrawPoly ().
//returns -1 if not facing, 1 if off screen, 0 if drew
int G3CheckAndDrawPoly (int nv, g3sPoint **pointlist, CFixVector *norm, CFixVector *pnt);
int G3CheckAndDrawTMap (int nv, g3sPoint **pointlist, tUVL *uvl_list, CBitmap *bmP, CFixVector *norm, CFixVector *pnt);

//draws a line. takes two points.
int G3DrawLine (g3sPoint *p0,g3sPoint *p1);

//draw a polygon that is always facing you
//returns 1 if off screen, 0 if drew
int G3DrawRodPoly (g3sPoint *bot_point,fix bot_width,g3sPoint *top_point,fix top_width);

//draw a bitmap CObject that is always facing you
//returns 1 if off screen, 0 if drew
int G3DrawRodTexPoly (CBitmap *bitmap,g3sPoint *bot_point,fix bot_width,g3sPoint *top_point,fix top_width,fix light, tUVL *uvlList);

//draws a bitmap with the specified 3d width & height
//returns 1 if off screen, 0 if drew
int G3DrawBitmap (const CFixVector& vPos, fix xWidth, fix xHeight, CBitmap *bmP, tRgbaColorf *color, float alpha, int nTransp);

int G3DrawSprite (const CFixVector& vPos, fix xWidth, fix xHeight, CBitmap *bmP, tRgbaColorf *color, float alpha, int bAdditive, float fSoftRad);

//specifies 2d drawing routines to use instead of defaults.  Passing
//NULL for either or both restores defaults
void G3SetSpecialRender (tmap_drawer_fp tmap_drawer, flat_drawer_fp flat_drawer, line_drawer_fp line_drawer);

extern g3sPoint *Vbuf0 [];
extern g3sPoint *Vbuf1 [];

#endif
