/* $Id: ogl.c, v 1.14 204/05/11 23:15:55 btb Exp $ */
/*
 *
 * Graphics support functions for OpenGL.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef _WIN32
#	include <windows.h>
#	include <stddef.h>
#	include <io.h>
#endif
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef __macosx__
# include <stdlib.h>
# include <SDL/SDL.h>
#else
# include <malloc.h>
# include <SDL.h>
#endif

#include "inferno.h"
#include "maths.h"
#include "strutil.h"
#include "gameseg.h"
#include "lighting.h"
#include "lightmap.h"
#include "network.h"
#include "gr.h"
#include "gamefont.h"
#include "screens.h"
#include "interp.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_texcache.h"
#include "ogl_color.h"
#include "ogl_shader.h"
#include "ogl_render.h"
#include "render.h"

//------------------------------------------------------------------------------

#define FBO_DRAW_BUFFER 1

#if DBG_SHADOWS
int bShadowTest = 0;
#endif

int bSingleStencil = 0;

extern int bZPass;

GLuint hBigSphere = 0;
GLuint hSmallSphere = 0;
GLuint circleh5 = 0;
GLuint circleh10 = 0;
GLuint mouseIndList = 0;
GLuint cross_lh [2]={0, 0};
GLuint primary_lh [3]={0, 0, 0};
GLuint secondary_lh [5] = {0, 0, 0, 0, 0};
GLuint g3InitTMU [4][2] = {{0,0},{0,0},{0,0},{0,0}};
GLuint g3ExitTMU [2] = {0,0};

int r_polyc, r_tpolyc, r_bitmapc, r_ubitmapc, r_ubitbltc, r_upixelc, r_tvertexc;
int r_texcount = 0;
int gr_renderstats = 0;
int gr_badtexture = 0;

//------------------------------------------------------------------------------

tRenderQuality renderQualities [] = {
	{GL_NEAREST, GL_NEAREST, 0, 0},	// no smoothing
	{GL_NEAREST, GL_LINEAR, 0, 0},	// smooth close textures, don't smooth distant textures
	{GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, 1, 0},	// smooth close textures, use non-smoothed mipmaps distant textures
	{GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 1, 0},	//smooth close textures, use smoothed mipmaps for distant ones
	{GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 1, 1}	//smooth close textures, use smoothed mipmaps for distant ones, anti-aliasing
	};
	
//------------------------------------------------------------------------------

void SetRenderQuality (void)
{
	static int nCurQual = -1;

if (nCurQual == gameOpts->render.nQuality)
	return;
nCurQual = gameOpts->render.nQuality;
gameStates.ogl.texMagFilter = renderQualities [gameOpts->render.nQuality].texMagFilter;
gameStates.ogl.texMinFilter = renderQualities [gameOpts->render.nQuality].texMinFilter;
gameStates.ogl.bNeedMipMaps = renderQualities [gameOpts->render.nQuality].bNeedMipmap;
gameStates.ogl.bAntiAliasing = renderQualities [gameOpts->render.nQuality].bAntiAliasing;
ResetTextures (1, gameStates.app.bGameRunning);
}

//------------------------------------------------------------------------------

void OglDeleteLists (GLuint *lp, int n)
{
for (; n; n--, lp++) {
	if (*lp) {
		glDeleteLists (*lp, 1);
		*lp = 0;
		}
	}
}

//------------------------------------------------------------------------------

void OglBlendFunc (GLenum nSrcBlend, GLenum nDestBlend)
{
gameData.render.ogl.nSrcBlend = nSrcBlend;
gameData.render.ogl.nDestBlend = nDestBlend;
glBlendFunc (nSrcBlend, nDestBlend);
}

//------------------------------------------------------------------------------

void OglComputeSinCos (int nSides, tSinCosf *sinCosP)
{
	int 		i;
	double	ang;

for (i = 0; i < nSides; i++, sinCosP++) {
	ang = 2.0 * Pi * i / nSides;
	sinCosP->fSin = (float) sin (ang);
	sinCosP->fCos = (float) cos (ang);
	}
}

//------------------------------------------------------------------------------

int CircleListInit (int nSides, int nType, int mode) 
{
	int h = glGenLists (1);

glNewList (h, mode);
/* draw a unit radius circle in xy plane centered on origin */
OglDrawCircle (nSides, nType);
glEndList ();
return h;
}

//------------------------------------------------------------------------------

