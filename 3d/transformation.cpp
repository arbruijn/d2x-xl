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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>

#include "descent.h"
#include "error.h"
#include "3d.h"
#include "globvars.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "vecmat.h"
#include "interp.h"
#include "oof.h"
#include "dynlight.h"

#define MAX_INSTANCE_DEPTH	10

//------------------------------------------------------------------------------

void CTransformation::Init (void)
{
m_info.pos.SetZero ();
m_info.playerHeadAngles.SetZero ();
m_info.bUsePlayerHeadAngles = 0;
m_info.view [0] = 
m_info.view [1] = CFixMatrix::IDENTITY;
m_info.scale.SetZero ();
m_info.scalef.SetZero ();
m_info.aspect.SetZero ();	
m_info.posf [0].SetZero ();
m_info.posf [1].SetZero ();
m_info.viewf [0] = 
m_info.viewf [1] = 
m_info.viewf [2] = CFloatMatrix::IDENTITY;
m_info.zoom = 0;
m_info.zoomf = 0;
m_save.Create (MAX_INSTANCE_DEPTH);
}

//------------------------------------------------------------------------------

bool CTransformation::Push (void)
{
if (m_save.ToS () >= MAX_INSTANCE_DEPTH)
	return false;
m_save.Push (m_info);
glMatrixMode (GL_MODELVIEW);
glPushMatrix ();
return true;
}

//------------------------------------------------------------------------------

bool CTransformation::Pop (void)
{
if (m_save.ToS () <= 0)
	return false;
m_info = m_save.Pop ();
glMatrixMode (GL_MODELVIEW);
glPopMatrix ();
return true;
}

//------------------------------------------------------------------------------
//instance at specified point with specified orientation
//if matrix==NULL, don't modify matrix.  This will be like doing an offset
void CTransformation::Begin (const CFixVector& vPos, CFixMatrix& mOrient)
{
	CFixVector	vOffs;
	CFixMatrix	mTrans, mRot;

//Assert (nInstanceDepth < MAX_INSTANCE_DEPTH);
if (!Push ())
	return;
if (ogl.m_states.bUseTransform) {
	CFixVector	h;

	if (m_save.ToS () > 1) {
		glScalef (-1.0f, -1.0f, -1.0f);
		Move (vPos);
		glScalef (-1.0f, -1.0f, -1.0f);
		Rotate (mOrient);
		}
	else {
		glLoadIdentity ();
#if 0
		glScalef (transformation.m_info.scalef.dir.coord.x, transformation.m_info.scalef.dir.coord.y, -transformation.m_info.scalef.dir.coord.z);
#else
		glScalef (1, 1, -1);
#endif
		Rotate (m_info.viewf [2]);
		h = m_info.pos - vPos;
		Move (h);
		Rotate (mOrient);
		if (!gameData.models.vScale.IsZero ()) {
			CFloatVector fScale;
			fScale.Assign (gameData.models.vScale);
			glScalef (fScale.v.coord.x, fScale.v.coord.y, fScale.v.coord.z);
			}
		}
	}

//step 1: subtract object position from view position
vOffs = m_info.pos - vPos;
//step 2: rotate view vector through CObject matrix
m_info.pos = mOrient * vOffs;
//step 3: rotate CObject matrix through view_matrix (vm = ob * vm)
mTrans = mOrient.Transpose ();
for (int i = 0; i < 2; i++) {
	mRot = mTrans * m_info.view [i];
	m_info.view [i] = mRot;
	m_info.viewf [i].Assign (m_info.view [i]);
	}
m_info.posf [0].Assign (m_info.pos);
}

// -----------------------------------------------------------------------------------
//delta rotation functions
CFixVector CTransformation::RotateScaledX (CFixVector& dest, fix scale)
{
dest.v.coord.x = m_info.view [0].m.dir.r.v.coord.x;
dest.v.coord.y = m_info.view [0].m.dir.u.v.coord.x;
dest.v.coord.z = m_info.view [0].m.dir.f.v.coord.x;
dest *= scale;
return dest;
}

// -----------------------------------------------------------------------------------

