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

#include "inferno.h"
#include "error.h"
#include "3d.h"
#include "globvars.h"
#include "ogl_defs.h"
#include "vecmat.h"
#include "interp.h"
#include "oof.h"

#define MAX_INSTANCE_DEPTH	10

struct CViewInfo instanceStack [MAX_INSTANCE_DEPTH];

int nInstanceDepth = 0;

//------------------------------------------------------------------------------

inline void OglMove (float *pv)
{
glTranslatef (-pv [0], -pv [1], -pv [2]);
}

//------------------------------------------------------------------------------

inline void OglRot (float *pm)
{
glMultMatrixf (pm);
}

//------------------------------------------------------------------------------

void VmsMove(const CFixVector& pv)
{
glVectorf p;
OglMove (OOF_VecVms2Gl (p, pv));
}

//------------------------------------------------------------------------------

inline void VmsRot(const CFixMatrix& pm)
{
glMatrixf m;

memset (m, 0, sizeof (m));
m [0] = X2F (pm.RVec ()[X]);
m [1] = X2F (pm.RVec ()[Y]);
m [2] = X2F (pm.RVec ()[Z]);
m [4] = X2F (pm.UVec ()[X]);
m [5] = X2F (pm.UVec ()[Y]);
m [6] = X2F (pm.UVec ()[Z]);
m [8] = X2F (pm.FVec ()[X]);
m [9] = X2F (pm.FVec ()[Y]);
m [10] = X2F (pm.FVec ()[Z]);
m [15] = 1;
OglRot (m);
}

//------------------------------------------------------------------------------

int G3PushMatrix (void)
{
if (nInstanceDepth >= MAX_INSTANCE_DEPTH)
	return 0;
instanceStack [nInstanceDepth++] = viewInfo;
#if 0
glMatrixMode (GL_PROJECTION);
glPushMatrix ();
#endif
glMatrixMode (GL_MODELVIEW);
glPushMatrix ();
return 1;
}

//------------------------------------------------------------------------------

int G3PopMatrix (void)
{
if (nInstanceDepth <= 0)
	return 0;
viewInfo = instanceStack [--nInstanceDepth];
#if 0
glMatrixMode (GL_PROJECTION);
glPopMatrix ();
#endif
glMatrixMode (GL_MODELVIEW);
glPopMatrix ();
return 1;
}

//------------------------------------------------------------------------------
//instance at specified point with specified orientation
//if matrix==NULL, don't modify matrix.  This will be like doing an offset
void G3StartInstanceMatrix(const CFixVector& vPos, const CFixMatrix& mOrient)
{
	CFixVector	vOffs;
	CFixMatrix	mTrans, mRot;

//Assert (nInstanceDepth < MAX_INSTANCE_DEPTH);
if (!G3PushMatrix ())
	return;
if (gameStates.ogl.bUseTransform) {
	CFixVector	h;

	if (nInstanceDepth > 1) {
		glScalef (-1.0f, -1.0f, -1.0f);
		VmsMove (vPos);
		glScalef (-1.0f, -1.0f, -1.0f);
		VmsRot (mOrient);
	}
	else {
		glLoadIdentity ();
		//glScalef (X2F (viewInfo.scale.p.x), X2F (viewInfo.scale.p.y), -X2F (viewInfo.scale.p.z));
		glScalef (1, 1, -1);
		OglRot (viewInfo.glViewf);
		h = viewInfo.pos - vPos;
		VmsMove (h);
		VmsRot (mOrient);
		if (!gameData.models.vScale.IsZero ()) {
			CFloatVector fScale;
			fScale.Assign (gameData.models.vScale);
			glScalef (fScale [X], fScale [Y], fScale [Z]);
			}
		}
	}

//step 1: subtract object position from view position
vOffs = viewInfo.pos - vPos;

	int i;
	//step 2: rotate view vector through CObject matrix
	viewInfo.pos = mOrient * vOffs;
	//step 3: rotate CObject matrix through view_matrix (vm = ob * vm)
	mTrans = mOrient.Transpose ();
	for (i = 0; i < 2; i++) {
		mRot = mTrans * viewInfo.view [i];
		viewInfo.view [i] = mRot;
		viewInfo.viewf [i].Assign (viewInfo.view [i]);
		}

viewInfo.posf.Assign (viewInfo.pos);
}

//------------------------------------------------------------------------------
//instance at specified point with specified orientation
//if angles==NULL, don't modify matrix.  This will be like doing an offset
void G3StartInstanceAngles (const CFixVector& pos, const CAngleVector& angles) 
{
G3StartInstanceMatrix(pos, CFixMatrix::Create(angles));
}

//------------------------------------------------------------------------------
//pops the old context
void G3DoneInstance ()
{
if (!G3PopMatrix ())
	return;
#if 0
if (gameStates.ogl.bUseTransform) {
	glMatrixMode (GL_MODELVIEW);
	glPopMatrix ();
	}
viewInfo.posf.Assign (viewInfo.pos);
viewInfo.viewf.Assign (viewInfo.view);
#endif
}

//------------------------------------------------------------------------------
//eof
