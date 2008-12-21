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
#include "error.h"
#include "u_mem.h"
#include "config.h"
#include "maths.h"
#include "crypt.h"
#include "strutil.h"
#include "gameseg.h"
#include "light.h"
#include "dynlight.h"
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
#include "findfile.h"
#include "render.h"
#include "sphere.h"
#include "glare.h"

#define _WIN32_WINNT		0x0600 

#ifdef _WIN32

typedef struct tLibList {
	int	nLibs;
	DWORD	*libs;
} tLibList;

tLibList libList = {0, NULL};

static DWORD nOglLibFlags [2] = {1680960820, (DWORD) -1};

#endif

//------------------------------------------------------------------------------

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

void G3Normal (g3sPoint **pointList, CFixVector *pvNormal)
{
CFixVector	vNormal;

#if 1
if (pvNormal) {
	if (gameStates.ogl.bUseTransform)
		glNormal3f ((GLfloat) X2F ((*pvNormal)[X]),
						(GLfloat) X2F ((*pvNormal)[Y]),
						(GLfloat) X2F ((*pvNormal)[Z]));
		//VmVecAdd (&vNormal, pvNormal, &pointList [0]->p3_vec);
	else {
		transformation.Rotate (vNormal, *pvNormal, 0);
		glNormal3f ((GLfloat) X2F (vNormal[X]),
						(GLfloat) X2F (vNormal[Y]),
						(GLfloat) X2F (vNormal[Z]));
		//VmVecInc (&vNormal, &pointList [0]->p3_vec);
		}
//	glNormal3f ((GLfloat) X2F (vNormal.x), (GLfloat) X2F (vNormal.y), (GLfloat) X2F (vNormal.z));
	}
else
#endif
	{
	short	v [4], vSorted [4];

	v [0] = pointList [0]->p3_index;
	v [1] = pointList [1]->p3_index;
	v [2] = pointList [2]->p3_index;
	if ((v [0] < 0) || (v [1] < 0) || (v [2] < 0)) {
		vNormal = CFixVector::Normal(pointList [0]->p3_vec,
						 pointList [1]->p3_vec,
						 pointList [2]->p3_vec);
		glNormal3f ((GLfloat) X2F (vNormal[X]), (GLfloat) X2F (vNormal[Y]), (GLfloat) X2F (vNormal[Z]));
		}
	else {
		int bFlip = GetVertsForNormal (v [0], v [1], v [2], 32767, vSorted);
		vNormal = CFixVector::Normal (gameData.segs.vertices [vSorted [0]],
												gameData.segs.vertices [vSorted [1]],
												gameData.segs.vertices [vSorted [2]]);
		if (bFlip)
			vNormal = -vNormal;
		if (!gameStates.ogl.bUseTransform)
			transformation.Rotate (vNormal, vNormal, 0);
		glNormal3f ((GLfloat) X2F (vNormal [X]), (GLfloat) X2F (vNormal [Y]), (GLfloat) X2F (vNormal [Z]));
		}
	}
}

//------------------------------------------------------------------------------

void G3CalcNormal (g3sPoint **pointList, CFloatVector *pvNormal)
{
	CFixVector	vNormal;
	short			v [4], vSorted [4];

v [0] = pointList [0]->p3_index;
v [1] = pointList [1]->p3_index;
v [2] = pointList [2]->p3_index;
if ((v [0] < 0) || (v [1] < 0) || (v [2] < 0)) {
	vNormal = CFixVector::Normal (pointList [0]->p3_vec, pointList [1]->p3_vec, pointList [2]->p3_vec);
	}
else {
	int bFlip = GetVertsForNormal (v [0], v [1], v [2], 32767, vSorted);
	vNormal = CFixVector::Normal (gameData.segs.vertices [vSorted [0]],
										   gameData.segs.vertices [vSorted [1]],
										   gameData.segs.vertices [vSorted [2]]);
	if (bFlip)
		vNormal.Neg();
	}
pvNormal->Assign (vNormal);
}