CFixVector CTransformation::RotateScaledY (CFixVector& dest, fix scale)
{
dest.v.coord.x = m_info.view [0].m.dir.r.v.coord.y;
dest.v.coord.y = m_info.view [0].m.dir.u.v.coord.y;
dest.v.coord.z = m_info.view [0].m.dir.f.v.coord.y;
dest *= scale;
return dest;
}

// -----------------------------------------------------------------------------------

CFixVector CTransformation::RotateScaledZ (CFixVector& dest, fix scale)
{
dest.v.coord.x = m_info.view [0].m.dir.r.v.coord.z;
dest.v.coord.y = m_info.view [0].m.dir.u.v.coord.z;
dest.v.coord.z = m_info.view [0].m.dir.f.v.coord.z;
dest *= scale;
return dest;
}

// -----------------------------------------------------------------------------------

const CFixVector& CTransformation::RotateScaled (CFixVector& dest, const CFixVector& src) 
{
dest = m_info.view [0] * src;
return dest;
}

//------------------------------------------------------------------------------
//compute aspect ratio for this canvas
void CTransformation::ComputeAspect (int nWidth, int nHeight)
{
fix s = FixMulDiv (screen.Aspect (), (nWidth > 0) ? nWidth : CCanvas::Current ()->Height (), (nHeight > 0) ? nHeight : CCanvas::Current ()->Width ());
if (s <= I2X (1)) {	   //scale x
	m_info.aspect.v.coord.x = s;
	m_info.aspect.v.coord.y = I2X (1);
	}
else {
	m_info.aspect.v.coord.y = FixDiv (I2X (1), s);
	m_info.aspect.v.coord.x = I2X (1);
	}
m_info.aspect.v.coord.z = I2X (1);		//always 1
}

//------------------------------------------------------------------------------

void CTransformation::SetupProjection (float aspectRatio)
{
m_info.oglProjection.Get (GL_PROJECTION_MATRIX);
glMatrixMode (GL_MODELVIEW);
glPushMatrix ();
glLoadIdentity ();
m_info.oglModelview.Get (GL_MODELVIEW_MATRIX);
glPopMatrix ();
glGetIntegerv (GL_VIEWPORT, m_info.oglViewport);
#if 1
glGetFloatv (GL_PROJECTION_MATRIX, (GLfloat*) m_info.projection.m.vec);
#else
memcpy (m_info.projection.m.vec, &m_info.oglProjection [0], sizeof (m_info.projection.m.vec));
#endif
m_info.projection.Flip ();
m_info.aspectRatio = aspectRatio;
}

//------------------------------------------------------------------------------

ubyte CTransformation::Codes (CFixVector& v)
{
	ubyte codes = (v.v.coord.z < 0) ? CC_BEHIND : 0;
#if 1
	tScreenPos s;
	ProjectPoint (v, s);
	if (s.x < 0)
		codes |= CC_OFF_LEFT;
	else if (s.x > screen.Width ())
		codes |= CC_OFF_RIGHT;
	if (s.y < 0)
		codes |= CC_OFF_BOT;
	else if (s.y > screen.Height ())
		codes |= CC_OFF_TOP;
#else
	fix z = v.v.coord.z;
	fix r = fix (m_info.zoom * m_info.aspectRatio);
	fix x = FixMulDiv (v.v.coord.x, m_info.scale.v.coord.x, r);

	if (x > z)
		codes |= CC_OFF_RIGHT;
	else if (x < -z)
		codes |= CC_OFF_LEFT;
	if (v.v.coord.y > z)
		codes |= CC_OFF_TOP;
	else if (v.v.coord.y < -z)
		codes |= CC_OFF_BOT;
#endif
	return codes;
	}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#define COMPUTE_TYPE 0

static int planeVerts [6][4] = {
	{0,1,2,3},{0,1,5,4},{1,2,6,5},{2,3,7,6},{0,3,7,4},{4,5,6,7}
	};

//static int oppSides [6] = {5, 3, 4, 1, 2, 0};

static int normRefs [6][2] = {{1,5},{4,7},{5,4},{7,4},{4,5},{5,1}};