void G3Normal (g3sPoint **pointList, vmsVector *pvNormal)
{
vmsVector	vNormal;

#if 1
if (pvNormal) {
	if (gameStates.ogl.bUseTransform)
		glNormal3f ((GLfloat) f2fl (pvNormal->p.x), 
						(GLfloat) f2fl (pvNormal->p.y), 
						(GLfloat) f2fl (pvNormal->p.z));
		//VmVecAdd (&vNormal, pvNormal, &pointList [0]->p3_vec);
	else {
		G3RotatePoint (&vNormal, pvNormal, 0);
		glNormal3f ((GLfloat) f2fl (vNormal.p.x), 
						(GLfloat) f2fl (vNormal.p.y), 
						(GLfloat) f2fl (vNormal.p.z));
		//VmVecInc (&vNormal, &pointList [0]->p3_vec);
		}
//	glNormal3f ((GLfloat) f2fl (vNormal.x), (GLfloat) f2fl (vNormal.y), (GLfloat) f2fl (vNormal.z));
	}
else 
#endif
	{
	int	v [4];

	v [0] = pointList [0]->p3_index;
	v [1] = pointList [1]->p3_index;
	v [2] = pointList [2]->p3_index;
	if ((v [0] < 0) || (v [1] < 0) || (v [2] < 0)) {
		VmVecNormal (&vNormal, 
						 &pointList [0]->p3_vec,
						 &pointList [1]->p3_vec,
						 &pointList [2]->p3_vec);
		glNormal3f ((GLfloat) f2fl (vNormal.p.x), (GLfloat) f2fl (vNormal.p.y), (GLfloat) f2fl (vNormal.p.z));
		}
	else {
		int bFlip = GetVertsForNormal (v [0], v [1], v [2], 32767, v, v + 1, v + 2, v + 3);
		VmVecNormal (&vNormal, 
							gameData.segs.vertices + v [0], 
							gameData.segs.vertices + v [1], 
							gameData.segs.vertices + v [2]);
		if (bFlip)
			VmVecNegate (&vNormal);
		if (!gameStates.ogl.bUseTransform)
			G3RotatePoint (&vNormal, &vNormal, 0);
		//VmVecInc (&vNormal, &pointList [0]->p3_vec);
		glNormal3f ((GLfloat) f2fl (vNormal.p.x), 
						(GLfloat) f2fl (vNormal.p.y), 
						(GLfloat) f2fl (vNormal.p.z));
		}
	}
}

//------------------------------------------------------------------------------

void G3CalcNormal (g3sPoint **pointList, fVector *pvNormal)
{
	vmsVector	vNormal;
	int	v [4];

v [0] = pointList [0]->p3_index;
v [1] = pointList [1]->p3_index;
v [2] = pointList [2]->p3_index;
if ((v [0] < 0) || (v [1] < 0) || (v [2] < 0)) {
	VmVecNormal (&vNormal, 
					 &pointList [0]->p3_vec,
					 &pointList [1]->p3_vec,
					 &pointList [2]->p3_vec);
	}
else {
	int bFlip = GetVertsForNormal (v [0], v [1], v [2], 32767, v, v + 1, v + 2, v + 3);
	VmVecNormal (&vNormal, 
					 gameData.segs.vertices + v [0], 
					 gameData.segs.vertices + v [1], 
					 gameData.segs.vertices + v [2]);
	if (bFlip)
		VmVecNegate (&vNormal);
	}
VmsVecToFloat (pvNormal, &vNormal);
}

//------------------------------------------------------------------------------

fVector *G3Reflect (fVector *vReflect, fVector *vLight, fVector *vNormal)
{
//2 * n * (l dot n) - l
	float		LdotN = 2 * VmVecDotf (vLight, vNormal);

#if 0
VmVecScalef (vReflect, vNormal, LdotN);
VmVecDecf (vReflect, vLight);
#else
vReflect->p.x = vNormal->p.x * LdotN - vLight->p.x;
vReflect->p.y = vNormal->p.y * LdotN - vLight->p.y;
vReflect->p.z = vNormal->p.z * LdotN - vLight->p.z;
#endif
return vReflect;
}

//------------------------------------------------------------------------------

int G3EnableClientState (GLuint nState, int nTMU)
{
if (nTMU >= 0) {
	glActiveTexture (nTMU);
	glClientActiveTexture (nTMU);
	}
glEnableClientState (nState);
if (!glGetError ())
	return 1;
glDisable (nState);
glEnableClientState (nState);
return glGetError () == 0;
}

//------------------------------------------------------------------------------

void G3DisableClientStates (int bTexCoord, int bColor, int bNormals, int nTMU)
{
if (nTMU >= 0) {
	glActiveTexture (nTMU);
	glClientActiveTexture (nTMU);
	}
if (bNormals)
	glDisableClientState (GL_NORMAL_ARRAY);
if (bTexCoord)
	glDisableClientState (GL_TEXTURE_COORD_ARRAY);
if (bColor)
	glDisableClientState (GL_COLOR_ARRAY);
glDisableClientState (GL_VERTEX_ARRAY);
}