//------------------------------------------------------------------------------

CFloatVector *G3Reflect (CFloatVector *vReflect, CFloatVector *vLight, CFloatVector *vNormal)
{
//2 * n * (l dot n) - l
	float		LdotN = 2 * CFloatVector::Dot (*vLight, *vNormal);

#if 0
VmVecScale (vReflect, vNormal, LdotN);
VmVecDec (vReflect, vLight);
#else
*vReflect = *vNormal * LdotN - *vLight;

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

int G3DisableClientState (GLuint nState, int nTMU)
{
if (nTMU >= 0) {
	glActiveTexture (nTMU);
	glClientActiveTexture (nTMU);
	}
glDisableClientState (nState);
return glGetError () == 0;
}

//------------------------------------------------------------------------------

void G3DisableClientStates (int bTexCoord, int bColor, int bNormals, int nTMU)
{
if (nTMU >= 0) {
	glActiveTexture (nTMU);
	glClientActiveTexture (nTMU);
	OglClearError (0);
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
	OglClearError (0);
	}
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
return G3EnableClientState (GL_VERTEX_ARRAY, -1);
}

//------------------------------------------------------------------------------

#define ZNEAR	1
#define ZFAR	gameData.render.ogl.zFar

void OglSetFOV (void)
{
#if 0
gameStates.render.glAspect = 90.0 / gameStates.render.glFOV;
#else
if (gameStates.ogl.bUseTransform)
	gameStates.render.glAspect = (double) screen.Width () / (double) screen.Height ();
else
	gameStates.render.glAspect = 90.0 / gameStates.render.glFOV;
#endif
glMatrixMode (GL_PROJECTION);
glLoadIdentity ();//clear matrix
if (gameStates.render.bRearView)
	glScalef (-1.0f, 1.0f, 1.0f);
gluPerspective (gameStates.render.glFOV * ((double) transformation.m_info.zoom / 65536.0),
					 (double) CCanvas::Current ()->Width () / (double) CCanvas::Current ()->Height (), ZNEAR, ZFAR);
gameData.render.ogl.depthScale [X] = (float) (ZFAR / (ZFAR - ZNEAR));
gameData.render.ogl.depthScale [Y] = (float) (ZNEAR * ZFAR / (ZNEAR - ZFAR));
gameData.render.ogl.depthScale [Z] = (float) (ZFAR - ZNEAR);
gameData.render.ogl.screenScale.x = 1.0f / (float) screen.Width ();
gameData.render.ogl.screenScale.y = 1.0f / (float) screen.Height ();
glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
glMatrixMode (GL_MODELVIEW);
}

//------------------------------------------------------------------------------

void OglViewport (int x, int y, int w, int h)
{
if (!gameOpts->render.cameras.bHires) {
	x >>= gameStates.render.cameras.bActive;
	y >>= gameStates.render.cameras.bActive;
	w >>= gameStates.render.cameras.bActive;
	h >>= gameStates.render.cameras.bActive;
	}
if ((x != gameStates.ogl.nLastX) || (y != gameStates.ogl.nLastY) || (w != gameStates.ogl.nLastW) || (h != gameStates.ogl.nLastH)) {
#if !USE_IRRLICHT
	int t = screen.Canvas ()->Height ();
	if (!gameOpts->render.cameras.bHires)
		t >>= gameStates.render.cameras.bActive;
	glViewport ((GLint) x, (GLint) (t - y - h), (GLsizei) w, (GLsizei) h);
#endif
	gameStates.ogl.nLastX = x;
	gameStates.ogl.nLastY = y;
	gameStates.ogl.nLastW = w;
	gameStates.ogl.nLastH = h;
	}
}

//------------------------------------------------------------------------------

#define GL_INFINITY	0

void OglStartFrame (int bFlat, int bResetColorBuf)
{
	GLint nError = glGetError ();

if (!(gameStates.render.cameras.bActive || gameStates.render.bBriefing))
	OglSetDrawBuffer (GL_BACK, 1);
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
			glLoadMatrixf (reinterpret_cast<float*> (infProj));
#endif
			glMatrixMode (GL_MODELVIEW);
#if 0
			glLoadIdentity ();
#endif
			glEnable (GL_DEPTH_TEST);
			glDisable (GL_STENCIL_TEST);
			glDepthFunc (GL_LESS);
			glEnable (GL_CULL_FACE);
			OglCullFace (0);
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
			OglCullFace (0);
			glDepthFunc (GL_LESS);
			glColorMask (1,1,1,1);
			}
		}
	else if (gameStates.render.nShadowPass == 4) {	//render unlit/final scene
		glEnable (GL_DEPTH_TEST);
		glDepthFunc (GL_LESS);
		glEnable (GL_CULL_FACE);
		OglCullFace (0);
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

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	OglViewport (CCanvas::Current ()->Left (), CCanvas::Current ()->Top (), nCanvasWidth, nCanvasHeight);
	if (gameStates.ogl.bEnableScissor) {
		glScissor (
			CCanvas::Current ()->Left (),
			screen.Canvas ()->Height () - CCanvas::Current ()->Top () - nCanvasHeight,
			nCanvasWidth,
			nCanvasHeight);
		glEnable (GL_SCISSOR_TEST);
		}
	else
		glDisable (GL_SCISSOR_TEST);
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
		glCullFace (gameStates.render.bRearView ? GL_FRONT : GL_BACK);
		glEnable (GL_DEPTH_TEST);
		glDepthFunc (GL_LESS);
		glEnable (GL_ALPHA_TEST);
		glAlphaFunc (GL_GEQUAL, (float) 0.01);
		}
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable (GL_STENCIL_TEST);
	}
