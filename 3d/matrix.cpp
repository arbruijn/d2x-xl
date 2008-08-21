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

#include "inferno.h"
#include "3d.h"
#include "globvars.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "oof.h"

void ScaleMatrix (int bOglScale);

//------------------------------------------------------------------------------
//set view from x,y,z & p,b,h, xZoom.  Must call one of g3_setView_*()
void G3SetViewAngles (vmsVector *vPos, vmsAngVec *mOrient, fix xZoom)
{
viewInfo.zoom = xZoom;
viewInfo.pos = *vPos;
viewInfo.view[0] = vmsMatrix::Create(*mOrient);
viewInfo.posf = viewInfo.pos.toFloat();
viewInfo.viewf[0] = viewInfo.view[0].toFloat();
ScaleMatrix (1);
}

//------------------------------------------------------------------------------
//set view from x,y,z, viewer matrix, and xZoom.  Must call one of g3_setView_*()
void G3SetViewMatrix (const vmsVector& vPos, const vmsMatrix& mOrient, fix xZoom, int bOglScale)
{
viewInfo.zoom = xZoom;
viewInfo.glZoom = (float) xZoom / 65536.0f;

	viewInfo.pos = vPos;
	viewInfo.posf = viewInfo.pos.toFloat();
	OOF_VecVms2Gl (viewInfo.glPosf, viewInfo.pos);

	viewInfo.view [0] = mOrient;
	viewInfo.viewf[0] = viewInfo.view[0].toFloat();
	OOF_MatVms2Gl (OOF_GlIdent (viewInfo.glViewf), viewInfo.view[0]);

ScaleMatrix (bOglScale);
OglSetFOV ();
}

//------------------------------------------------------------------------------
//performs aspect scaling on global view matrix
void ScaleMatrix (int bOglScale)
{
	viewInfo.view [1] = viewInfo.view [0];		//so we can use unscaled if we want
	viewInfo.viewf [1] = viewInfo.viewf [0];		//so we can use unscaled if we want

viewInfo.scale = viewInfo.windowScale;
if (viewInfo.zoom <= f1_0) 		//xZoom in by scaling z
	viewInfo.scale[Z] = FixMul (viewInfo.scale[Z], viewInfo.zoom);
else {			//xZoom out by scaling x&y
	fix s = FixDiv (f1_0, viewInfo.zoom);

	viewInfo.scale[X] = FixMul (viewInfo.scale[X], s);
	viewInfo.scale[Y] = FixMul (viewInfo.scale[Y], s);
	}
viewInfo.scalef = viewInfo.scale.toFloat();
//viewInfo.scale.p.x = viewInfo.scale.p.y = viewInfo.scale.p.z = F1_0;
//now scale matrix elements
if (bOglScale) {
	//glScalef (f2fl (viewInfo.scale.p.x), f2fl (viewInfo.scale.p.y), -f2fl (viewInfo.scale.p.z));
	glScalef (1, 1, -1);
	}
else {
	//VmVecScale (&viewInfo.view [0].rVec, viewInfo.scale.p.x);
	//VmVecScale (&viewInfo.view [0].uVec, viewInfo.scale.p.y);
	//viewInfo.scale.p.x = viewInfo.scale.p.y = viewInfo.scale.p.z = F1_0;
	viewInfo.view[0][FVEC] *= (-viewInfo.scale[Z]);
	glScalef (1, 1, 1);
	}
viewInfo.viewf[0] = viewInfo.view[0].toFloat();
}

//------------------------------------------------------------------------------
//eof
