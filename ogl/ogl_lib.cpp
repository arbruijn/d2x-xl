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

#include "descent.h"
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
#include "rendermine.h"
#include "sphere.h"
#include "glare.h"
#include "menu.h"
#include "menubackground.h"
#include "cockpit.h"
#include "gpgpu_lighting.h"

#define _WIN32_WINNT		0x0600 

#ifdef _WIN32

typedef struct tLibList {
	int	nLibs;
	DWORD	*libs;
} tLibList;

tLibList libList = {0, NULL};

static DWORD nOglLibFlags [2] = {1680960820, (DWORD) -1};

#endif

COGL ogl;

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
	{GL_LINEAR, GL_LINEAR, 0, 0},	// smooth textures
	{GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, 1, 0},	// smooth close textures, use non-smoothed mipmaps distant textures
	{GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 1, 0},	//smooth close textures, use smoothed mipmaps for distant ones
	{GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 1, 1},	//smooth close textures, use smoothed mipmaps for distant ones, anti-aliasing
	{GL_LINEAR, GL_LINEAR, 0, 1}	// special mode for high quality movie rendering: smooth textures + anti aliasing
	};

//------------------------------------------------------------------------------

void COGL::SetRenderQuality (int nQuality)
{
	static int nCurQual = -1;

if (nQuality < 0)
	nQuality = gameOpts->render.nImageQuality;
if (nCurQual == nQuality)
	return;
nCurQual = nQuality;
m_states.texMagFilter = renderQualities [nQuality].texMagFilter;
m_states.texMinFilter = renderQualities [nQuality].texMinFilter;
m_states.bNeedMipMaps = renderQualities [nQuality].bNeedMipmap;
m_states.bAntiAliasing = renderQualities [nQuality].bAntiAliasing;
if (m_states.bAntiAliasingOk && m_states.bAntiAliasing)
	glEnable (GL_MULTISAMPLE);
else
	glDisable (GL_MULTISAMPLE);
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

void ComputeSinCosTable (int nSides, tSinCosf *sinCosP)
{
	double	ang;

for (int i = 0; i < nSides; i++, sinCosP++) {
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
	if (ogl.m_states.bUseTransform)
		glNormal3f ((GLfloat) X2F ((*pvNormal) [X]),
						(GLfloat) X2F ((*pvNormal) [Y]),
						(GLfloat) X2F ((*pvNormal) [Z]));
		//VmVecAdd (&vNormal, pvNormal, &pointList [0]->p3_vec);
	else {
		transformation.Rotate (vNormal, *pvNormal, 0);
		glNormal3f ((GLfloat) X2F (vNormal [X]),
						(GLfloat) X2F (vNormal [Y]),
						(GLfloat) X2F (vNormal [Z]));
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
		vNormal = CFixVector::Normal (pointList [0]->p3_vec, pointList [1]->p3_vec, pointList [2]->p3_vec);
		glNormal3f ((GLfloat) X2F (vNormal [X]), (GLfloat) X2F (vNormal [Y]), (GLfloat) X2F (vNormal [Z]));
		}
	else {
		int bFlip = GetVertsForNormal (v [0], v [1], v [2], 32767, vSorted);
		vNormal = CFixVector::Normal (gameData.segs.vertices [vSorted [0]],
												gameData.segs.vertices [vSorted [1]],
												gameData.segs.vertices [vSorted [2]]);
		if (bFlip)
			vNormal = -vNormal;
		if (!ogl.m_states.bUseTransform)
			transformation.Rotate (vNormal, vNormal, 0);
		glNormal3f ((GLfloat) X2F (vNormal [X]), (GLfloat) X2F (vNormal [Y]), (GLfloat) X2F (vNormal [Z]));
		}
	}
}

//------------------------------------------------------------------------------

void G3CalcNormal (g3sPoint **pointList, CFloatVector *pvNormal)
{
	CFixVector	vNormal;

if (gameStates.render.nState == 1)	// an object polymodel
	vNormal = CFixVector::Normal (pointList [0]->p3_vec, pointList [1]->p3_vec, pointList [2]->p3_vec);
else {
	short	v [4], vSorted [4];

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
			vNormal.Neg ();
		}
	}
pvNormal->Assign (vNormal);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void COglStates::Initialize (void)
{
memset (this, 0, sizeof (*this));
ogl.m_states.bPerPixelLightingOk = 2;
ogl.m_states.bUseRender2Texture = 1;
ogl.m_states.nContrast = 8;
ogl.m_states.nColorBits = 32;
ogl.m_states.nDepthBits = 24;
ogl.m_states.bEnableTexture2D = -1;
ogl.m_states.bEnableTexClamp = -1;
ogl.m_states.texMinFilter = GL_NEAREST;
ogl.m_states.texMinFilter = GL_NEAREST;
ogl.m_states.nTexMagFilterState = -1, 
ogl.m_states.nTexMinFilterState = -1;
ogl.m_states.nTexEnvModeState = -1, 
ogl.m_states.nLastX =
ogl.m_states.nLastY =
ogl.m_states.nLastW =
ogl.m_states.nLastH = 1;
ogl.m_states.nCurWidth =
ogl.m_states.nCurHeight = -1;
ogl.m_states.bCurFullScreen = -1;
ogl.m_states.bpp = 32;
ogl.m_states.nRGBAFormat = GL_RGBA;
ogl.m_states.nRGBFormat = GL_RGB;
ogl.m_states.bIntensity4 = 1;
ogl.m_states.bLuminance4Alpha4 = 1;
ogl.m_states.nDrawBuffer = -1;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void COglData::Initialize (void)
{
palette = NULL;
nSrcBlend = GL_SRC_ALPHA;
nDestBlend = GL_ONE_MINUS_SRC_ALPHA;
zNear = 1.0f;
zFar = 5000.0f;
depthScale.SetZero ();
screenScale.x =
screenScale.y = 0;
CLEAR (nPerPixelLights);
CLEAR (lightRads);
CLEAR (lightPos);
bLightmaps = 0;
nHeadlights = 0;
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void COGL::Initialize (void)
{
m_states.Initialize ();
m_data.Initialize ();
}

//------------------------------------------------------------------------------

void COGL::InitState (void)
{
// select clearing (background) color
glClearColor (0, 0, 0, 0);
glShadeModel (GL_SMOOTH);
// initialize viewing values
glMatrixMode (GL_PROJECTION);
glLoadIdentity ();
glOrtho (0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
glMatrixMode (GL_MODELVIEW);
glLoadIdentity ();//clear matrix
glScalef (1.0f, -1.0f, 1.0f);
glTranslatef (0.0f, -1.0f, 0.0f);
glDisable (GL_CULL_FACE);
SetDrawBuffer (GL_BACK, 1);
glDisable (GL_SCISSOR_TEST);
glDisable (GL_ALPHA_TEST);
glDisable (GL_DEPTH_TEST);
glDisable (GL_CULL_FACE);
glDisable (GL_STENCIL_TEST);
glDisable (GL_LIGHTING);
glDisable (GL_COLOR_MATERIAL);
glDepthMask (1);
glColorMask (1,1,1,1);
if (m_states.bAntiAliasingOk && m_states.bAntiAliasing)
	glDisable (GL_MULTISAMPLE_ARB);
}

//------------------------------------------------------------------------------

void COGL::BlendFunc (GLenum nSrcBlend, GLenum nDestBlend)
{
ogl.m_data.nSrcBlend = nSrcBlend;
ogl.m_data.nDestBlend = nDestBlend;
glBlendFunc (nSrcBlend, nDestBlend);
}

//------------------------------------------------------------------------------

void COGL::SelectTMU (int nTMU, bool bClient)
{
	int	i = nTMU - GL_TEXTURE0;

#if DBG
if ((i < 0) || (i > 3))
	return;
#endif
if (m_states.nTMU [0] != i) {
	glActiveTexture (nTMU);
	m_states.nTMU [0] = i;
	}	
if (bClient) {
	if (m_states.nTMU [1] != i) {
		glClientActiveTexture (nTMU);
		m_states.nTMU [1] = i;
		}
	}
ClearError (0);
}

//------------------------------------------------------------------------------

int COGL::EnableClientState (GLuint nState, int nTMU)
{
if (nTMU >= GL_TEXTURE0)
	SelectTMU (nTMU, true);
glEnableClientState (nState);
#if DBG_OGL
memset (&m_states.clientBuffers [m_states.nTMU [0]][nState - GL_VERTEX_ARRAY], 0, sizeof (m_states.clientBuffers [m_states.nTMU [0]][nState - GL_VERTEX_ARRAY]));
#endif
if (!glGetError ()) {
	m_states.clientStates [m_states.nTMU [0]][nState - GL_VERTEX_ARRAY] = 1;
	return 1;
	}
DisableClientState (nState, -1);
return glGetError () == 0;
}

//------------------------------------------------------------------------------

int COGL::DisableClientState (GLuint nState, int nTMU)
{
if (nTMU >= GL_TEXTURE0)
	SelectTMU (nTMU, true);
glDisableClientState (nState);
m_states.clientStates [m_states.nTMU [0]][nState - GL_VERTEX_ARRAY] = 0;
#if DBG_OGL
memset (&m_states.clientBuffers [m_states.nTMU [0]][nState - GL_VERTEX_ARRAY], 0, sizeof (m_states.clientBuffers [m_states.nTMU [0]][nState - GL_VERTEX_ARRAY]));
#endif
return glGetError () == 0;
}

//------------------------------------------------------------------------------

int COGL::EnableClientStates (int bTexCoord, int bColor, int bNormals, int nTMU)
{
if (nTMU >= GL_TEXTURE0)
	SelectTMU (nTMU, true);
if (!bNormals) 
	DisableClientState (GL_NORMAL_ARRAY);
else if (!EnableClientState (GL_NORMAL_ARRAY, -1)) {
	DisableClientStates (0, 0, 0, -1);
	return 0;
	}
if (!bTexCoord) 
	DisableClientState (GL_TEXTURE_COORD_ARRAY);
else if (!EnableClientState (GL_TEXTURE_COORD_ARRAY, -1)) {
	DisableClientStates (0, 0, 0, -1);
	return 0;
	}
if (!bColor) 
	DisableClientState (GL_COLOR_ARRAY);
else if (!EnableClientState (GL_COLOR_ARRAY, -1)) {
	DisableClientStates (bTexCoord, 0, 0, -1);
	return 0;
	}
return EnableClientState (GL_VERTEX_ARRAY, -1);
}

//------------------------------------------------------------------------------

void COGL::DisableClientStates (int bTexCoord, int bColor, int bNormals, int nTMU)
{
if (nTMU >= GL_TEXTURE0)
	SelectTMU (nTMU, true);
if (bNormals)
	DisableClientState (GL_NORMAL_ARRAY);
if (bTexCoord)
	DisableClientState (GL_TEXTURE_COORD_ARRAY);
if (bColor)
	DisableClientState (GL_COLOR_ARRAY);
DisableClientState (GL_VERTEX_ARRAY);
}

//------------------------------------------------------------------------------

#define ZNEAR	1
#define ZFAR	ogl.m_data.zFar

void COGL::SetFOV (void)
{
#if 0
gameStates.render.glAspect = 90.0 / gameStates.render.glFOV;
#else
if (ogl.m_states.bUseTransform)
	gameStates.render.glAspect = double (screen.Width ()) / double (screen.Height ());
else
	gameStates.render.glAspect = 1.0;
#endif
glMatrixMode (GL_PROJECTION);
glLoadIdentity ();//clear matrix
if (gameStates.render.bRearView < 0)
	glScalef (-1.0f, 1.0f, 1.0f);
gluPerspective (gameStates.render.glFOV * (double (transformation.m_info.zoom) / 65536.0),
      			 double (CCanvas::Current ()->Width ()) / double (CCanvas::Current ()->Height ()), ZNEAR, ZFAR);
ogl.m_data.depthScale [X] = float (ZFAR / (ZFAR - ZNEAR));
ogl.m_data.depthScale [Y] = float (ZNEAR * ZFAR / (ZNEAR - ZFAR));
ogl.m_data.depthScale [Z] = float (ZFAR - ZNEAR);
ogl.m_data.screenScale.x = 1.0f / float (screen.Width ());
ogl.m_data.screenScale.y = 1.0f / float (screen.Height ());
glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
glMatrixMode (GL_MODELVIEW);
}

//------------------------------------------------------------------------------

void COGL::Viewport (int x, int y, int w, int h)
{
if (!gameOpts->render.cameras.bHires) {
	x >>= gameStates.render.cameras.bActive;
	y >>= gameStates.render.cameras.bActive;
	w >>= gameStates.render.cameras.bActive;
	h >>= gameStates.render.cameras.bActive;
	}
if ((x != ogl.m_states.nLastX) || (y != ogl.m_states.nLastY) || (w != ogl.m_states.nLastW) || (h != ogl.m_states.nLastH)) {
#if !USE_IRRLICHT
	int t = screen.Canvas ()->Height ();
	if (!gameOpts->render.cameras.bHires)
		t >>= gameStates.render.cameras.bActive;
	glViewport ((GLint) x, (GLint) (t - y - h), (GLsizei) w, (GLsizei) h);
#endif
	ogl.m_states.nLastX = x;
	ogl.m_states.nLastY = y;
	ogl.m_states.nLastW = w;
	ogl.m_states.nLastH = h;
	}
}

//------------------------------------------------------------------------------

#define GL_INFINITY	0

void COGL::StartFrame (int bFlat, int bResetColorBuf)
{
	GLint nError = glGetError ();

if (!(gameStates.render.cameras.bActive || gameStates.render.bBriefing))
	ogl.SetDrawBuffer (GL_BACK, 1);
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
	ogl.Viewport (CCanvas::Current ()->Left (), CCanvas::Current ()->Top (), CCanvas::Current ()->Width (), CCanvas::Current ()->Height ());
	if (ogl.m_states.bEnableScissor) {
		glScissor (
			CCanvas::Current ()->Left (),
			screen.Canvas ()->Height () - CCanvas::Current ()->Top () - CCanvas::Current ()->Height (),
			CCanvas::Current ()->Width (),
			CCanvas::Current ()->Height ());
		glEnable (GL_SCISSOR_TEST);
		}
	else
		glDisable (GL_SCISSOR_TEST);
	if (gameStates.render.nRenderPass < 0) {
		glDepthMask (1);
		glColorMask (1,1,1,1);
		glClearColor (0,0,0,0);
#if 0
		if (bResetColorBuf)
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		else
#endif
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
	if (ogl.m_states.bAntiAliasingOk && ogl.m_states.bAntiAliasing)
		glEnable (GL_MULTISAMPLE_ARB);
	if (bFlat) {
		glDisable (GL_DEPTH_TEST);
		glDisable (GL_ALPHA_TEST);
		glDisable (GL_CULL_FACE);
		}
	else {
		glEnable (GL_CULL_FACE);
		glFrontFace (GL_CW);	//Weird, huh? Well, D2 renders everything reverse ...
		glCullFace ((gameStates.render.bRearView < 0) ? GL_FRONT : GL_BACK);
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

void COGL::EndFrame (void)
{
//	ogl.Viewport (CCanvas::Current ()->Left (), CCanvas::Current ()->Top (), );
//	glViewport (0, 0, screen.Width (), screen.Height ());
//OglFlushDrawBuffer ();
//glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
if (!(gameStates.render.cameras.bActive || gameStates.render.bBriefing))
	ogl.SetDrawBuffer (GL_BACK, 1);
if (ogl.m_states.bShadersOk)
	glUseProgramObject (0);
#if 0
// There's a weird effect of string pooling causing the renderer to stutter every time a 
// new string texture is uploaded to the OpenGL driver when depth textures are used.
DestroyGlareDepthTexture ();
#endif
glEnable (GL_TEXTURE_2D);
ogl.DisableClientStates (1, 1, 1, GL_TEXTURE3);
OGL_BINDTEX (0);
ogl.DisableClientStates (1, 1, 1, GL_TEXTURE2);
OGL_BINDTEX (0);
ogl.DisableClientStates (1, 1, 1, GL_TEXTURE1);
OGL_BINDTEX (0);
ogl.DisableClientStates (1, 1, 1, GL_TEXTURE0);
OGL_BINDTEX (0);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
ogl.Viewport (0, 0, screen.Width (), screen.Height ());
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
if (ogl.m_states.bAntiAliasingOk && ogl.m_states.bAntiAliasing)
	glDisable (GL_MULTISAMPLE_ARB);
}

//------------------------------------------------------------------------------

void COGL::EnableLighting (int bSpecular)
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
		glMateriali (GL_FRONT_AND_BACK, GL_SHININESS, (bSpecular < 0) ? 8 : gameOpts->render.bHiresModels [0] ? 127 : 8);
		}
	else
		glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, fBlack);
	glColorMaterial (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable (GL_COLOR_MATERIAL);
	}
}

//------------------------------------------------------------------------------

void COGL::DisableLighting (void)
{
if (gameOpts->ogl.bObjLighting || (gameStates.render.bPerPixelLighting == 2)) {
	glDisable (GL_COLOR_MATERIAL);
	glDisable (GL_LIGHTING);
	}
}

//------------------------------------------------------------------------------

void COGL::SwapBuffers (int bForce, int bClear)
{
if (!gameStates.menus.nInMenu || bForce) {
#	if PROFILING
	if (gameStates.render.bShowProfiler && gameStates.app.bGameRunning && !gameStates.menus.nInMenu && fontManager.Current () && SMALL_FONT) {
		static time_t t0 = -1000;
		time_t t1 = clock ();
		static tProfilerData p;
		static float nFrameCount = 1;
		if (t1 - t0 >= 1000) {
			memcpy (&p, &gameData.profiler, sizeof (p));
			nFrameCount = 1; //float (gameData.app.nFrameCount);
			t0 = t1;
			}
		int h = SMALL_FONT->Height () + 3, i = 3;
		fontManager.SetColorRGBi (ORANGE_RGBA, 1, 0, 0);
		float t, s = 0;
		GrPrintF (NULL, 5, h * i++, "frame: %1.2f", float (p.t [ptFrame]) / nFrameCount);
		if (p.t [ptRenderFrame]) {
			GrPrintF (NULL, 5, h * i++, "  scene: %1.2f %c (%1.2f)", 100.0f * float (p.t [ptRenderFrame]) / float (p.t [ptFrame]), '%', float (p.t [ptRenderFrame]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "    mine: %1.2f %c (%1.2f)", t = 100.0f * float (p.t [ptRenderMine]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptRenderMine]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "    light: %1.2f %c (%1.2f)", t = 100.0f * float (p.t [ptLighting]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptLighting]) / nFrameCount);
			s += t;
			GrPrintF (NULL, 5, h * i++, "    render: %1.2f %c (%1.2f)", t = 100.0f * float (p.t [ptRenderPass]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptRenderPass]) / nFrameCount);
			s += t;
			GrPrintF (NULL, 5, h * i++, "      face list: %1.2f %c (%1.2f)", t = 100.0f * float (p.t [ptFaceList]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptFaceList]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "      faces: %1.2f %c (%1.2f)", t = 100.0f * float (p.t [ptRenderFaces]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptRenderFaces]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "      objects: %1.2f %c (%1.2f) ", t = 100.0f * float (p.t [ptRenderObjects]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptRenderObjects]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "      transparency: %1.2f %c (%1.2f) ", t = 100.0f * float (p.t [ptTranspPolys]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptTranspPolys]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "      effects: %1.2f %c (%1.2f) ", t = 100.0f * float (p.t [ptEffects]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptEffects]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "      cockpit: %1.2f %c (%1.2f) ", t = 100.0f * float (p.t [ptCockpit]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptCockpit]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "      states: %1.2f %c (%1.2f)", t = 100.0f * float (p.t [ptRenderStates]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptRenderStates]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "      shaders: %1.2f %c (%1.2f)", t = 100.0f * float (p.t [ptShaderStates]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptShaderStates]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "    other: %1.2f %c (%1.2f)", t = 100.0f * float (p.t [ptAux]) / float (p.t [ptRenderFrame]), '%');
			s += t;
			GrPrintF (NULL, 5, h * i++, "      transform: %1.2f %c (%1.2f) ", t = 100.0f * float (p.t [ptTransform]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptTransform]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "      seg list: %1.2f %c (%1.2f)", t = 100.0f * float (p.t [ptBuildSegList]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptBuildSegList]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "      obj list: %1.2f %c (%1.2f)", t = 100.0f * float (p.t [ptBuildObjList]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptBuildObjList]) / nFrameCount);
			}
		GrPrintF (NULL, 5, h * i++, "  total: %1.2f %c", s, '%');
		}
#endif
	//if (gameStates.app.bGameRunning && !gameStates.menus.nInMenu)
	paletteManager.ApplyEffect ();
	ogl.FlushDrawBuffer ();
	SDL_GL_SwapBuffers ();
	ogl.SetDrawBuffer (GL_BACK, 1);
#if 1
	//if (gameStates.menus.nInMenu || bClear)
		glClear (GL_COLOR_BUFFER_BIT);
#endif
	}
}

// -----------------------------------------------------------------------------------

void COGL::SetupTransform (int bForce)
{
if (!m_states.nTransformCalls && (m_states.bUseTransform || bForce)) {
	glMatrixMode (GL_MODELVIEW);
	glPushMatrix ();
	glLoadIdentity ();
	glScalef (1, 1, -1);
	glMultMatrixf (reinterpret_cast<GLfloat*> (transformation.m_info.viewf [2].Vec ()));
	glTranslatef (-transformation.m_info.posf [1][0], -transformation.m_info.posf [1][1], -transformation.m_info.posf [1][2]);
	}
++m_states.nTransformCalls;
}

// -----------------------------------------------------------------------------------

void COGL::ResetTransform (int bForce)
{
if ((m_states.nTransformCalls > 0) && !--m_states.nTransformCalls && (m_states.bUseTransform || bForce)) {
	glMatrixMode (GL_MODELVIEW);
	glPopMatrix ();
	}
}

//------------------------------------------------------------------------------

void COGL::DrawArrays (GLenum mode, GLint first, GLsizei count)
{
if (count < 1)
	PrintLog ("glDrawArrays: invalid count\n");
else if (count > 10000)
	PrintLog ("glDrawArrays: suspiciously high count\n");
else if ((mode < 0) || (mode > GL_POLYGON))
	PrintLog ("glDrawArrays: invalid mode\n");
else if (!m_states.clientStates [m_states.nTMU [0]][0])
	PrintLog ("glDrawArrays: client data not enabled\n");
else
	glDrawArrays (mode, first, count);
}

//------------------------------------------------------------------------------

void COGL::RebuildContext (int bGame)
{
m_states.bRebuilding = 1;
backgroundManager.Rebuild ();
if (!gameStates.app.bGameRunning)
	messageBox.Show (" Setting up renderer...");
ResetTextures (1, bGame);
if (bGame) {
	InitShaders ();
	gameData.models.Destroy ();
	gameData.models.Prepare ();
	if (bGame && lightmapManager.HaveLightmaps ())
		lightmapManager.BindAll ();
	gpgpuLighting.End ();
	gpgpuLighting.Begin ();
	CreateDrawBuffer ();
	cameraManager.Create ();
	InitSpheres ();
	cockpit->Rebuild ();
	}
//gameData.models.Prepare ();
if (!gameStates.app.bGameRunning)
	messageBox.Clear ();
SetDrawBuffer (m_states.nDrawBuffer, 1);
m_states.bRebuilding = 0;
}

//------------------------------------------------------------------------------

void COGL::SetScreenMode (void)
{
if ((gameStates.video.nLastScreenMode == gameStates.video.nScreenMode) &&
	 (m_states.bLastFullScreen == m_states.bFullScreen) &&
	 (gameStates.app.bGameRunning || (gameStates.video.nScreenMode == SCREEN_GAME) || (m_states.nDrawBuffer == GL_FRONT)))
	return;
if (gameStates.video.nScreenMode == SCREEN_GAME)
	SetDrawBuffer (GL_BACK, 1);
else {
	SetDrawBuffer (GL_BACK, 1);
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
m_states.bLastFullScreen = m_states.bFullScreen;
}

//------------------------------------------------------------------------------

const char *oglVendor, *oglRenderer, *oglVersion, *oglExtensions;

void COGL::GetVerInfo (void)
{
oglVendor = reinterpret_cast<const char*> (glGetString (GL_VENDOR));
oglRenderer = reinterpret_cast<const char*> (glGetString (GL_RENDERER));
oglVersion = reinterpret_cast<const char*> (glGetString (GL_VERSION));
oglExtensions = reinterpret_cast<const char*> (glGetString (GL_EXTENSIONS));
}

//------------------------------------------------------------------------------

void COGL::CreateDrawBuffer (void)
{
#if FBO_DRAW_BUFFER
if (gameStates.render.bRenderIndirect && ogl.m_states.bRender2TextureOk && !m_data.drawBuffer.Handle ()) {
	PrintLog ("creating draw buffer\n");
	m_data.drawBuffer.Create (ogl.m_states.nCurWidth, ogl.m_states.nCurHeight, 1);
	}
#endif
}

//------------------------------------------------------------------------------

void COGL::DestroyDrawBuffer (void)
{
#if FBO_DRAW_BUFFER
#	if 1
	static int bSemaphore = 0;

if (bSemaphore)
	return;
bSemaphore++;
#	endif
if (ogl.m_states.bRender2TextureOk && m_data.drawBuffer.Handle ()) {
	SetDrawBuffer (GL_BACK, 0);
	m_data.drawBuffer.Destroy ();
	ogl.m_states.bDrawBufferActive = 0;
	}
#	if 1
bSemaphore--;
#	endif
#endif
}

//------------------------------------------------------------------------------

void COGL::SetDrawBuffer (int nBuffer, int bFBO)
{
#if 1
	static int bSemaphore = 0;

if (bSemaphore)
	return;
bSemaphore++;
#endif
#if FBO_DRAW_BUFFER
if (bFBO && (nBuffer == GL_BACK) && m_states.bRender2TextureOk && m_data.drawBuffer.Handle ()) {
	if (!m_states.bDrawBufferActive) {
		if (m_data.drawBuffer.Enable ()) {
			glDrawBuffer (GL_COLOR_ATTACHMENT0_EXT);
			m_states.bDrawBufferActive = 1;
			}
		else {
			DestroyDrawBuffer ();
			CreateDrawBuffer ();
			glDrawBuffer (GL_BACK);
			m_states.bDrawBufferActive = 0;
			}
		}
	}
else {
	if (m_states.bDrawBufferActive) {
		m_data.drawBuffer.Disable ();
		m_states.bDrawBufferActive = 0;
		}
	glDrawBuffer (m_states.nDrawBuffer = nBuffer);
	}
#else
glDrawBuffer (m_states.nDrawBuffer = nBuffer);
#endif
#if 1
bSemaphore--;
#endif
}

//------------------------------------------------------------------------------

void COGL::SetReadBuffer (int nBuffer, int bFBO)
{
#if FBO_DRAW_BUFFER
if (bFBO && (nBuffer == GL_BACK) && m_states.bRender2TextureOk && m_data.drawBuffer.Handle ()) {
	if (!m_states.bReadBufferActive) {
		m_data.drawBuffer.Enable ();
		glReadBuffer (GL_COLOR_ATTACHMENT0_EXT);
		m_states.bReadBufferActive = 1;
		}
	}
else {
	if (m_states.bReadBufferActive) {
		m_data.drawBuffer.Disable ();
		m_states.bReadBufferActive = 0;
		}
	glReadBuffer (nBuffer);
	}
#else
glReadBuffer (nBuffer);
#endif
}

//------------------------------------------------------------------------------

void COGL::FlushDrawBuffer (bool bAdditive)
{
#if FBO_DRAW_BUFFER
if (ogl.HaveDrawBuffer ()) {
	SetDrawBuffer (GL_BACK, 0);
	ogl.SelectTMU (GL_TEXTURE0);
	if (bAdditive) {
		glEnable (GL_BLEND);
		glBlendFunc (GL_ONE, GL_ONE);
		}
	glEnable (GL_TEXTURE_2D);
	glBindTexture (GL_TEXTURE_2D, m_data.drawBuffer.RenderBuffer ());
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
	SetDrawBuffer (GL_BACK, 1);
	}
#endif
}

// -----------------------------------------------------------------------------------

GLuint COGL::CreateDepthTexture (int nTMU, int bFBO)
{
	GLuint	hBuffer;

if (nTMU > 0)
	ogl.SelectTMU (nTMU);
glEnable (GL_TEXTURE_2D);
ogl.GenTextures (1, &hBuffer);
if (glGetError ())
	return hBuffer = 0;
glBindTexture (GL_TEXTURE_2D, hBuffer);
glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, m_states.nCurWidth, m_states.nCurHeight,
				  0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);
if (glGetError ()) {
	ogl.DeleteTextures (1, &hBuffer);
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
	ogl.DeleteTextures (1, &hBuffer);
	return hBuffer = 0;
	}
return hBuffer;
}

// -----------------------------------------------------------------------------------

#if 0

GLuint COGL::CreateStencilTexture (int nTMU, int bFBO)
{
	GLuint	hBuffer;

if (nTMU > 0)
	ogl.SelectTMU (nTMU);
glEnable (GL_TEXTURE_2D);
ogl.GenTextures (1, &hBuffer);
if (glGetError ())
	return hDepthBuffer = 0;
glBindTexture (GL_TEXTURE_2D, hBuffer);
glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
glTexImage2D (GL_TEXTURE_2D, 0, GL_STENCIL_COMPONENT8, m_states.nCurWidth, m_states.nCurHeight,
				  0, GL_STENCIL_COMPONENT, GL_UNSIGNED_BYTE, NULL);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
if (glGetError ()) {
	ogl.DeleteTextures (1, &hBuffer);
	return hBuffer = 0;
	}
return hBuffer;
}

#endif

//------------------------------------------------------------------------------

int COGL::StencilOff (void) 
{
if (!SHOW_SHADOWS || (gameStates.render.nShadowPass != 3))
	return 0;
glDisable (GL_STENCIL_TEST);
m_states.nStencil--;
return 1;
}

//------------------------------------------------------------------------------

void COGL::StencilOn (int bStencil) 
{
if (bStencil) {
	glEnable (GL_STENCIL_TEST);
	m_states.nStencil++;
	}
}

//------------------------------------------------------------------------------

#if DBG

void COGL::VertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, char* pszFile, int nLine) 
{ 
m_states.clientBuffers [m_states.nTMU [0]][0].buffer = pointer;
m_states.clientBuffers [m_states.nTMU [0]][0].pszFile = pszFile;
m_states.clientBuffers [m_states.nTMU [0]][0].nLine = nLine;
glVertexPointer (size, type, stride, pointer); 
}

//------------------------------------------------------------------------------

void COGL::NormalPointer (GLenum type, GLsizei stride, const GLvoid* pointer, char* pszFile, int nLine)
{ 
m_states.clientBuffers [m_states.nTMU [0]][1].buffer = pointer;
m_states.clientBuffers [m_states.nTMU [0]][1].pszFile = pszFile;
m_states.clientBuffers [m_states.nTMU [0]][1].nLine = nLine;
glNormalPointer (type, stride, pointer); 
}

//------------------------------------------------------------------------------

void COGL::ColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, char* pszFile, int nLine)
{
m_states.clientBuffers [m_states.nTMU [0]][2].buffer = pointer;
m_states.clientBuffers [m_states.nTMU [0]][2].pszFile = pszFile;
m_states.clientBuffers [m_states.nTMU [0]][2].nLine = nLine;
glColorPointer (size, type, stride, pointer); 
}

