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

CTransformation instanceStack [MAX_INSTANCE_DEPTH];

int nInstanceDepth = 0;

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
m_info.m_save.Create (MAX_INSTANCE_DEPTH);
}

//------------------------------------------------------------------------------

bool CTransformation::Push (void)
{
if (m_save.Length () >= MAX_INSTANCE_DEPTH)
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
m_save.Pop (m_info);
glMatrixMode (GL_MODELVIEW);
glPopMatrix ();
return this;
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
if (gameStates.ogl.bUseTransform) {
	CFixVector	h;

	if (nInstanceDepth > 1) {
		glScalef (-1.0f, -1.0f, -1.0f);
		transformation.Move (vPos);
		glScalef (-1.0f, -1.0f, -1.0f);
		transformation.Rotate (mOrient);
	}
	else {
		glLoadIdentity ();
		//glScalef (X2F (transformation.m_info.scale.p.x), X2F (transformation.m_info.scale.p.y), -X2F (transformation.m_info.scale.p.z));
		glScalef (1, 1, -1);
		transformation.Rotate (transformation.m_info.viewf [2]);
		h = transformation.m_info.pos - vPos;
		transformation.Move (h);
		transformation.Rotate (mOrient);
		if (!gameData.models.vScale.IsZero ()) {
			CFloatVector fScale;
			fScale.Assign (gameData.models.vScale);
			glScalef (fScale [X], fScale [Y], fScale [Z]);
			}
		}
	}

//step 1: subtract object position from view position
vOffs = transformation.m_info.pos - vPos;

	int i;
	//step 2: rotate view vector through CObject matrix
	transformation.m_info.pos = mOrient * vOffs;
	//step 3: rotate CObject matrix through view_matrix (vm = ob * vm)
	mTrans = mOrient.Transpose ();
	for (i = 0; i < 2; i++) {
		mRot = mTrans * transformation.m_info.view [i];
		transformation.m_info.view [i] = mRot;
		transformation.m_info.viewf [i].Assign (transformation.m_info.view [i]);
		}

transformation.m_info.posf [0].Assign (transformation.m_info.pos);
}

//------------------------------------------------------------------------------
//pops the old context
void CTransformation::End (void)
{
Pop ();
}

//------------------------------------------------------------------------------
//eof