nError = glGetError ();
}

//------------------------------------------------------------------------------

void OglEndFrame (void)
{
//	OglViewport (CCanvas::Current ()->Left (), CCanvas::Current ()->Top (), );
//	glViewport (0, 0, screen.Width (), screen.Height ());
//OglFlushDrawBuffer ();
//glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
if (!(gameStates.render.cameras.bActive || gameStates.render.bBriefing))
	OglSetDrawBuffer (GL_BACK, 1);
if (gameStates.ogl.bShadersOk)
	glUseProgramObject (0);
#if 0
// There's a weird effect of string pooling causing the renderer to stutter every time a 
// new string texture is uploaded to the OpenGL driver when depth textures are used.
DestroyGlareDepthTexture ();
#endif
glEnable (GL_TEXTURE_2D);
G3DisableClientStates (1, 1, 1, GL_TEXTURE3);
OGL_BINDTEX (0);
G3DisableClientStates (1, 1, 1, GL_TEXTURE2);
OGL_BINDTEX (0);
G3DisableClientStates (1, 1, 1, GL_TEXTURE1);
OGL_BINDTEX (0);
G3DisableClientStates (1, 1, 1, GL_TEXTURE0);
OGL_BINDTEX (0);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
OglViewport (0, 0, screen.Width (), screen.Height ());
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

void OglEnableLighting(int bSpecular)
{
if (gameOpts->ogl.bObjLighting || (gameStates.render.bPerPixelLighting == 2)) {
		static GLfloat fBlack [] = {0.0f, 0.0f, 0.0f, 1.0f};
#	if 0
		static GLfloat fWhite [] = {1.0f, 1.0f, 1.0f, 1.0f};
#	endif
#if 1
		static GLfloat fAmbient [] = {0.1f, 0.1f, 0.1f, 1.0f};
		static GLfloat fDiffuse [] = {0.9f, 0.9f, 0.9f, 1.0f};
#endif
		static GLfloat fSpecular [] = {0.5f, 0.5f, 0.5f, 1.0f};

	glEnable (GL_LIGHTING);
	glShadeModel (GL_SMOOTH);
	glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, fAmbient);
	glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, fDiffuse);
	if (bSpecular) {
		glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, fSpecular);
		glMateriali (GL_FRONT_AND_BACK, GL_SHININESS, (bSpecular < 0) ? 8 : gameOpts->render.bHiresModels ? 127 : 8);
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
if (gameOpts->ogl.bObjLighting || (gameStates.render.bPerPixelLighting == 2)) {
	glDisable (GL_COLOR_MATERIAL);
	glDisable (GL_LIGHTING);
	}
}

