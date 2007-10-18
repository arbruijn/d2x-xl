/* $Id: render.c, v 1.18 2003/10/10 09:36:35 btb Exp $ */
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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "inferno.h"
#include "ogl_init.h"
#include "gameseg.h"
#include "shadows.h"
#include "objrender.h"
#include "render.h"
#include "renderlib.h"
#include "rendershadows.h"

//------------------------------------------------------------------------------

#define SOFT_SHADOWS		0

//------------------------------------------------------------------------------

void RenderFaceShadow (tFaceProps *propsP)
{
	int			i, nVertices = propsP->nVertices;
	g3sPoint		*p;
	tOOF_vector	v [9];

for (i = 0; i < nVertices; i++) {
	p = gameData.segs.points + propsP->vp [i];
	if (p->p3_index < 0)
		OOF_VecVms2Oof (v + i, &p->p3_vec);
	else
		memcpy (v + i, gameData.render.pVerts + p->p3_index, sizeof (tOOF_vector));
	}
v [nVertices] = v [0];
glEnableClientState (GL_VERTEX_ARRAY);
glVertexPointer (3, GL_FLOAT, 0, v);
#if DBG_SHADOWS
if (bShadowTest) {
	if (bFrontCap)
		glDrawArrays (GL_LINE_LOOP, 0, nVertices);
	}
else
#endif
glDrawArrays (GL_TRIANGLE_FAN, 0, nVertices);
#if DBG_SHADOWS
if (!bShadowTest || bShadowVolume)
#endif
for (i = 0; i < nVertices; i++)
	G3RenderShadowVolumeFace (v + i);
glDisableClientState (GL_VERTEX_ARRAY);
#if DBG_SHADOWS
if (!bShadowTest || bRearCap)
#endif
G3RenderFarShadowCapFace (v, nVertices);
}

//------------------------------------------------------------------------------

void RenderShadowQuad (int bWhite)
{
	static GLdouble shadowHue [2][4] = {{0, 0, 0, 0.6},{0, 0, 0, 1}};

glMatrixMode (GL_MODELVIEW);
glPushMatrix ();
glLoadIdentity ();
glMatrixMode (GL_PROJECTION);
glPushMatrix ();
glLoadIdentity ();
glOrtho (0, 1, 1, 0, 0, 1);
glDisable (GL_DEPTH_TEST);
glEnable (GL_STENCIL_TEST);
glDepthMask (0);
if (gameStates.render.nShadowBlurPass)
	glDisable (GL_BLEND);
else
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glDisable (GL_TEXTURE_2D);
glColor4dv (shadowHue [gameStates.render.nShadowBlurPass]);// / fDist);
glBegin (GL_QUADS);
glVertex2f (0,0);
glVertex2f (1,0);
glVertex2f (1,1);
glVertex2f (0,1);
glEnd ();
glEnable (GL_DEPTH_TEST);
glDisable (GL_STENCIL_TEST);
glDepthMask (1);
glPopMatrix ();
glMatrixMode (GL_MODELVIEW);
glPopMatrix ();
}

//------------------------------------------------------------------------------

#define STB_SIZE_X	2048
#define STB_SIZE_Y	2048

grsBitmap	shadowBuf;
ubyte			shadowTexBuf [STB_SIZE_X * STB_SIZE_Y * 4];
static int	bHaveShadowBuf = 0;

void CreateShadowTexture (void)
{
	GLint	i;

if (!bHaveShadowBuf) {
	memset (&shadowBuf, 0, sizeof (shadowBuf));
	shadowBuf.bmProps.w = STB_SIZE_X;
	shadowBuf.bmProps.h = STB_SIZE_Y;
	shadowBuf.bmProps.flags = (char) BM_FLAG_TGA;
	shadowBuf.bmTexBuf = shadowTexBuf;
	OglLoadBmTextureM (&shadowBuf, 0, -1, 0, NULL);
	bHaveShadowBuf = 1;
	}
#if 1
//glStencilFunc (GL_EQUAL, 0, ~0);
//RenderShadowQuad (1);
#	if 0
glCopyTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 0, grdCurCanv->cvBitmap.bmProps.h - 128, 128, 128, 0);
#	else
glCopyTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 0, 0,
						grdCurCanv->cvBitmap.bmProps.w, 
						grdCurCanv->cvBitmap.bmProps.h, 0);