//------------------------------------------------------------------------------

int G3EnableClientStates (int bTexCoord, int bColor, int bNormals, int nTMU)
{
if (nTMU >= 0) {
	glActiveTexture (nTMU);
	glClientActiveTexture (nTMU);
	}
if (!G3EnableClientState (GL_VERTEX_ARRAY, -1))
	return 0;
if (bNormals) {
	if (!G3EnableClientState (GL_NORMAL_ARRAY, -1)) {
		G3DisableClientStates (0, 0, 0, -1);
		return 0;
		}
	}
else
	glDisableClientState (GL_NORMAL_ARRAY);
if (bTexCoord) {
	if (!G3EnableClientState (GL_TEXTURE_COORD_ARRAY, -1)) {
		G3DisableClientStates (0, 0, 0, -1);
		return 0;
		}
	}
else
	glDisableClientState (GL_TEXTURE_COORD_ARRAY);
if (bColor) {
	if (!G3EnableClientState (GL_COLOR_ARRAY, -1)) {
		G3DisableClientStates (bTexCoord, 0, 0, -1);
		return 0;
		}
	}
else
	glDisableClientState (GL_COLOR_ARRAY);
return 1;
}

//------------------------------------------------------------------------------

#define ZNEAR	gameData.render.ogl.zNear
#define ZFAR	gameData.render.ogl.zFar