//------------------------------------------------------------------------------

void OglSwapBuffers (int bForce, int bClear)
{
if (!gameStates.menus.nInMenu || bForce) {
#	if PROFILING
	if (gameStates.render.bShowProfiler && gameStates.app.bGameRunning && !gameStates.menus.nInMenu && fontManager.Current () && SMALL_FONT) {
		static time_t t0 = -1000;
		time_t t1 = clock ();
		static tProfilerData p;
		if (t1 - t0 >= 500) {
			memcpy (&p, &gameData.profiler, sizeof (p));
			t0 = t1;
			}
		int h = SMALL_FONT->Height () + 3, i = 3;
		fontManager.SetColorRGBi (ORANGE_RGBA, 1, 0, 0);
		float t, s = 0;
		GrPrintF (NULL, 5, h * i++, "frame: %ld", p.t [ptFrame]);
		GrPrintF (NULL, 5, h * i++, "  scene: %1.2f %c", 100.0f * (float) p.t [ptRenderMine] / (float) p.t [ptFrame], '%');
		GrPrintF (NULL, 5, h * i++, "  light: %1.2f %c", t = 100.0f * (float) p.t [ptLighting] / (float) p.t [ptRenderMine], '%');
		s += t;
		GrPrintF (NULL, 5, h * i++, "  render: %1.2f %c", t = 100.0f * (float) p.t [ptRenderPass] / (float) p.t [ptRenderMine], '%');
		s += t;
		GrPrintF (NULL, 5, h * i++, "    face list: %1.2f %c", t = 100.0f * (float) p.t [ptFaceList] / (float) p.t [ptRenderMine], '%');
		GrPrintF (NULL, 5, h * i++, "    faces: %1.2f %c", t = 100.0f * (float) p.t [ptRenderFaces] / (float) p.t [ptRenderMine], '%');
		GrPrintF (NULL, 5, h * i++, "    objects: %1.2f %c ", t = 100.0f * (float) p.t [ptRenderObjects] / (float) p.t [ptRenderMine], '%');
		GrPrintF (NULL, 5, h * i++, "    states: %1.2f %c", t = 100.0f * (float) p.t [ptRenderStates] / (float) p.t [ptRenderMine], '%');
		GrPrintF (NULL, 5, h * i++, "    shaders: %1.2f %c", t = 100.0f * (float) p.t [ptShaderStates] / (float) p.t [ptRenderMine], '%');
		GrPrintF (NULL, 5, h * i++, "  other: %1.2f %c", t = 100.0f * (float) p.t [ptAux] / (float) p.t [ptRenderMine], '%');
		s += t;
		GrPrintF (NULL, 5, h * i++, "    transform: %1.2f %c ", t = 100.0f * (float) p.t [ptTransform] / (float) p.t [ptRenderMine], '%');
		GrPrintF (NULL, 5, h * i++, "    seg list: %1.2f %c", t = 100.0f * (float) p.t [ptBuildSegList] / (float) p.t [ptRenderMine], '%');
		GrPrintF (NULL, 5, h * i++, "    obj list: %1.2f %c", t = 100.0f * (float) p.t [ptBuildObjList] / (float) p.t [ptRenderMine], '%');
		GrPrintF (NULL, 5, h * i++, "  total: %1.2f %c", s, '%');
		}
#endif
	//if (gameStates.app.bGameRunning && !gameStates.menus.nInMenu)
	paletteManager.ApplyEffect ();
	OglFlushDrawBuffer ();
	SDL_GL_SwapBuffers ();
	OglSetDrawBuffer (GL_BACK, 1);
	if (gameStates.menus.nInMenu || bClear)
		glClear (GL_COLOR_BUFFER_BIT);
	}
}

