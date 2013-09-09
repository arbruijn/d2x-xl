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

#include "descent.h"
#include "3d.h"
#include "globvars.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "oof.h"

void ScaleTransformation (CTransformation& transformation, int bOglScale);

//------------------------------------------------------------------------------
//set view from x,y,z & p,b,h, xZoom.  Must call one of g3_setView_*()
void SetupViewAngles (CFixVector *vPos, CAngleVector *mOrient, fix xZoom)
{
transformation.m_info.zoom = xZoom;
transformation.m_info.pos = *vPos;
transformation.m_info.view [0] = CFixMatrix::Create (*mOrient);
transformation.m_info.posf [0].Assign (transformation.m_info.pos);
transformation.m_info.viewf [0].Assign (transformation.m_info.view [0]);
ScaleTransformation (transformation, 1);
}

//------------------------------------------------------------------------------
//set view from x,y,z, viewer matrix, and xZoom.  Must call one of g3_setView_*()

#define ZSCREEN 10.0

void SetupTransformation (CTransformation& transformation, const CFixVector& vPos, const CFixMatrix& mOrient, fix xZoom, int bOglScale, fix xStereoSeparation, bool bSetupRenderer)
{
transformation.m_info.zoom = ogl.IsOculusRift () ? 3 * xZoom / 2 : xZoom;
transformation.m_info.zoomf = (float) xZoom / 65536.0f;
transformation.m_info.pos = vPos;
transformation.m_info.posf [0].Assign (transformation.m_info.pos);
transformation.m_info.posf [1].Assign (transformation.m_info.pos);
if (!ogl.StereoSeparation ()) 
	transformation.m_info.view [0] = mOrient;
else if (ogl.IsOculusRift ()) {
	//TODO: Adjust the orientation (view) matrix properly for left/right eye coordinate transformation
	transformation.m_info.view [0] = mOrient;
	}
else if (gameOpts->render.stereo.nMethod == STEREO_TOE_IN) {
	fix zScreen = F2X (ogl.ZScreen () * 10.0);
	CFixVector o = CFixVector::Create (fix (xStereoSeparation / 2), 0, 0);
	CFixVector h = CFixVector::Create (fix (xStereoSeparation / 2), 0, zScreen);
	CFixVector::Normalize (o);
	CFixVector::Normalize (h);
	fix d = CFixVector::Dot (h, o);
	CAngleVector a = CAngleVector::Create (0, 0, (xStereoSeparation < 0) ? d : -d);
	CFixMatrix r = CFixMatrix::Create (a);
	transformation.m_info.view [0] = const_cast<CFixMatrix&> (mOrient) * r;
	}
else
	transformation.m_info.view [0] = mOrient;
transformation.m_info.viewf [0].Assign (transformation.m_info.view [0]);
ScaleTransformation (transformation, bOglScale);
CFixMatrix::Transpose (transformation.m_info.viewf [2], transformation.m_info.view [0]);
if (bSetupRenderer)
	ogl.SetupProjection (transformation);
}

//------------------------------------------------------------------------------
//performs aspect scaling on global view matrix
void ScaleTransformation (CTransformation& transformation, int bOglScale)
{
	transformation.m_info.view [1] = transformation.m_info.view [0];		//so we can use unscaled if we want
	transformation.m_info.viewf [1] = transformation.m_info.viewf [0];		//so we can use unscaled if we want

transformation.m_info.scale = transformation.m_info.aspect;
if (transformation.m_info.zoom <= I2X (1)) 		//xZoom in by scaling z
	transformation.m_info.scale.v.coord.z = FixMul (transformation.m_info.scale.v.coord.z, transformation.m_info.zoom);
else {			//zoom out by scaling x&y
	fix s = FixDiv (I2X (1), transformation.m_info.zoom);

	transformation.m_info.scale.v.coord.x = FixMul (transformation.m_info.scale.v.coord.x, s);
	transformation.m_info.scale.v.coord.y = FixMul (transformation.m_info.scale.v.coord.y, s);
	}
transformation.m_info.scalef.Assign (transformation.m_info.scale);
//transformation.m_info.scale.v.c.x = transformation.m_info.scale.v.c.y = transformation.m_info.scale.v.c.z = I2X (1);
//now scale matrix elements
if (bOglScale > 0) {
#if 0
	glScalef (transformation.m_info.scalef.dir.coord.x, transformation.m_info.scalef.dir.coord.y, -transformation.m_info.scalef.dir.coord.z);
#else
	glScalef (1, 1, -1);
#endif
	}
else {
	transformation.m_info.view [0].m.dir.r *= (transformation.m_info.scale.v.coord.x);
	transformation.m_info.view [0].m.dir.u *= (transformation.m_info.scale.v.coord.y);
	transformation.m_info.view [0].m.dir.f *= (transformation.m_info.scale.v.coord.z);
	transformation.m_info.viewf [0].Assign (transformation.m_info.view [0]);
	if (!bOglScale)
		glScalef (1, 1, -1);
	}
}

//------------------------------------------------------------------------------
//eof
