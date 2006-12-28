/* $Id: instance.c,v 1.4 2002/07/17 21:55:19 bradleyb Exp $ */
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
#include "error.h"

#include "3d.h"
#include "globvars.h"
#include "inferno.h"
#include "ogl_init.h"
#include "vecmat.h"
#include "oof.h"

#define MAX_INSTANCE_DEPTH	10

struct instance_context {
	vmsMatrix m [2];
	vmsVector p;
} instanceStack[MAX_INSTANCE_DEPTH];

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

void VmsMove (vmsVector *pv)
{
glVectorf p;
OglMove (OOF_VecVms2Gl (p, pv));
}

//------------------------------------------------------------------------------

inline void VmsRot (vmsMatrix *pm)
{
glMatrixf m;
if (nInstanceDepth)
	OglRot (OOF_MatVms2Gl (OOF_GlIdent (m), pm));
else
	OglRot (OOF_GlInverse (NULL, OOF_MatVms2Gl (OOF_GlIdent (m), pm)));
}

//------------------------------------------------------------------------------

int G3PushMatrix (void)
{
if (nInstanceDepth >= MAX_INSTANCE_DEPTH)
	return 0;
memcpy (instanceStack [nInstanceDepth].m, viewInfo.view, 2 * sizeof (vmsMatrix));
instanceStack [nInstanceDepth].p = viewInfo.pos;
nInstanceDepth++;
return 1;
}

//------------------------------------------------------------------------------

int G3PopMatrix (void)
{
if (nInstanceDepth <= 0)
	return 0;
nInstanceDepth--;
viewInfo.pos = instanceStack [nInstanceDepth].p;
memcpy (viewInfo.view, instanceStack [nInstanceDepth].m, 2 * sizeof (vmsMatrix));
return 1;
}

//------------------------------------------------------------------------------
//instance at specified point with specified orientation
//if matrix==NULL, don't modify matrix.  This will be like doing an offset   
void G3StartInstanceMatrix (vmsVector *vPos, vmsMatrix *mOrient)
{
if (gameStates.ogl.bUseTransform) {
	vmsVector	h;

	glMatrixMode (GL_MODELVIEW);
	glPushMatrix ();
	if (!nInstanceDepth) {
		glLoadIdentity ();
		glScalef (f2fl (viewInfo.scale.p.x), f2fl (viewInfo.scale.p.y), -f2fl (viewInfo.scale.p.z));
		OglRot (viewInfo.glViewf);
		VmVecSub (&h, &viewInfo.pos, vPos);
		VmsMove (&h);
		}
	else {
		glScalef (-1.0f, -1.0f, 1.0f);
		VmsMove (vPos);
		}
	if (mOrient)
		VmsRot (mOrient);
	if (nInstanceDepth)
		glScalef (-1.0f, -1.0f, 1.0f);
	else
		glScalef (1.0f, 1.0f, -1.0f);
	}
	{
	vmsVector	vOffs;
	vmsMatrix	mTrans, mRot;

	//Assert (nInstanceDepth < MAX_INSTANCE_DEPTH);
	if (!G3PushMatrix ())
		return;
	//step 1: subtract object position from view position
	VmVecSub (&vOffs, &viewInfo.pos, vPos);
	if (mOrient) {
		int i;
		//step 2: rotate view vector through tObject matrix
		VmVecRotate (&viewInfo.pos, &vOffs, mOrient);
		//step 3: rotate tObject matrix through view_matrix (vm = ob * vm)
		VmCopyTransposeMatrix (&mTrans, mOrient);
		for (i = 0; i < 2; i++) {
			VmMatMul (&mRot, &mTrans, viewInfo.view + i);
			viewInfo.view [i] = mRot;
			VmsMatToFloat (viewInfo.viewf + i, viewInfo.view + i);
			}
		}
	VmsVecToFloat (&viewInfo.posf, &viewInfo.pos);
	}
}

//------------------------------------------------------------------------------
//instance at specified point with specified orientation
//if angles==NULL, don't modify matrix.  This will be like doing an offset
void G3StartInstanceAngles (vmsVector *pos, vmsAngVec *angles)
{
	vmsMatrix tm;

if (!angles) {
	G3StartInstanceMatrix (pos, NULL);
	return;
	}
VmAngles2Matrix (&tm, angles);
G3StartInstanceMatrix (pos, &tm);
}

//------------------------------------------------------------------------------
//pops the old context
void G3DoneInstance ()
{
if (!G3PopMatrix ())
	return;
if (gameStates.ogl.bUseTransform) {
	glMatrixMode (GL_MODELVIEW);
	glPopMatrix ();
	}
VmsVecToFloat (&viewInfo.posf, &viewInfo.pos);
VmsMatToFloat (viewInfo.viewf, viewInfo.view);
}

//------------------------------------------------------------------------------
//eof