#	endif
#else
glCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 0, 0, 128, 128);
#endif
i = glGetError ();
}

//------------------------------------------------------------------------------

GLhandleARB shadowProg = 0;
GLhandleARB shadowFS = 0; 
GLhandleARB shadowVS = 0; 

#if DBG_SHADERS

char *pszShadowFS = "shadows.frag";
char *pszShadowVS = "shadows.vert";

#else

char *pszShadowFS = "shadows.frag";
char *pszShadowVS = "shadows.vert";

#endif

void RenderShadowTexture (void)
{
if (!(shadowProg ||
	   (CreateShaderProg (&shadowProg) &&
	    CreateShaderFunc (&shadowProg, &shadowFS, &shadowVS, pszShadowFS, pszShadowVS, 1) &&
	    LinkShaderProg (&shadowProg))))
	return;
glMatrixMode (GL_MODELVIEW);
glPushMatrix ();
glLoadIdentity ();
glMatrixMode (GL_PROJECTION);
glPushMatrix ();
glLoadIdentity ();
glOrtho (0, 1, 1, 0, 0, 1);
glDisable (GL_DEPTH_TEST);
glDepthMask (0);
#if 1
glDisable (GL_BLEND);
#else
glEnable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
glEnable (GL_TEXTURE_2D);
OglActiveTexture (GL_TEXTURE0, 0);
if (OglBindBmTex (&shadowBuf, 0, 0))
	return;
glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#if 0
glUseProgramObject (shadowProg);
glUniform1i (glGetUniformLocation (shadowProg, "shadowTex"), 0);
#endif
glBegin (GL_QUADS);
glTexCoord2d (0,0);
glVertex2d (0,0);
glTexCoord2d (1,0);
glVertex2d (0.5,0);
glTexCoord2d (1,-1);
glVertex2d (0.5,0.5);
glTexCoord2d (0,-1);
glVertex2d (0,0.5);
glEnd ();
glUseProgramObject (0);
glEnable (GL_DEPTH_TEST);
glDepthMask (1);
glPopMatrix ();
glMatrixMode (GL_MODELVIEW);
glPopMatrix ();
}

//------------------------------------------------------------------------------

int RenderShadowMap (tDynLight *pLight)
{
	tCamera	*pc;

if (pLight->shadow.nFrame == gameData.render.shadows.nFrame)
	return 0;
if (gameData.render.shadows.nShadowMaps == MAX_SHADOW_MAPS)
	return 0;
pLight->shadow.nFrame = !pLight->shadow.nFrame;
gameStates.render.nShadowPass = 2;
pc = gameData.render.shadows.shadowMaps + gameData.render.shadows.nShadowMaps++;
CreateCamera (pc, pLight->nSegment, pLight->nSide, pLight->nSegment, pLight->nSide, NULL, 1, 0);
RenderCamera (pc);
gameStates.render.nShadowPass = 2;
return 1;
}

//------------------------------------------------------------------------------
//The following code is an attempt to find all objects that cast a shadow visible
//to the player. To accomplish that, for each robot the line of sight to each
//tSegment visible to the tPlayer is computed. If there is a los to any of these 
//segments, the tObject's shadow is rendered. Far from perfect solution though. :P

void RenderObjectShadows (void)
{
	tObject		*objP = gameData.objs.objects;
	int			i, j, bSee;
	tObject		fakePlayerPos = *gameData.objs.viewer;

for (i = 0; i <= gameData.objs.nLastObject; i++, objP++)
	if (objP == gameData.objs.console)
		RenderObject (objP, 0, 0);
	else if ((objP->nType == OBJ_PLAYER) || 
				(gameOpts->render.shadows.bRobots && (objP->nType == OBJ_ROBOT))) {
		for (j = gameData.render.mine.nRenderSegs; j--;) {
			fakePlayerPos.nSegment = gameData.render.mine.nSegRenderList [j];
			COMPUTE_SEGMENT_CENTER_I (&fakePlayerPos.position.vPos, fakePlayerPos.nSegment);
			bSee = ObjectToObjectVisibility (objP, &fakePlayerPos, FQ_TRANSWALL);
			if (bSee) {
				RenderObject (objP, 0, 0);
				break;
				}
			}
		}
}

