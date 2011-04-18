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
void CTransformation::ComputeAspect (void)
{
fix s = FixMulDiv (screen.Aspect (), CCanvas::Current ()->Height (), CCanvas::Current ()->Width ());
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
glGetFloatv (GL_PROJECTION_MATRIX, (GLfloat*) m_info.projection.m.vec);
m_info.projection.Flip ();
m_info.aspectRatio = aspectRatio;
}

//------------------------------------------------------------------------------
//eof