void OglSetFOV (double fov)
{
gameStates.render.glFOV = fov;
#if 0
gameStates.render.glAspect = 90.0 / gameStates.render.glFOV;
#else
if (gameStates.ogl.bUseTransform)
	gameStates.render.glAspect = (double) grdCurScreen->scWidth / (double) grdCurScreen->scHeight;
else
	gameStates.render.glAspect = 90.0 / gameStates.render.glFOV;
#endif
gluPerspective (gameStates.render.glFOV, gameStates.render.glAspect, ZNEAR, ZFAR);
gameData.render.ogl.depthScale.x = (float) (ZFAR / (ZFAR - ZNEAR));
gameData.render.ogl.depthScale.y = (float) (ZNEAR * ZFAR / (ZNEAR - ZFAR));
gameData.render.ogl.screenScale.x = 1.0f / (float) grdCurScreen->scWidth;
gameData.render.ogl.screenScale.y = 1.0f / (float) grdCurScreen->scHeight;
glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

//------------------------------------------------------------------------------

#define GL_INFINITY	0

void OglStartFrame (int bFlat, int bResetColorBuf)
{
	GLint nError = glGetError ();

if (!gameStates.render.cameras.bActive)
	OglDrawBuffer (GL_BACK, 1);
#if SHADOWS
if (gameStates.render.nShadowPass) {
#if GL_INFINITY
	float	infProj [4][4];	//projection to infinity
#endif
	
	if (gameStates.render.nShadowPass == 1) {	//render unlit/final scene
		if (!gameStates.render.bShadowMaps) {
#if GL_INFINITY
			glMatrixMode (GL_PROJECTION);
			memset (infProj, 0, sizeof (infProj));
			infProj [1][1] = 1.0f / (float) tan (gameStates.render.glFOV);
			infProj [0][0] = infProj [1][1] / (float) gameStates.render.glAspect;
			infProj [3][2] = -0.02f;	// -2 * near
			infProj [2][2] =
			infProj [2][3] = -1.0f;
			glLoadMatrixf ((float *) infProj);
#endif
#if 0
			glMatrixMode (GL_MODELVIEW);
			glLoadIdentity ();
#endif
			glEnable (GL_DEPTH_TEST);
			glDisable (GL_STENCIL_TEST);
			glDepthFunc (GL_LESS);
			glEnable (GL_CULL_FACE);		
			glCullFace (GL_BACK);
			if (!FAST_SHADOWS)
				glColorMask (0,0,0,0);
			}
		}
	else if (gameStates.render.nShadowPass == 2) {	//render occluders / shadow maps
		if (gameStates.render.bShadowMaps) {
			glColorMask (0,0,0,0);
			glEnable (GL_POLYGON_OFFSET_FILL);
			glPolygonOffset (1.0f, 2.0f);
			}
		else {
#	if DBG_SHADOWS
			if (bShadowTest) {
				glColorMask (1,1,1,1);
				glDepthMask (0);
				glEnable (GL_BLEND);
				glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glDisable (GL_STENCIL_TEST);
				}
			else 
#	endif
				{
				glColorMask (0,0,0,0);
				glDepthMask (0);
				glEnable (GL_STENCIL_TEST);
				if (!glIsEnabled (GL_STENCIL_TEST))
					extraGameInfo [0].bShadows = 
					extraGameInfo [1].bShadows = 0;
				glClearStencil (0);
				glClear (GL_STENCIL_BUFFER_BIT);
#if 0
				if (!glActiveStencilFaceEXT)
#endif
					bSingleStencil = 1;
#	if DBG_SHADOWS
				if (bSingleStencil || bShadowTest) {
#	else
				if (bSingleStencil) {
#	endif
					glStencilMask (~0);
					glStencilFunc (GL_ALWAYS, 0, ~0);
					}
#if 0
				else {
					glEnable (GL_STENCIL_TEST_TWO_SIDE_EXT);
					glActiveStencilFaceEXT (GL_BACK);
					if (bZPass)
						glStencilOp (GL_KEEP, GL_KEEP, GL_DECR_WRAP);
					else
						glStencilOp (GL_KEEP, GL_DECR_WRAP, GL_KEEP);
					glStencilOp (GL_KEEP, GL_DECR_WRAP, GL_KEEP);
					glStencilMask (~0);
					glStencilFunc (GL_ALWAYS, 0, ~0);
					glActiveStencilFaceEXT (GL_FRONT);
					if (bZPass)
						glStencilOp (GL_KEEP, GL_KEEP, GL_INCR_WRAP);
					else
						glStencilOp (GL_KEEP, GL_INCR_WRAP, GL_KEEP);
					glStencilMask (~0);
					glStencilFunc (GL_ALWAYS, 0, ~0);
					}
#endif
#if 0
				glEnable (GL_POLYGON_OFFSET_FILL);
				glPolygonOffset (1.0f, 1.0f);
#endif
				}
			}
		}
	else if (gameStates.render.nShadowPass == 3) { //render final lit scene
		if (gameStates.render.bShadowMaps) {
			glDisable (GL_POLYGON_OFFSET_FILL);
			glDepthFunc (GL_LESS);
			}
		else {
#if 0
			glDisable (GL_POLYGON_OFFSET_FILL);
#endif
			if (gameStates.render.nShadowBlurPass == 2)
				glDisable (GL_STENCIL_TEST);
         else if (FAST_SHADOWS) {
				glStencilFunc (GL_NOTEQUAL, 0, ~0);
				glStencilOp (GL_REPLACE, GL_KEEP, GL_KEEP);		
				}
			else 
				{
				glStencilFunc (GL_EQUAL, 0, ~0);
#if 0
				glStencilOp (GL_KEEP, GL_KEEP, GL_INCR);	//problem: layered texturing fails
#else
				glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);
#endif
				}
			glCullFace (GL_BACK);
			glDepthFunc (GL_LESS);
			glColorMask (1,1,1,1);
			}
		}
	else if (gameStates.render.nShadowPass == 4) {	//render unlit/final scene
		glEnable (GL_DEPTH_TEST);
		glDepthFunc (GL_LESS);
		glEnable (GL_CULL_FACE);		
		glCullFace (GL_BACK);
		}
#if GL_INFINITY
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
#endif
	}
else 
#endif //SHADOWS
	{
	r_polyc =  
	r_tpolyc = 
	r_tvertexc =
	r_bitmapc = 
	r_ubitmapc = 
	r_ubitbltc = 
	r_upixelc = 0;

	//if (gameStates.render.nShadowBlurPass < 2) 
		{
		glMatrixMode (GL_PROJECTION);
		glLoadIdentity ();//clear matrix
		OglSetFOV (gameStates.render.glFOV);
		glMatrixMode (GL_MODELVIEW);
		glLoadIdentity ();
		OGL_VIEWPORT (grdCurCanv->cvBitmap.bmProps.x, grdCurCanv->cvBitmap.bmProps.y, nCanvasWidth, nCanvasHeight);
		}
	if (gameStates.render.nRenderPass < 0) {
		glDepthMask (1);
		glColorMask (1,1,1,1);
		glClearColor (0,0,0,0);
		if (bResetColorBuf)
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		else
			glClear (GL_DEPTH_BUFFER_BIT);
		}
	else if (gameStates.render.nRenderPass) {
		glDepthMask (0);
		glColorMask (1,1,1,1);
		glClearColor (0,0,0,0);
		if (bResetColorBuf)
			glClear (GL_COLOR_BUFFER_BIT);
		}
	else { //make a depth-only render pass first to decrease bandwidth waste due to overdraw
		glDepthMask (1);
		glColorMask (0,0,0,0);
		glClear (GL_DEPTH_BUFFER_BIT);
		}
	if (gameStates.ogl.bEnableScissor) {
		glScissor (
			grdCurCanv->cvBitmap.bmProps.x, 
			grdCurScreen->scCanvas.cvBitmap.bmProps.h - grdCurCanv->cvBitmap.bmProps.y - nCanvasHeight, 
			nCanvasWidth, 
			nCanvasHeight);
		glEnable (GL_SCISSOR_TEST);
		}
	else
		glDisable (GL_SCISSOR_TEST);
	if (gameStates.ogl.bAntiAliasingOk && gameStates.ogl.bAntiAliasing)
		glEnable (GL_MULTISAMPLE_ARB);
	if (bFlat) {
		glDisable (GL_DEPTH_TEST);
		glDisable (GL_ALPHA_TEST);
		glDisable (GL_CULL_FACE);
		}
	else {
		glEnable (GL_CULL_FACE);		
		glFrontFace (GL_CW);	//Weird, huh? Well, D2 renders everything reverse ...
		glCullFace (GL_BACK);
		glEnable (GL_DEPTH_TEST);
		glDepthFunc (GL_LESS);
		glEnable (GL_ALPHA_TEST);
		glAlphaFunc (GL_GEQUAL, (float) 0.01);	
		}
#if 0
	if (SHOW_DYN_LIGHT) {//for optional hardware lighting
		GLfloat fAmbient [4] = {0.0f, 0.0f, 0.0f, 1.0f};
		glEnable (GL_LIGHTING);
		glLightModelfv (GL_LIGHT_MODEL_AMBIENT, fAmbient);
		glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, 0);
		glShadeModel (GL_SMOOTH);
		glEnable (GL_COLOR_MATERIAL);
		glColorMaterial (GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
		}
#endif
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable (GL_STENCIL_TEST);
	}
nError = glGetError ();
}

//------------------------------------------------------------------------------

void OglEndFrame (void)
{
//	OGL_VIEWPORT (grdCurCanv->cvBitmap.bmProps.x, grdCurCanv->cvBitmap.bmProps.y, );
//	glViewport (0, 0, grdCurScreen->scWidth, grdCurScreen->scHeight);
//OglFlushDrawBuffer ();
//glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
if (!gameStates.render.cameras.bActive)
	OglDrawBuffer (GL_BACK, 1);
glUseProgramObject (0);
G3DisableClientStates (1, 1, 1, GL_TEXTURE3);
G3DisableClientStates (1, 1, 1, GL_TEXTURE2);
G3DisableClientStates (1, 1, 1, GL_TEXTURE1);
G3DisableClientStates (1, 1, 1, GL_TEXTURE0);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
OGL_VIEWPORT (0, 0, grdCurScreen->scWidth, grdCurScreen->scHeight);
#ifndef NMONO
//	merge_textures_stats ();
//	ogl_texture_stats ();
#endif
glMatrixMode (GL_PROJECTION);
glLoadIdentity ();//clear matrix
glOrtho (0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
glMatrixMode (GL_MODELVIEW);
glLoadIdentity ();//clear matrix
glDisable (GL_SCISSOR_TEST);
glDisable (GL_ALPHA_TEST);
glDisable (GL_DEPTH_TEST);
glDisable (GL_CULL_FACE);
glDisable (GL_STENCIL_TEST);
if (SHOW_DYN_LIGHT) {
	glDisable (GL_LIGHTING);
	glDisable (GL_COLOR_MATERIAL);
	}
glDepthMask (1);
glColorMask (1,1,1,1);
if (gameStates.ogl.bAntiAliasingOk && gameStates.ogl.bAntiAliasing)
	glDisable (GL_MULTISAMPLE_ARB);
}

//------------------------------------------------------------------------------

void OglEnableLighting (int bSpecular)
{
if (gameOpts->ogl.bObjLighting || GEO_LIGHTING) {
		static GLfloat fBlack [] = {0.0f, 0.0f, 0.0f, 1.0f};
		static GLfloat fWhite [] = {1.0f, 1.0f, 1.0f, 1.0f};
		static GLfloat fAmbient [] = {0.2f, 0.2f, 0.2f, 1.0f};
		static GLfloat fDiffuse [] = {0.8f, 0.8f, 0.8f, 1.0f};
		static GLfloat fSpecular [] = {0.5f, 0.5f, 0.5f, 1.0f};

	glEnable (GL_LIGHTING);
	glShadeModel (GL_SMOOTH);
	glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, fBlack);
	glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, fWhite);
	if (bSpecular) {
		glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, fSpecular);
		glMateriali (GL_FRONT_AND_BACK, GL_SHININESS, gameOpts->render.bHiresModels ? 127 : 8);
		}
	else
		glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, fBlack);
	glColorMaterial (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable (GL_COLOR_MATERIAL);
	}
}