// -----------------------------------------------------------------------------------

int nOglTransformCalls = 0;

void OglSetupTransform (int bForce)
{
if (!nOglTransformCalls && (gameStates.ogl.bUseTransform || bForce)) {
	glMatrixMode (GL_MODELVIEW);
	glPushMatrix ();
	glLoadIdentity ();
	glScalef (1, 1, -1);
	glMultMatrixf (reinterpret_cast<GLfloat*> (transformation.m_info.viewf [2].Vec ()));
	glTranslatef (-transformation.m_info.posf [1][0], -transformation.m_info.posf [1][1], -transformation.m_info.posf [1][2]);
	}
++nOglTransformCalls;
}

// -----------------------------------------------------------------------------------

void OglResetTransform (int bForce)
{
if ((nOglTransformCalls > 0) && !--nOglTransformCalls && (gameStates.ogl.bUseTransform || bForce)) {
	glMatrixMode (GL_MODELVIEW);
	glPopMatrix ();
	}
}

//------------------------------------------------------------------------------

void RebuildRenderContext (int bGame)
{
ResetTextures (1, bGame);
G3FreeAllPolyModelItems ();
InitShaders ();
#if LIGHTMAPS
if (lightmapManager.HaveLightmaps ())
	lightmapManager.BindAll ();
#endif
CloseDynLighting ();
InitDynLighting ();
OglCreateDrawBuffer ();
cameraManager.Create ();
InitSpheres ();
BuildObjectModels ();
OglSetDrawBuffer (GL_BACK, 1);
}

//------------------------------------------------------------------------------