void CFrustum::Compute (void)
{
	int i;

#if COMPUTE_TYPE == 1

// Note: WNEARHALF = ZNEAR * tan(LIGHTFOV * (PI / 180.0) / 2.0);
//       WFARHALF = ZFAR * tan(LIGHTFOV * (PI / 180.0) / 2.0);

float h = float (tan (gameStates.render.glFOV * X2D (transformation.m_info.zoom) * Pi / 360.0));
float wNearHalf = float (ZNEAR) * h;
float wFarHalf = float (ZFAR) * h;

// Calculate the light direction vector components.
CFloatVector &f = transformation.m_info.viewf [0].m.dir.f;
CFloatVector &r = transformation.m_info.viewf [0].m.dir.r;
CFloatVector &u = transformation.m_info.viewf [0].m.dir.u;

// Calculate vector fc (vector from viewer to center of far frustum plane.
CFloatVector hr = r * wFarHalf;
CFloatVector hu = u * wFarHalf;
CFloatVector fc = f * ZFAR;
// Calculate vertex of far top left frustum plane.
//CFloatVector ftl = fc + hu - hr;
//CFloatVector ftr = fc + hu + hr;
//CFloatVector fbl = fc - hu - hr;
//CFloatVector fbr = fc - hu + hr;
CFloatVector ftl = fc + hu;
CFloatVector ftr = ftl;
ftl -= hr;
ftr += hr;
CFloatVector fbl = fc - hu;
CFloatVector fbr = fbl;
fbl -= hr;
fbr += hr;

// Calculate vector nc (vector from viewer to center of near frustum plane.
hr = r * wNearHalf;
hu = u * wNearHalf;
CFloatVector nc = f * ZNEAR;
//CFloatVector ntl = nc + hu - hr;
//CFloatVector ntr = nc + hu + hr;
//CFloatVector nbl = nc - hu - hr;
//CFloatVector nbr = nc - hu + hr;
CFloatVector ntl = nc + hu;
CFloatVector ntr = ntl;
ntl -= hr;
ntr += hr;
CFloatVector nbl = nc - hu;
CFloatVector nbr = nbl;
nbl -= hr;
nbr += hr;

m_corners [0].Assign (nbl);
m_corners [1].Assign (ntl);
m_corners [2].Assign (ntr);
m_corners [3].Assign (nbr);
m_corners [4].Assign (fbl);
m_corners [5].Assign (ftl);
m_corners [6].Assign (ftr);
m_corners [7].Assign (fbr);

#else

// compute the frustum. ZNEAR is 0.0 here!

float h = float (tan (gameStates.render.glFOV * X2D (transformation.m_info.zoom) * Pi / 360.0));
float w = float (h * CCanvas::Current ()->AspectRatio ());
float n = float (ZNEAR);
float f = float (ZFAR);
float m = f * 0.5f;
float r = f / n;

#define ln -w
#define rn +w
#define tn +h
#define bn -h

#define lf (ln * r)
#define rf (rn * r)
#define tf (tn * r)
#define bf (bn * r)

	CFloatVector corners [8] = {
#if 1
		{{{0.0f, 0.0f, 0.0f}}}, {{{0.0f, 0.0f, 0.0f}}}, {{{0.0f, 0.0f, 0.0f}}}, {{{0.0f, 0.0f, 0.0f}}},
#else
		{{{ln, bn, n}}}, {{{ln, tn, n}}}, {{{rn, tn, n}}}, {{{rn, bn, n}}},
#endif
		{{{lf, bf, f}}}, {{{lf, tf, f}}}, {{{rf, tf, f}}}, {{{rf, bf, f}}}
		};

	static CFloatVector centers [6] = {
#if 1
		{{{0.0f, 0.0f, 0.0f}}},
#else
		{{{0.0f, 0.0f, n}}},
#endif
		{{{lf * 0.5f, 0.0f, m}}}, {{{0.0f, tf * 0.5f, m}}}, {{{rf * 0.5f, 0.0f, m}}}, {{{0.0f, bf * 0.5f, m}}}, {{{0.0f, 0.0f, f}}}
		};

for (i = 0; i < 8; i++)
	m_corners [i].Assign (corners [i]);
for (i = 0; i < 6; i++)
	m_centers [i].Assign (centers [i]);

#endif

m_centers [0].SetZero ();
m_normals [0].Set (0, 0, I2X (1));
for (i = 1; i < 6; i++) {
	m_normals [i] = CFixVector::Normal (m_corners [planeVerts [i][1]], m_corners [planeVerts [i][2]], m_corners [planeVerts [i][3]]);
	m_centers [i].Assign (centers [i]);
#if COMPUTE_TYPE == 1
#else
	for (int j = 0; j < 4; j++)
		m_centers [i] += m_corners [planeVerts [i][j]];
	m_centers [i] /= I2X (4);
#endif
#if 1
	CFixVector v = m_corners [normRefs [i][1]] - m_corners [normRefs [i][0]];
	CFixVector::Normalize (v);
	if (CFixVector::Dot (v, m_normals [i]) < 0)
		m_normals [i].Neg ();
#endif
	}
}