//------------------------------------------------------------------------------

void OglDisableLighting (void)
{
if (gameOpts->ogl.bObjLighting) {
	glDisable (GL_COLOR_MATERIAL);
	glDisable (GL_LIGHTING);
	}
}

//------------------------------------------------------------------------------

#if PROFILING
extern time_t tRenderMine, tRenderObject, tG3VertexColor, tSetNearestDynamicLights, tTransform;
#endif

void OglSwapBuffers (int bForce, int bClear)
{
if (!gameStates.menus.nInMenu || bForce) {
	OglCleanTextureCache ();
#if 1//def _DEBUG
	if (gr_renderstats) {
		if (gameStates.app.bGameRunning && !gameStates.menus.nInMenu && FONT && SMALL_FONT) {
			GrPrintF (NULL, 5, SMALL_FONT->ftHeight * 2 + 2, "flat:%i tex:%i vert:%i sprite:%i msg:%i", r_polyc, r_tpolyc, r_tvertexc, r_bitmapc, r_ubitmapc);
#	if PROFILING
			GrPrintF (NULL, 5, SMALL_FONT->ftHeight * 3 + 3, "mine:%ld", tRenderMine);
			GrPrintF (NULL, 5, SMALL_FONT->ftHeight * 4 + 4, "obj:%ld ", tRenderObject);
			GrPrintF (NULL, 5, SMALL_FONT->ftHeight * 5 + 5, "rot:%ld ", tTransform);
			GrPrintF (NULL, 5, SMALL_FONT->ftHeight * 6 + 6, "color:%ld", tG3VertexColor);
			GrPrintF (NULL, 5, SMALL_FONT->ftHeight * 7 + 7, "lights:%ld", tSetNearestDynamicLights);
			tRenderMine = tRenderObject = tG3VertexColor = tSetNearestDynamicLights = tTransform = 0;
#	endif
			}
		}
#endif
	//if (gameStates.app.bGameRunning && !gameStates.menus.nInMenu)
		OglDoPalFx ();
	OglFlushDrawBuffer ();
	SDL_GL_SwapBuffers ();
	OglDrawBuffer (GL_BACK, 1);
	if (gameStates.menus.nInMenu || bClear)
		glClear (GL_COLOR_BUFFER_BIT);
	}
}