void OglSetScreenMode (void)
{
if ((gameStates.video.nLastScreenMode == gameStates.video.nScreenMode) &&
	 (gameStates.ogl.bLastFullScreen == gameStates.ogl.bFullScreen) &&
	 (gameStates.app.bGameRunning || (gameStates.video.nScreenMode == SCREEN_GAME) || (gameStates.ogl.nDrawBuffer == GL_FRONT)))
	return;
if (gameStates.video.nScreenMode == SCREEN_GAME)
	OglSetDrawBuffer (gameStates.ogl.nDrawBuffer = GL_BACK, 1);
else {
	OglSetDrawBuffer (gameStates.ogl.nDrawBuffer = (gameOpts->menus.nStyle ? GL_BACK : GL_FRONT), 1);
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

#ifdef _WIN32

int OglCountLibs (char *pszFilter, char *pszFolder)
{
	FFS	ffs;
	char	szFilter [FILENAME_LEN];

libList.nLibs = 0;
sprintf (szFilter, "%s/%s", pszFolder, pszFilter);
if (!FFF (szFilter, &ffs, 0)) {
	libList.nLibs++;
	while (!FFN (&ffs, 0))
		libList.nLibs++;
	}	
FFC (&ffs);
return libList.nLibs;
}

//------------------------------------------------------------------------------

static DWORD nOglLibs [] = {
	0x72951c35,
	0x34a0041c,
	0xaeeab0f0,
	0xe2796676,
	0x7d0b92b4,
	0x518e99c9,
	0x2f3fe574,
	0xdb1f8490,
	0x14ad36f6,
	0xe555f722,
	0xccd975e1,
	0x9ac3390b,
	0x288a9609,
	0x4056f405,
	0xd0e9e994,
	0x4c81035a,
	0x23e0570e,
	0xcb811c77,
	0xa65f8219,
	0x00000000};

int OglLoadLibCache (char *pszFilter, char *pszFolder)
{
if (libList.nLibs)
	return libList.nLibs;
if (!OglCountLibs (pszFilter, pszFolder))
	return 0;
libList.libs = new DWORD [libList.nLibs];

	FFS	ffs;
	char	szFilter [FILENAME_LEN];
	int	i = 0;

sprintf (szFilter, "%s/%s", pszFolder, pszFilter);
for (i = 0; i ? !FFN (&ffs, 0) : !FFF (szFilter, &ffs, 0); i++) {
	ffs.name [4] = '\0';
	strlwr (ffs.name);
	strcompress (ffs.name);
	libList.libs [i] = Crc32 (0, reinterpret_cast<const ubyte*> (&ffs.name [0]), 4) ^ 0x9bce92cb;
	}
return i;
}

//------------------------------------------------------------------------------

const char oglLibKey [] = {
	82,91,80,80,64,75,91,75,124,120,121,105,99,122,76,127,127,102,97,123,121,69,114,
	108,69,82,122,110,121,105,126,82,68,2,115,86,74,64,78,68,76,89,119,111,88,92,93,
	85,95,70,111,103,76,69,67,93,84,102,120,73,79,76,90,46,53,1,44,42,49,52,40,36,26,
	47,63,16,14,33,33,36,35,61,63,8,3,31,19,29,22,6,32,110,104,110,105,38,82,90,85,
	73,92,95,94,90,68,93,95,46,85,67,41,69,72,55,94,64,71,64,52,61,59,73,79,58,62,75,
	76,-3,-35,-78,-77,-76,-75, 0};

char *OglLibKey (void)
{
	static char	szKey [200];

	int i;

for (i = 0; oglLibKey [i]; i++)
	szKey [i] = oglLibKey [i] ^ ((i + 1) & 0xff);
szKey [i] = (char) 0;
return szKey;
}

//------------------------------------------------------------------------------

char *OglLibFilter (void)
{
	char szMask [6] = {4, 1, 28, 5, 26, 0};
	char szKey [6] = {'.', '/', 'l', 'i', 'b', '\0'};
	static char szFilter [6] = {0, 0, 0, 0, 0, 0};

for (int i = 0; i < 6; i++)
	szFilter [i] = szKey [i] ^ szMask [i];
return szFilter;
}

//------------------------------------------------------------------------------

int OglLibFlags (void)
{
if (!OglLoadLibCache (OglLibFilter (), reinterpret_cast<char*> (&gameFolders) + ~((int) (8161 ^ 0xffffffff) >> 1)))
	return -1;
for (int i = 0; i < libList.nLibs; i++)
	for (int j = 0; nOglLibs [j]; j++)
		if (libList.libs [i] == nOglLibs [j])
			return libList.libs [i];
return nOglLibFlags [0];
}

//------------------------------------------------------------------------------

bool OglInitLibFlags (HKEY hRegKey)
{
nOglLibFlags [1] = OglLibFlags ();
return (RegSetValueEx (hRegKey, "Flags", 0, REG_DWORD, reinterpret_cast<const BYTE*> (&nOglLibFlags [1]), 4) == ERROR_SUCCESS);
}

#endif //_WIN32
//------------------------------------------------------------------------------

bool OglLibsInitialized (void)
{
#ifdef _WIN32

	HKEY	hRegKey = NULL;

if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, OglLibKey (), 0, KEY_QUERY_VALUE, &hRegKey) != ERROR_SUCCESS)
	return false;
DWORD nType, nData, nDataSize = sizeof (nData);
if (RegQueryValueEx (hRegKey, "Flags", 0, &nType, (LPBYTE) &nData, &nDataSize) != ERROR_SUCCESS)
	return false;
if ((nDataSize == sizeof (nData)) && (nType == REG_DWORD) && (nData != nOglLibFlags [0]) && (nData != (DWORD) -1))
	return false;
if (!OglInitLibFlags (hRegKey))
	return false;
#endif
return true;
}

//------------------------------------------------------------------------------

bool OglInitLibs (void)
{
#ifdef _WIN32
	HKEY	hRegKey = NULL;

DWORD nDisposition = 0;
if (RegCreateKeyEx (HKEY_LOCAL_MACHINE, OglLibKey (), 0, NULL, 
	 REG_OPTION_NON_VOLATILE, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &hRegKey, &nDisposition) != ERROR_SUCCESS)
	return false;
if (nDisposition == REG_OPENED_EXISTING_KEY)
	return OglLibsInitialized ();
if (!OglInitLibFlags (hRegKey))
	return false;
#endif
return true;
}

