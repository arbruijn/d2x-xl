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
#include "bitmap.h" //the vector/matrix library
#include "gr.h"
#include "segment.h"
#include "transformation.h"

extern int32_t g3d_interp_outline;      //if on, polygon models outlined in white

//Structure for storing u,v,light values.  This structure doesn't have a
//prefix because it was defined somewhere else before it was moved here
//Stucture to store clipping codes in a word
typedef struct tRenderCodes {
	uint8_t ccOr, ccAnd;   //or is low byte, and is high byte
} tRenderCodes;

//flags for point structure
#define PF_PROJECTED    1   //has been projected, so sx,sy valid
#define PF_OVERFLOW     2   //can't project
#define PF_TEMP_POINT   4   //created during clip
#define PF_UVS          8   //has uv values set
#define PF_LS           16  //has lighting values set


//Used to store rotated points for mines.  Has frame count to indicate
//if rotated, and flag to indicate if projected.
class CRenderNormal {
	public:
		CFloatVector	vNormal;
		uint8_t				nFaces;	// # of faces that use this vertex

		inline uint8_t operator++ (void) { return ++nFaces; }

		inline uint8_t operator++ (int32_t) { return nFaces++; }

		inline CFloatVector& operator+= (CFloatVector& other) {
			vNormal += other;
			nFaces++;
			return vNormal;
			}

		inline CFloatVector& operator+= (CFloatVector3& other) {
			*vNormal.XYZ () += other;
			nFaces++;
			return vNormal;
			}

		inline CFloatVector& operator+= (CFixVector& other) {
			CFloatVector v;
			v.Assign (other);
			vNormal += v;
			nFaces++;
			return vNormal;
			}

		inline CFloatVector& operator/= (int32_t i) {
			vNormal /= float (i);
			return vNormal;
			}	

		inline float Normalize (void) {
			if (nFaces < 1)
				return 0.0f;
			if (nFaces > 1) {
				vNormal /= nFaces;
				nFaces = 1;
				}
			return CFloatVector::Normalize (vNormal);
			}

		inline void Reset (void) {
			nFaces = 0;
			vNormal.SetZero ();
			}
	};

class CRenderPoint {
	private:
		CFixVector		m_vertex [2];	//untransformed and transformed point
		tUVL				m_uvl;			//u,v,l coords
		tScreenPos		m_screen;		//screen x&y
		uint8_t			m_codes;			//clipping codes
		uint8_t			m_flags;			//projected?
		uint16_t			m_key;
		int32_t			m_index;			//keep structure longword aligned
		CRenderNormal	m_normal;

	public:
		CRenderPoint () : m_codes (0), m_flags (0), m_key (0), m_index (-1) {}

		void Project (void);

		uint8_t Project (CTransformation& transformation, CFloatVector3& viewPos);

		uint8_t ProjectAndEncode (CTransformation& transformation, int32_t nVertex);

		uint8_t Add (CRenderPoint& other, CFixVector& vDelta);

		uint8_t Encode (void);

		void Transform (int32_t nVertex = -1);

		inline uint8_t TransformAndEncode (const CFixVector& src) {
			m_vertex [0] = src;
			Transform ();
			m_flags = 0;
			return Encode ();
			}

		inline uint8_t Codes (void) { return m_codes; }
		inline void SetCodes (uint8_t codes = 0) { m_codes = codes; }
		inline void AddCodes (uint8_t codes) { m_codes |= codes; }
		inline uint8_t Flags (void) { return m_flags; }
		inline void SetFlags (uint8_t flags = 0) { m_flags = flags; }
		inline void AddFlags (uint8_t flags) { m_flags |= flags; }
		inline void Reset (void) { m_flags = m_codes = 0; }
		inline uint8_t Behind (void) { return m_codes & CC_BEHIND; }
		inline bool Visible (void) { return m_codes == 0; }
		inline uint8_t Projected (void) { return m_flags & PF_PROJECTED; }
		inline uint8_t Overflow (void) { return m_flags & PF_OVERFLOW; }
		inline int32_t Index (void) { return m_index; }
		inline void SetIndex (int32_t i) { m_index = i; }
		inline int32_t Key (void) { return m_key; }
		inline void SetKey (int32_t i) { m_key = i; }
		inline tScreenPos& Screen (void) { return m_screen; }
		inline fix X (void) { return m_screen.x; }
		inline fix Y (void) { return m_screen.y; }

		inline CRenderNormal& Normal (void) { return m_normal; }

		inline CFloatVector* GetNormal (void) { return &m_normal.vNormal; }

		inline CFloatVector3* GetNormal (CRenderPoint *pPoint, CFloatVector* vNormal) {
			return m_normal.nFaces ? m_normal.vNormal.XYZ () : vNormal->XYZ ();
			}