// -----------------------------------------------------------------------------------

void OglSetupTransform (int bForce)
{
if (gameStates.ogl.bUseTransform || bForce) {
	glMatrixMode (GL_MODELVIEW);
	glPushMatrix ();
	glLoadIdentity ();
	glScalef (f2fl (viewInfo.scale.p.x), f2fl (viewInfo.scale.p.y), -f2fl (viewInfo.scale.p.z));
	glMultMatrixf (viewInfo.glViewf);
	glTranslatef (-viewInfo.glPosf [0], -viewInfo.glPosf [1], -viewInfo.glPosf [2]);
	}
}

// -----------------------------------------------------------------------------------

void OglResetTransform (int bForce)
{
if (gameStates.ogl.bUseTransform || bForce) {
	glPopMatrix ();
	}
}

//------------------------------------------------------------------------------

void RebuildRenderContext (int bGame, int bCameras)
{
ResetTextures (1, bGame);
G3FreeAllPolyModelItems ();
InitShaders ();
#if LIGHTMAPS
CreateLightMaps ();
#endif
if (bCameras) {
	DestroyCameras ();
	CreateCameras ();		
	}
CloseDynLighting ();
InitDynLighting ();
OglDestroyDrawBuffer ();
OglCreateDrawBuffer ();
OglDrawBuffer (GL_BACK, 1);
}

//------------------------------------------------------------------------------