//------------------------------------------------------------------------------

void COGL::TexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, char* pszFile, int nLine)
{ 
m_states.clientBuffers [m_states.nTMU [0]][4].buffer = pointer;
m_states.clientBuffers [m_states.nTMU [0]][4].pszFile = pszFile;
m_states.clientBuffers [m_states.nTMU [0]][4].nLine = nLine;
glTexCoordPointer (size, type, stride, pointer); 
}

#endif

//------------------------------------------------------------------------------

#if DBG

void COGL::GenTextures (GLsizei n, GLuint *hTextures)
{
GLuint nError = glGetError ();
glGenTextures (n, hTextures);
if ((*hTextures == m_data.drawBuffer.RenderBuffer ()) &&
	 (hTextures != &m_data.drawBuffer.RenderBuffer ()))
	DestroyDrawBuffer ();
}

//------------------------------------------------------------------------------

void COGL::DeleteTextures (GLsizei n, GLuint *hTextures)
{
if ((*hTextures == m_data.drawBuffer.RenderBuffer ()) &&
	 (hTextures != &m_data.drawBuffer.RenderBuffer ()))
	DestroyDrawBuffer ();
#if DBG
for (int i = 0; i < n;)
	if (int (hTextures [i]) < 0)
		hTextures [i] = hTextures [--n];
	else
		i++;
if (n)
#endif
glDeleteTextures (n, hTextures);
}

#endif

//------------------------------------------------------------------------------