//------------------------------------------------------------------------------
// Check whether the frustum intersects with a face defined by side *sideP.

bool CFrustum::Contains (CSide* sideP)
{
	static int lineVerts [12][2] = {
		{0,1}, {1,2}, {2,3}, {3,0}, 
		{4,5}, {5,6}, {6,7}, {7,4},
		{0,4}, {1,5}, {2,6}, {3,7}
	};

	int i, j, nInside = 0, nOutside [4] = {0, 0, 0, 0};
	CRenderPoint* points [4];
	CFixVector intersection;

for (j = 0; j < 4; j++) {
	points [j] = &RENDERPOINTS [sideP->m_corners [j]];
	if (!(points [j]->Projected ()))
		points [j]->Transform (sideP->m_corners [j]);
	}

// check whether all vertices of the face are at the back side of at least one frustum plane,
// or if at least one is at at least on one frustum plane's front side
for (i = 0; i < 6; i++) {
	int nPtInside = 4;
	int bPtInside = 1;
	CFixVector& c = m_centers [i];
	CFixVector& n = m_normals [i];
	for (j = 0; j < 4; j++) {
		CFixVector v = points [j]->ViewPos () - c;
		CFixVector::Normalize (v);
		if (CFixVector::Dot (n, v) < 0) {
			if (!--nPtInside)
				return false;
			++nOutside [j];
			bPtInside = 0;
			}
		}
	nInside += bPtInside;
	}

if (nInside == 6)
	return true; // face completely contained
for (j = 0; j < 4; j++) 
	if (!nOutside [j])
		return true; // some vertex inside frustum

if (sideP->m_nFaces == 2) {
	points [1] = &RENDERPOINTS [sideP->m_vertices [3]];
	if (!points [1]->Projected ())
		points [1]->Transform (sideP->m_vertices [1]);
	}

// check whether the frustum intersects with the face
// to do that, compute the intersections of all frustum edges with the plane(s) spanned up by the face (two planes if face not planar)
// if an edge intersects, check whether the intersection is inside the face
// since the near plane is at 0.0, only 8 edges of 5 planes need to be checked
gameStates.render.bRendering = 1; // make sure CSide::PointToFaceRelation uses the transformed vertices
for (j = 0; j < sideP->m_nFaces; j++) 
	transformation.Rotate (sideP->m_rotNorms [j], sideP->m_normals [j], 0);
for (i = 11; i >= 4; i--) {
	for (j = 0; j < sideP->m_nFaces; j++) {
		if (!FindPlaneLineIntersection (intersection, &points [j]->ViewPos (), &sideP->m_rotNorms [j],
												  &m_corners [lineVerts [i][0]], &m_corners [lineVerts [i][1]], 0, false))
			continue;
		if (!sideP->PointToFaceRelation (intersection, j, sideP->m_rotNorms [j])) {
			gameStates.render.bRendering = 0;
			return true;
			}
		}
	}
gameStates.render.bRendering = 0;
return false;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//eof