		inline CFixVector& WorldPos () { return m_vertex [0]; }
		inline CFixVector& ViewPos () { return m_vertex [1]; }

	};

typedef void tmap_drawer_func (CBitmap *, int32_t, CRenderPoint **);
typedef void flat_drawer_func (int32_t, int32_t *);
typedef int32_t line_drawer_func (fix, fix, fix, fix);
typedef tmap_drawer_func *tmap_drawer_fp;
typedef flat_drawer_func *flat_drawer_fp;
typedef line_drawer_func *line_drawer_fp;

//Functions in library

//3d system startup and shutdown:

//Frame setup functions:

//start the frame
void G3StartFrame (CTransformation& transformation, int32_t bFlat, int32_t bResetColorBuf, fix xStereoSeparation);

//set view from x,y,z & p,b,h, zoom.  Must call one of g3_setView_* ()
void SetupViewAngles (const CFixVector& vPos, const CAngleVector& vOrient, fix zoom);

//set view from x,y,z, viewer matrix, and zoom.  Must call one of g3_setView_* ()
void SetupTransformation (CTransformation& transformation, const CFixVector& vPos, const CFixMatrix& mOrient, fix xZoom, int32_t bOglScale, fix xStereoSeparation = 0, bool bSetupRenderer = true);

//end the frame
void G3EndFrame (CTransformation& transformation, int32_t nWindow);

//Instancing

//instance at specified point with specified orientation
//void transformation.Begin (const CFixVector& pos);


//Misc utility functions:

//returns true if a plane is facing the viewer. takes the unrotated surface
//normal of the plane, and a point on it.  The normal need not be normalized
int32_t G3CheckNormalFacing (const CFixVector& v, const CFixVector& norm);

//Point definition and rotation functions:

//specify the arrays refered to by the 'pointlist' parms in the following
//functions.  I'm not sure if we will keep this function, but I need
//it now.
//void g3_set_points (CRenderPoint *points,CFixVector *vecs);

//projects a point
uint8_t ProjectPoint (CFloatVector3& v, tScreenPos& screen, uint8_t flags = 0, uint8_t codes = 0);

uint8_t ProjectPoint (CFixVector& v, tScreenPos& s, uint8_t flags = 0, uint8_t codes = 0);

uint8_t OglProjectPoint (CFloatVector3& v, tScreenPos& s, uint8_t flags = 0, uint8_t codes = 0);


//calculate the depth of a point - returns the z coord of the rotated point
fix G3CalcPointDepth (const CFixVector& pnt);

//Drawing functions:

//draw a flat-shaded face.
//returns 1 if off screen, 0 if drew
int32_t G3DrawPoly (int32_t nv,CRenderPoint **pointlist);

//draw a texture-mapped face.
//returns 1 if off screen, 0 if drew
int32_t G3DrawTMap (int32_t nv,CRenderPoint **pointlist,tUVL *uvl_list,CBitmap *bm, int32_t bBlend);

//draw a sortof sphere - i.e., the 2d radius is proportional to the 3d
//radius, but not to the distance from the eye
int32_t G3DrawSphere (CRenderPoint *pnt,fix rad, int32_t bBigSphere);

//@@//return ligting value for a point
//@@fix g3_compute_lightingValue (CRenderPoint *rotated_point,fix normval);


//like G3DrawPoly (), but checks to see if facing.  If surface normal is
//NULL, this routine must compute it, which will be slow.  It is better to
//pre-compute the normal, and pass it to this function.  When the normal
//is passed, this function works like G3CheckNormalFacing () plus
//G3DrawPoly ().
//returns -1 if not facing, 1 if off screen, 0 if drew
int32_t G3CheckAndDrawPoly (int32_t nv, CRenderPoint **pointlist, CFixVector *norm, CFixVector *pnt);
int32_t G3CheckAndDrawTMap (int32_t nv, CRenderPoint **pointlist, tUVL *uvl_list, CBitmap *bmP, CFixVector *norm, CFixVector *pnt);

//draws a line. takes two points.
int32_t G3DrawLine (CRenderPoint *p0,CRenderPoint *p1);

//draw a polygon that is always facing you
//returns 1 if off screen, 0 if drew
int32_t G3DrawRodPoly (CRenderPoint *bot_point,fix bot_width,CRenderPoint *top_point,fix top_width);

//draw a bitmap CObject that is always facing you
//returns 1 if off screen, 0 if drew
int32_t G3DrawRodTexPoly (CBitmap *bitmap,CRenderPoint *bot_point,fix bot_width,CRenderPoint *top_point,fix top_width,fix light, tUVL *uvlList, int32_t bAdditive = 0);

void InitFreePoints (void);

//specifies 2d drawing routines to use instead of defaults.  Passing
//NULL for either or both restores defaults
extern CRenderPoint *Vbuf0 [];
extern CRenderPoint *Vbuf1 [];

#endif