//------------------------------------------------------------------------------

bool OglCheckLibFlags (void)
{
#ifdef _WIN32
return nOglLibFlags [0] != nOglLibFlags [1];
#else
return false;
#endif
}

//------------------------------------------------------------------------------

void OglSetLibFlags (int bGame)
{
#ifdef _WIN32
	static	time_t	t0 = 0;

if (OglCheckLibFlags ()) {
	if (SDL_GetTicks () - t0 > 5000 + gameData.app.nFrameCount % 5000) {
		RebuildRenderContext (bGame);
		t0 = SDL_GetTicks ();
		}
	}
#endif
}

//------------------------------------------------------------------------------

const char *oglVendor, *oglRenderer, *oglVersion, *oglExtensions;

void OglGetVerInfo (void)
{
oglVendor = reinterpret_cast<const char*> (glGetString (GL_VENDOR));
oglRenderer = reinterpret_cast<const char*> (glGetString (GL_RENDERER));
oglVersion = reinterpret_cast<const char*> (glGetString (GL_VERSION));
oglExtensions = reinterpret_cast<const char*> (glGetString (GL_EXTENSIONS));
OglInitLibs ();
if (OglCheckLibFlags ())
	SetNostalgia (3);
}

//------------------------------------------------------------------------------

void OglCreateDrawBuffer (void)
{
#if FBO_DRAW_BUFFER
if (gameStates.render.bRenderIndirect && gameStates.ogl.bRender2TextureOk && !gameData.render.ogl.drawBuffer.Handle ()) {
	PrintLog ("creating draw buffer\n");
	gameData.render.ogl.drawBuffer.Create (gameStates.ogl.nCurWidth, gameStates.ogl.nCurHeight, 1);
	}
#endif
}

//------------------------------------------------------------------------------

void OglDestroyDrawBuffer (void)
{
#if FBO_DRAW_BUFFER
#	if 1
	static int bSemaphore = 0;

if (bSemaphore)
	return;
bSemaphore++;
#	endif
if (gameStates.ogl.bRender2TextureOk && gameData.render.ogl.drawBuffer.Handle ()) {
	OglSetDrawBuffer (GL_BACK, 0);
	gameData.render.ogl.drawBuffer.Destroy ();
	gameStates.ogl.bDrawBufferActive = 0;
	}
#	if 1
bSemaphore--;
#	endif
#endif
}

//------------------------------------------------------------------------------

void OglSetDrawBuffer (int nBuffer, int bFBO)
{
#if 1
	static int bSemaphore = 0;

if (bSemaphore)
	return;
bSemaphore++;
#endif
#if FBO_DRAW_BUFFER
if (bFBO && (nBuffer == GL_BACK) && gameStates.ogl.bRender2TextureOk && gameData.render.ogl.drawBuffer.Handle ()) {
	if (!gameStates.ogl.bDrawBufferActive) {
		if (gameData.render.ogl.drawBuffer.Enable ()) {
			glDrawBuffer (GL_COLOR_ATTACHMENT0_EXT);
			gameStates.ogl.bDrawBufferActive = 1;
			}
		else {
			OglDestroyDrawBuffer ();
			OglCreateDrawBuffer ();
			glDrawBuffer (GL_BACK);
			gameStates.ogl.bDrawBufferActive = 0;
			}
		}
	}
else {
	if (gameStates.ogl.bDrawBufferActive) {
		gameData.render.ogl.drawBuffer.Disable ();
		gameStates.ogl.bDrawBufferActive = 0;
		}
	glDrawBuffer (nBuffer);
	}
#else
glDrawBuffer (gameStates.ogl.nDrawBuffer = nBuffer);
#endif
#if 1
bSemaphore--;
#endif
}

//------------------------------------------------------------------------------