//------------------------------------------------------------------------------

void DestroyShadowMaps (void)
{
	tCamera	*pc;

for (pc = gameData.render.shadows.shadowMaps; gameData.render.shadows.nShadowMaps; gameData.render.shadows.nShadowMaps--, pc++)
	DestroyCamera (pc);
}

//------------------------------------------------------------------------------

void ApplyShadowMaps (short nStartSeg, fix nEyeOffset, int nWindow)
{	
	static float mTexBiasf [] = {
		0.5f, 0.0f, 0.0f, 0.0f, 
		0.0f, 0.5f, 0.0f, 0.0f, 
		0.0f, 0.0f, 0.5f, 0.0f, 
		0.5f, 0.5f, 0.5f, 1.0f};

	static float mPlanef [] = {
		1.0f, 0.0f, 0.0f, 0.0f, 
		0.0f, 1.0f, 0.0f, 0.0f, 
		0.0f, 0.0f, 1.0f, 0.0f, 
		0.0f, 0.0f, 0.0f, 1.0f};

	static GLenum nTexCoord [] = {GL_S, GL_T, GL_R, GL_Q};

	float mProjectionf [16];
	float mModelViewf [16];

	int			i;
	tCamera		*pc;

#if 1
OglActiveTexture (GL_TEXTURE0, 0);
glEnable (GL_TEXTURE_2D); 

glEnable (GL_TEXTURE_GEN_S);
glEnable (GL_TEXTURE_GEN_T);
glEnable (GL_TEXTURE_GEN_R);
glEnable (GL_TEXTURE_GEN_Q);

for (i = 0; i < 4; i++)
	glTexGeni (nTexCoord [i], GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
for (i = 0; i < 4; i++)
	glTexGenfv (nTexCoord [i], GL_EYE_PLANE, mPlanef + 4 * i);

glGetFloatv (GL_PROJECTION_MATRIX, mProjectionf);
glMatrixMode (GL_TEXTURE);
for (i = 0, pc = gameData.render.shadows.shadowMaps; i < 1/*gameData.render.shadows.nShadowMaps*/; i++) {
	glBindTexture (GL_TEXTURE_2D, pc->fb.texId);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
	glLoadMatrixf (mTexBiasf);
	glMultMatrixf (mProjectionf);
	glMultMatrixf (OOF_MatVms2Gl (mModelViewf, &pc->objP->position.mOrient));
	}
glMatrixMode (GL_MODELVIEW);
#endif
RenderMine (nStartSeg, nEyeOffset, nWindow);
#if 1
glMatrixMode (GL_TEXTURE);
glLoadIdentity ();
glMatrixMode (GL_MODELVIEW);
glDisable (GL_TEXTURE_GEN_S);
glDisable (GL_TEXTURE_GEN_T);
glDisable (GL_TEXTURE_GEN_R);
glDisable (GL_TEXTURE_GEN_Q);
OglActiveTexture (GL_TEXTURE0, 0);		
glDisable (GL_TEXTURE_2D);
#endif
DestroyShadowMaps ();
}

//------------------------------------------------------------------------------

int GatherShadowLightSources (void)
{
	tObject			*objP = gameData.objs.objects;
	int				h, i, j, k, n, m = gameOpts->render.shadows.nLights;
	short				*pnl;
//	tDynLight		*pl;
	tShaderLight	*psl;
	vmsVector		vLightDir;

psl = gameData.render.lights.dynamic.shader.lights;
for (h = 0, i = gameData.render.lights.dynamic.nLights; i; i--, psl++)
	psl->bShadow =
	psl->bExclusive = 0;
for (h = 0; h <= gameData.objs.nLastObject + 1; h++, objP++) {
	if (gameData.render.mine.bObjectRendered [h] != gameStates.render.nFrameFlipFlop)
		continue;
	pnl = gameData.render.lights.dynamic.nNearestSegLights + objP->nSegment * MAX_NEAREST_LIGHTS;
	k = h * MAX_SHADOW_LIGHTS;
	for (i = n = 0; (n < m) && (*pnl >= 0); i++, pnl++) {
		psl = gameData.render.lights.dynamic.shader.lights + *pnl;
		if (!psl->bState)
			continue;
		if (!CanSeePoint (objP, &objP->position.vPos, &psl->vPos, objP->nSegment))
			continue;
		VmVecSub (&vLightDir, &objP->position.vPos, &psl->vPos);
		VmVecNormalize (&vLightDir);
		if (n) {
			for (j = 0; j < n; j++)
				if (abs (VmVecDot (&vLightDir, gameData.render.shadows.vLightDir + j)) > 2 * F1_0 / 3) // 60 deg
					break;
			if (j < n)
				continue;
			}
		gameData.render.shadows.vLightDir [n] = vLightDir;
		gameData.render.shadows.objLights [k + n++] = *pnl;
		psl->bShadow = 1;
		}
	gameData.render.shadows.objLights [k + n] = -1;
	}
psl = gameData.render.lights.dynamic.shader.lights;
for (h = 0, i = gameData.render.lights.dynamic.nLights; i; i--, psl++)
	if (psl->bShadow)
		h++;
return h;
}

//------------------------------------------------------------------------------

void RenderFastShadows (fix nEyeOffset, int nWindow, short nStartSeg)
{
#if 0//OOF_TEST_CUBE
#	if 1
for (bShadowTest = 1; bShadowTest >= 0; bShadowTest--) 
#	else
for (bShadowTest = 0; bShadowTest < 2; bShadowTest++) 
#	endif
#endif
	{
	gameStates.render.nShadowPass = 2;
	OglStartFrame (0, 0);
	gameData.render.shadows.nFrame = !gameData.render.shadows.nFrame;
	//RenderObjectShadows ();
	RenderMine (nStartSeg, nEyeOffset, nWindow);
	}
#ifdef _DEBUG
if (!bShadowTest) 
#endif
	{
	gameStates.render.nShadowPass = 3;
	OglStartFrame (0, 0);
	if	(gameStates.render.bShadowMaps) {
#ifdef _DEBUG
		if (gameStates.render.bExternalView)
#else		
		if (gameStates.render.bExternalView && (!IsMultiGame || IsCoopGame || EGI_FLAG (bEnableCheats, 0, 0, 0)))
#endif			 	
			G3SetViewMatrix (&gameData.render.mine.viewerEye, externalView.pPos ? &externalView.pPos->mOrient : 
								  &gameData.objs.viewer->position.mOrient, gameStates.render.xZoom);
		else
			G3SetViewMatrix (&gameData.render.mine.viewerEye, &gameData.objs.viewer->position.mOrient, 
								  FixDiv (gameStates.render.xZoom, gameStates.render.nZoomFactor));
		ApplyShadowMaps (nStartSeg, nEyeOffset, nWindow);
		}
	else {
		RenderShadowQuad (0);
		}
	}
}

//------------------------------------------------------------------------------

void RenderNeatShadows (fix nEyeOffset, int nWindow, short nStartSeg)
{
	short				i;
	tShaderLight	*psl = gameData.render.lights.dynamic.shader.lights;

gameData.render.shadows.nLights = GatherShadowLightSources ();
for (i = 0; i < gameData.render.lights.dynamic.nLights; i++, psl++) {
	if (!psl->bShadow)
		continue;
	gameData.render.shadows.pLight = psl;
	psl->bExclusive = 1;
#if 1
	gameStates.render.nShadowPass = 2;
	OglStartFrame (0, 0);
	memcpy (&gameData.render.shadows.vLightPos, psl->pos + 1, sizeof (tOOF_vector));
	gameData.render.shadows.nFrame = !gameData.render.shadows.nFrame;
	RenderMine (nStartSeg, nEyeOffset, nWindow);
#endif
	gameStates.render.nShadowPass = 3;
	OglStartFrame (0, 0);
	gameData.render.shadows.nFrame = !gameData.render.shadows.nFrame;
	RenderMine (nStartSeg, nEyeOffset, nWindow);
	psl->bExclusive = 0;
	}
#if 0
gameStates.render.nShadowPass = 4;
RenderMine (nStartSeg, nEyeOffset, nWindow);
#endif
}


//------------------------------------------------------------------------------
// eof