void OglSetScreenMode (void)
{
if ((gameStates.video.nLastScreenMode == gameStates.video.nScreenMode) && 
	 (gameStates.ogl.bLastFullScreen == gameStates.ogl.bFullScreen) &&
	 (gameStates.app.bGameRunning || (gameStates.video.nScreenMode == SCREEN_GAME) || (curDrawBuffer == GL_FRONT)))
	return;
if (gameStates.video.nScreenMode == SCREEN_GAME)
	OglDrawBuffer (curDrawBuffer = GL_BACK, 1);
else {
	OglDrawBuffer (curDrawBuffer = (gameOpts->menus.nStyle ? GL_BACK : GL_FRONT), 1);
	if (!(gameStates.app.bGameRunning && gameOpts->menus.nStyle)) {
		glClearColor (0,0,0,0);
		glClear (GL_COLOR_BUFFER_BIT);
		glMatrixMode (GL_PROJECTION);
		glLoadIdentity ();//clear matrix
		glOrtho (0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
		glMatrixMode (GL_MODELVIEW);
		glLoadIdentity ();//clear matrix
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable (GL_TEXTURE_2D);
		glDepthFunc (GL_ALWAYS); //LEQUAL);
		glDisable (GL_DEPTH_TEST);
		}
	}
gameStates.video.nLastScreenMode = gameStates.video.nScreenMode;
gameStates.ogl.bLastFullScreen = gameStates.ogl.bFullScreen;
}

//------------------------------------------------------------------------------

const char *gl_vendor,*gl_renderer,*gl_version,*gl_extensions;

void OglGetVerInfo (void)
{
gl_vendor = (char *) glGetString (GL_VENDOR);
gl_renderer = (char *) glGetString (GL_RENDERER);
gl_version = (char *) glGetString (GL_VERSION);
gl_extensions = (char *) glGetString (GL_EXTENSIONS);
#if 0 //TRACE
con_printf(
	CON_VERBOSE, 
	"\ngl vendor:%s\nrenderer:%s\nversion:%s\nextensions:%s\n",
	gl_vendor,gl_renderer,gl_version,gl_extensions);
#endif
#if 0 //WGL only, I think
	dglMultiTexCoord2fARB = (glMultiTexCoord2fARB_fp)wglGetProcAddress("glMultiTexCoord2fARB");
	dglActiveTextureARB = (glActiveTextureARB_fp)wglGetProcAddress("glActiveTextureARB");
	dglMultiTexCoord2fSGIS = (glMultiTexCoord2fSGIS_fp)wglGetProcAddress("glMultiTexCoord2fSGIS");
	dglSelectTextureSGIS = (glSelectTextureSGIS_fp)wglGetProcAddress("glSelectTextureSGIS");
#endif

//multitexturing doesn't work yet.
#ifdef GL_ARB_multitexture
gameOpts->ogl.bArbMultiTexture=0;//(strstr(gl_extensions,"GL_ARB_multitexture")!=0 && glActiveTextureARB!=0 && 0);
#	if 0 //TRACE	
con_printf (CONDBG,"c:%p d:%p e:%p\n",strstr(gl_extensions,"GL_ARB_multitexture"),glActiveTextureARB,glBegin);
#	endif
#endif
#ifdef GL_SGIS_multitexture
gameOpts->ogl.bSgisMultiTexture=0;//(strstr(gl_extensions,"GL_SGIS_multitexture")!=0 && glSelectTextureSGIS!=0 && 0);
#	if TRACE	
con_printf (CONDBG,"a:%p b:%p\n",strstr(gl_extensions,"GL_SGIS_multitexture"),glSelectTextureSGIS);
#	endif	
#endif

//add driver specific hacks here.  whee.
if (gl_renderer) {
	if ((stricmp(gl_renderer,"Mesa NVIDIA RIVA 1.0\n")==0 || stricmp(gl_renderer,"Mesa NVIDIA RIVA 1.2\n")==0) && stricmp(gl_version,"1.2 Mesa 3.0")==0){
		gameStates.ogl.bIntensity4 = 0;	//ignores alpha, always black background instead of transparent.
		gameStates.ogl.bReadPixels = 0;	//either just returns all black, or kills the X server entirely
		gameStates.ogl.bGetTexLevelParam = 0;	//returns random data..
		}
	if (stricmp(gl_vendor,"Matrox Graphics Inc.")==0){
		//displays garbage. reported by
		//  redomen@crcwnet.com (render="Matrox G400" version="1.1.3 5.52.015")
		//  orulz (Matrox G200)
		gameStates.ogl.bIntensity4=0;
		}
	}
//allow overriding of stuff.
#if 0 //TRACE	
con_printf(CON_VERBOSE, 
	"gl_arb_multitexture:%i, gl_sgis_multitexture:%i\n",
	gameOpts->ogl.bArbMultiTexture,gameOpts->ogl.bSgisMultiTexture);
con_printf(CON_VERBOSE, 
	"gl_intensity4:%i, gl_luminance4_alpha4:%i, gl_readpixels:%i, gl_gettexlevelparam:%i\n",
	gameStates.ogl.bIntensity4, gameStates.ogl.bLuminance4Alpha4, gameStates.ogl.bReadPixels, gameStates.ogl.bGetTexLevelParam);
#endif
}

//------------------------------------------------------------------------------

void OglCreateDrawBuffer (void)
{
#if FBO_DRAW_BUFFER
if (gameStates.ogl.bRender2TextureOk && !gameData.render.ogl.drawBuffer.hFBO)
	OglCreateFBuffer (&gameData.render.ogl.drawBuffer, gameStates.ogl.nCurWidth, gameStates.ogl.nCurHeight, 1);
#endif
}

//------------------------------------------------------------------------------

void OglDestroyDrawBuffer (void)
{
#if FBO_DRAW_BUFFER
if (gameStates.ogl.bRender2TextureOk && gameData.render.ogl.drawBuffer.hFBO) {
	OglDestroyFBuffer (&gameData.render.ogl.drawBuffer);
	glDrawBuffer (GL_BACK);
	gameStates.ogl.bDrawBufferActive = 0;
	}
#endif
}

//------------------------------------------------------------------------------

void OglDrawBuffer (int nBuffer, int bFBO)
{
#if 1
	static int bSemaphore = 0;

if (bSemaphore)
	return;
bSemaphore++;
#endif
#if FBO_DRAW_BUFFER
if (bFBO && (nBuffer == GL_BACK) && gameStates.ogl.bRender2TextureOk && gameData.render.ogl.drawBuffer.hFBO) {
	if (!gameStates.ogl.bDrawBufferActive) {
		OglEnableFBuffer (&gameData.render.ogl.drawBuffer);
		glDrawBuffer (GL_COLOR_ATTACHMENT0_EXT);
		gameStates.ogl.bDrawBufferActive = 1;
		}
	}
else {
	if (gameStates.ogl.bDrawBufferActive) {
		OglDisableFBuffer (&gameData.render.ogl.drawBuffer);
		gameStates.ogl.bDrawBufferActive = 0;
		}
	glDrawBuffer (nBuffer);
	}
#else
glDrawBuffer (nBuffer);
#endif
#if 1
bSemaphore--;
#endif
}

//------------------------------------------------------------------------------

void OglReadBuffer (int nBuffer, int bFBO)
{
#if FBO_DRAW_BUFFER
if (bFBO && (nBuffer == GL_BACK) && gameStates.ogl.bRender2TextureOk && gameData.render.ogl.drawBuffer.hFBO) {
	if (!gameStates.ogl.bReadBufferActive) {
		OglEnableFBuffer (&gameData.render.ogl.drawBuffer);
		glReadBuffer (GL_COLOR_ATTACHMENT0_EXT);
		gameStates.ogl.bReadBufferActive = 1;
		}
	}
else {
	if (gameStates.ogl.bReadBufferActive) {
		OglDisableFBuffer (&gameData.render.ogl.drawBuffer);
		gameStates.ogl.bReadBufferActive = 0;
		}
	glReadBuffer (nBuffer);
	}
#else
glReadBuffer (nBuffer);
#endif
}

// -----------------------------------------------------------------------------------

GLuint OglCreateDepthTexture (int nTMU, int bFBO)
{
	GLuint	hDepthBuffer;

if (nTMU > 0)
	glActiveTexture (nTMU);
glEnable (GL_TEXTURE_2D);
glGenTextures (1, &hDepthBuffer);
if (glGetError ())
	return hDepthBuffer = 0;
glBindTexture (GL_TEXTURE_2D, hDepthBuffer);
glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, gameStates.ogl.nCurWidth, gameStates.ogl.nCurHeight, 
				  0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
if (!bFBO) {
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glTexParameteri (GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
	}
if (glGetError ()) {
	glDeleteTextures (1, &hDepthBuffer);
	return hDepthBuffer = 0;
	}
return hDepthBuffer;
}

//------------------------------------------------------------------------------

void OglFlushDrawBuffer (void)
{
#if FBO_DRAW_BUFFER
if (OglHaveDrawBuffer ()) {
	OglDrawBuffer (GL_BACK, 0);
#if 0
	glDisable (GL_CULL_FACE);
	glDisable (GL_BLEND);
#endif
	glActiveTexture (GL_TEXTURE0);
	glEnable (GL_TEXTURE_2D);
	glBindTexture (GL_TEXTURE_2D, gameData.render.ogl.drawBuffer.hRenderBuffer);
	glColor3f (1, 1, 1);
	glBegin (GL_QUADS);
	glTexCoord2f (0, 0);
	glVertex2f (0, 0);
	glTexCoord2f (0, 1);
	glVertex2f (0, 1);
	glTexCoord2f (1, 1);
	glVertex2f (1, 1);
	glTexCoord2f (1, 0);
	glVertex2f (1, 0);
	glEnd ();
#if 0
	glEnable (GL_CULL_FACE);
	glEnable (GL_BLEND);
#endif
	//OglDrawBuffer (GL_BACK, 1);
	}
#endif
}

//------------------------------------------------------------------------------
