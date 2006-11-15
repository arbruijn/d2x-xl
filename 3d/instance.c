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
	vmsMatrix m;
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
//instance at specified point with specified orientation
//if matrix==NULL, don't modify matrix.  This will be like doing an offset   
void G3StartInstanceMatrix (vmsVector *pos, vmsMatrix *orient)
{
if (gameStates.ogl.bUseTransform) {
	vmsVector	h;

	glMatrixMode (GL_MODELVIEW);
	glPushMatrix ();
	if (!nInstanceDepth) {
		glLoadIdentity ();
		glScalef (1.0f, 1.0f, -viewInfo.glZoom);
		OglRot (viewInfo.glViewf);
		VmVecSub (&h, &viewInfo.position, pos);
		VmsMove (&h);
		}
	else {
		glScalef (-1.0f, -1.0f, 1.0f);
		VmsMove (pos);
		}
	if (orient)
		VmsRot (orient);
	if (nInstanceDepth)
		glScalef (-1.0f, -1.0f, 1.0f);
	else
		glScalef (1.0f, 1.0f, -1.0f);
	}
	{
	vmsVector tempv;
	vmsMatrix tempm,tempm2;

#ifdef D1XD3D
	Win32_start_instance_matrix (pos, orient);
#endif
	//Assert (nInstanceDepth < MAX_INSTANCE_DEPTH);
	if (nInstanceDepth >= MAX_INSTANCE_DEPTH)
		nInstanceDepth = nInstanceDepth;
	instanceStack [nInstanceDepth].m = viewInfo.view;
	instanceStack [nInstanceDepth].p = viewInfo.position;
	nInstanceDepth++;
	//step 1: subtract tObject position from view position
	VmVecSub (&tempv, &viewInfo.position, pos);
	if (orient) {
		//step 2: rotate view vector through tObject matrix
		VmVecRotate (&viewInfo.position, &tempv, orient);
		//step 3: rotate tObject matrix through view_matrix (vm = ob * vm)
		VmCopyTransposeMatrix (&tempm2, orient);
		VmMatMul (&tempm, &tempm2, &viewInfo.view);
		viewInfo.view = tempm;
		VmsVecToFloat (&viewInfo.posf, &viewInfo.position);
		VmsMatToFloat (&viewInfo.viewf, &viewInfo.view);
		}
	}
}

//------------------------------------------------------------------------------
//instance at specified point with specified orientation
//if angles==NULL, don't modify matrix.  This will be like doing an offset
void G3StartInstanceAngles(vmsVector *pos,vmsAngVec *angles)
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
void G3DoneInstance()
{
if (gameStates.ogl.bUseTransform) {
	glMatrixMode (GL_MODELVIEW);
	glPopMatrix ();
	}
	{
#ifdef D1XD3D
	Win32_done_instance ();
#endif
	nInstanceDepth--;
	Assert(nInstanceDepth >= 0);
	viewInfo.position = instanceStack [nInstanceDepth].p;
	viewInfo.view = instanceStack [nInstanceDepth].m;
	VmsVecToFloat (&viewInfo.posf, &viewInfo.position);
	VmsMatToFloat (&viewInfo.viewf, &viewInfo.view);
	}
}

//------------------------------------------------------------------------------
//eof
