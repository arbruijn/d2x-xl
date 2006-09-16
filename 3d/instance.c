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
/*
 * 
 * Instancing routines
 * 
 * Old Log:
 *
 * Revision 1.2  1995/06/12  12:36:57  allender
 * fixed bug where G3StartInstanceAngles recursively called itself
 *
 * Revision 1.1  1995/05/05  08:51:27  allender
 * Initial revision
 *
 * Revision 1.1  1995/04/17  06:43:29  matt
 * Initial revision
 * 
 * 
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
#include "oof.h"

#define MAX_INSTANCE_DEPTH	5

struct instance_context {
	vms_matrix m;
	vms_vector p;
} instanceStack[MAX_INSTANCE_DEPTH];

static int nInstanceDepth = 0;

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

void VmsMove (vms_vector *pv)
{
glVectorf p;
OglMove (OOF_VecVms2Gl (p, pv));
}

//------------------------------------------------------------------------------

inline void VmsRot (vms_matrix *pm)
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
void G3StartInstanceMatrix(vms_vector *pos, vms_matrix *orient)
{
if (gameStates.render.bGlTransform) {
	vms_vector	h;

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
	vms_vector tempv;
	vms_matrix tempm,tempm2;

#ifdef D1XD3D
	Win32_start_instance_matrix (pos, orient);
#endif
	Assert (nInstanceDepth < MAX_INSTANCE_DEPTH);
	instanceStack [nInstanceDepth].m = viewInfo.view;
	instanceStack [nInstanceDepth].p = viewInfo.position;
	nInstanceDepth++;
	//step 1: subtract object position from view position
	VmVecSub (&tempv, &viewInfo.position, pos);
	if (orient) {
		//step 2: rotate view vector through object matrix
		VmVecRotate (&viewInfo.position, &tempv, orient);
		//step 3: rotate object matrix through view_matrix (vm = ob * vm)
		VmCopyTransposeMatrix (&tempm2, orient);
		VmMatMul (&tempm, &tempm2, &viewInfo.view);
		viewInfo.view = tempm;
		}
	}
}

//------------------------------------------------------------------------------
//instance at specified point with specified orientation
//if angles==NULL, don't modify matrix.  This will be like doing an offset
void G3StartInstanceAngles(vms_vector *pos,vms_angvec *angles)
{
	vms_matrix tm;

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
if (gameStates.render.bGlTransform) {
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
	}
}

//------------------------------------------------------------------------------
//eof