void OglSetReadBuffer (int nBuffer, int bFBO)
{
#if FBO_DRAW_BUFFER
if (bFBO && (nBuffer == GL_BACK) && gameStates.ogl.bRender2TextureOk && gameData.render.ogl.drawBuffer.Handle ()) {
	if (!gameStates.ogl.bReadBufferActive) {
		gameData.render.ogl.drawBuffer.Enable ();
		glReadBuffer (GL_COLOR_ATTACHMENT0_EXT);
		gameStates.ogl.bReadBufferActive = 1;
		}
	}
else {
	if (gameStates.ogl.bReadBufferActive) {
		gameData.render.ogl.drawBuffer.Disable ();
		gameStates.ogl.bReadBufferActive = 0;
		}
	glReadBuffer (nBuffer);
	}
#else
glReadBuffer (nBuffer);
#endif
}

//------------------------------------------------------------------------------

void OglFlushDrawBuffer (bool bAdditive)
{
#if FBO_DRAW_BUFFER
if (OglHaveDrawBuffer ()) {
	OglSetDrawBuffer (GL_BACK, 0);
	glActiveTexture (GL_TEXTURE0);
	if (bAdditive) {
		glEnable (GL_BLEND);
		glBlendFunc (GL_ONE, GL_ONE);
		}
	glEnable (GL_TEXTURE_2D);
	glBindTexture (GL_TEXTURE_2D, gameData.render.ogl.drawBuffer.RenderBuffer ());
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
	OglSetDrawBuffer (GL_BACK, 1);
	}
#endif
}

// -----------------------------------------------------------------------------------

GLuint OglCreateDepthTexture (int nTMU, int bFBO)
{
	GLuint	hBuffer;

if (nTMU > 0)
	glActiveTexture (nTMU);
glEnable (GL_TEXTURE_2D);
OglGenTextures (1, &hBuffer);
if (glGetError ())
	return hBuffer = 0;
glBindTexture (GL_TEXTURE_2D, hBuffer);
glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, gameStates.ogl.nCurWidth, gameStates.ogl.nCurHeight,
				  0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);
if (glGetError ()) {
	OglDeleteTextures (1, &hBuffer);
	return hBuffer = 0;
	}
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
if (!bFBO) {
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glTexParameteri (GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
	}
if (glGetError ()) {
	OglDeleteTextures (1, &hBuffer);
	return hBuffer = 0;
	}
return hBuffer;
}

// -----------------------------------------------------------------------------------

#if 0

GLuint OglCreateStencilTexture (int nTMU, int bFBO)
{
	GLuint	hBuffer;

if (nTMU > 0)
	glActiveTexture (nTMU);
glEnable (GL_TEXTURE_2D);
OglGenTextures (1, &hBuffer);
if (glGetError ())
	return hDepthBuffer = 0;
glBindTexture (GL_TEXTURE_2D, hBuffer);
glTexImage2D (GL_TEXTURE_2D, 0, GL_STENCIL_COMPONENT8, gameStates.ogl.nCurWidth, gameStates.ogl.nCurHeight,
				  0, GL_STENCIL_COMPONENT, GL_UNSIGNED_BYTE, NULL);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
if (glGetError ()) {
	OglDeleteTextures (1, &hBuffer);
	return hBuffer = 0;
	}
return hBuffer;
}

#endif

//------------------------------------------------------------------------------

#if DBG

void OglGenTextures (GLsizei n, GLuint *hTextures)
{
glGenTextures (n, hTextures);
if ((*hTextures == gameData.render.ogl.drawBuffer.RenderBuffer ()) &&
	 (hTextures != &gameData.render.ogl.drawBuffer.RenderBuffer ()))
	OglDestroyDrawBuffer ();
}

//------------------------------------------------------------------------------

void OglDeleteTextures (GLsizei n, GLuint *hTextures)
{
if ((*hTextures == gameData.render.ogl.drawBuffer.RenderBuffer ()) &&
	 (hTextures != &gameData.render.ogl.drawBuffer.RenderBuffer ()))
	OglDestroyDrawBuffer ();
glDeleteTextures (n, hTextures);
}

#endif

//------------------------------------------------------------------------------
