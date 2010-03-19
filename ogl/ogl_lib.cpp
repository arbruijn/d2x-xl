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
#include "renderframe.h"
#include "automap.h"
#include "gpgpu_lighting.h"

//#define _WIN32_WINNT		0x0600

#define TRACK_STATES		0

#ifdef _WIN32

typedef struct tLibList {
	int	nLibs;
	DWORD	*libs;
} tLibList;

tLibList libList = {0, NULL};

static DWORD nOglLibFlags [2] = {1680960820, (DWORD) -1};

#endif

COGL ogl;

#define ZNEAR		1.0
#define ZSCREEN	(automap.Display () ? 20.0 : 10.0)
#define ZFAR		ogl.m_data.zFar

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

double COGL::ZScreen (void)
{
return double (nScreenDists [gameOpts->render.nScreenDist] * (automap.Display () + 1));
}

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
	if (ogl.UseTransform ())
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
		if (!ogl.UseTransform ())
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
bPerPixelLightingOk = 2;
bUseRender2Texture = 1;
nContrast = 8;
nColorBits = 32;
nDepthBits = 24;
bEnableTexture2D = -1;
bEnableTexClamp = -1;
texMinFilter = GL_NEAREST;
texMinFilter = GL_NEAREST;
nTexMagFilterState = -1,
nTexMinFilterState = -1;
nTexEnvModeState = -1,
nLastX =
nLastY =
nLastW =
nLastH = 1;
nCurWidth =
nCurHeight = -1;
bCurFullScreen = -1;
bpp = 32;
nRGBAFormat = GL_RGBA;
nRGBFormat = GL_RGB;
bIntensity4 = 1;
bLuminance4Alpha4 = 1;
nDrawBuffer = -1;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void COglData::Initialize (void)
{
palette = NULL;
memset (bUseTextures, 0, sizeof (bUseTextures));
nTMU [0] = 0;
if (gameStates.app.bInitialized && ogl.m_states.bInitialized) {
#ifndef GL_VERSION_20
	if (glActiveTexture) 
#endif
		{
		glActiveTexture (GL_TEXTURE3);
		glBindTexture (GL_TEXTURE_2D, nTexture [3] = 0);
		glDisable (GL_TEXTURE_2D);
		glActiveTexture (GL_TEXTURE2);
		glBindTexture (GL_TEXTURE_2D, nTexture [2] = 0);
		glDisable (GL_TEXTURE_2D);
		glActiveTexture (GL_TEXTURE1);
		glBindTexture (GL_TEXTURE_2D, nTexture [1] = 0);
		glDisable (GL_TEXTURE_2D);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, nTexture [0] = 0);
		glDisable (GL_TEXTURE_2D);
		}
	bUseBlending = true;
	glEnable (GL_BLEND);
	glBlendFunc (nSrcBlendMode = GL_SRC_ALPHA, nDestBlendMode = GL_ONE_MINUS_SRC_ALPHA);
	bDepthTest = false;
	glDisable (GL_DEPTH_TEST);
	bDepthWrite = false;
	glDepthMask (0);
	bStencilTest = false;
	glDisable (GL_STENCIL_TEST);
	bCullFaces = false;
	glDisable (GL_CULL_FACE);
	glCullFace (nCullMode = GL_BACK);
	bScissorTest = false;
	glDisable (GL_SCISSOR_TEST);
	bAlphaTest = false;
	glDisable (GL_ALPHA_TEST);
	bLineSmooth = false;
	glDisable (GL_LINE_SMOOTH);
	bLighting = false;
	glDisable (GL_LIGHTING);
	bPolyOffsetFill = false;
	glDisable (GL_POLYGON_OFFSET_FILL);
	}
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
drawBufferP = drawBuffers;

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
glClearColor (0,0,0,1);
glShadeModel (GL_SMOOTH);
// initialize viewing values
glMatrixMode (GL_PROJECTION);
glLoadIdentity ();
glOrtho (0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
glMatrixMode (GL_MODELVIEW);
glLoadIdentity ();//clear matrix
glScalef (1.0f, -1.0f, 1.0f);
glTranslatef (0.0f, -1.0f, 0.0f);
SetDrawBuffer (GL_BACK, 1);
SetFaceCulling (false);
SetScissorTest (false);
SetAlphaTest (false);
SetDepthTest (false);
SetStencilTest (false);
SetStencilTest (false);
SetLighting (false);
glDisable (GL_COLOR_MATERIAL);
ogl.SetDepthWrite (true);
ColorMask (1,1,1,1,1);
if (m_states.bAntiAliasingOk && m_states.bAntiAliasing)
	glDisable (GL_MULTISAMPLE_ARB);
}

//------------------------------------------------------------------------------

void COGL::SelectTMU (int nTMU, bool bClient)
{
	int	i = nTMU - GL_TEXTURE0;

#if DBG
if ((i < 0) || (i > 3))
	return;
#endif
//if (m_data.nTMU [0] != i)
	{
	glActiveTexture (nTMU);
	m_data.nTMU [0] = i;
	}
if (bClient) {
	//if (m_data.nTMU [1] != i)
		{
		glClientActiveTexture (nTMU);
		m_data.nTMU [1] = i;
		}
	}
ClearError (0);
}

//------------------------------------------------------------------------------

int COGL::EnableClientState (GLuint nState, int nTMU)
{
if (nTMU >= GL_TEXTURE0)
	SelectTMU (nTMU, true);
#if TRACK_STATES
if (m_data.clientStates [m_data.nTMU [0]][nState - GL_VERTEX_ARRAY])
	return 1;
#endif
glEnableClientState (nState);
#if DBG_OGL
memset (&m_data.clientBuffers [m_data.nTMU [0]][nState - GL_VERTEX_ARRAY], 0, sizeof (m_data.clientBuffers [m_data.nTMU [0]][nState - GL_VERTEX_ARRAY]));
#endif
if (!glGetError ()) {
#if TRACK_STATES || DBG_OGL
	m_data.clientStates [m_data.nTMU [0]][nState - GL_VERTEX_ARRAY] = 1;
#endif
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
#if TRACK_STATES
if (!m_data.clientStates [m_data.nTMU [0]][nState - GL_VERTEX_ARRAY])
#endif
	{
	glDisableClientState (nState);
#if TRACK_STATES || DBG_OGL
	m_data.clientStates [m_data.nTMU [0]][nState - GL_VERTEX_ARRAY] = 0;
#endif
	}
#if DBG_OGL
//memset (&m_data.clientBuffers [m_data.nTMU [0]][nState - GL_VERTEX_ARRAY], 0, sizeof (m_data.clientBuffers [m_data.nTMU [0]][nState - GL_VERTEX_ARRAY]));
#endif
return glGetError () == 0;
}

//------------------------------------------------------------------------------

int COGL::EnableClientStates (int bTexCoord, int bColor, int bNormals, int nTMU)
{
if (nTMU >= GL_TEXTURE0) {
	SelectTMU (nTMU, true);
	SetTexturing (true);
	}
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
SetTexturing (false);
}

//------------------------------------------------------------------------------

void COGL::ResetClientStates (int nFirst)
{
for (int i = 3; i >= nFirst; i--) {
	DisableClientStates (1, 1, 1, GL_TEXTURE0 + i);
	SetTexturing (true);
	ogl.BindTexture (0);
	if (i)
		SetTexturing (false);
	}
memset (m_data.clientStates + nFirst, 0, (4 - nFirst) * sizeof (m_data.clientStates [0]));
}

//------------------------------------------------------------------------------

inline double DegToRad (double d)
{
return d * (Pi / 180.0);
}


void COGL::SetupFrustum (void)
{
double fovy = gameStates.render.glFOV * X2D (transformation.m_info.zoom);
double aspect = double (CCanvas::Current ()->Width ()) / double (CCanvas::Current ()->Height ());
double h = ZNEAR * tan (DegToRad (fovy * 0.5));
double w = aspect * h;
double shift = X2D (StereoSeparation ()) / 2.0 * ZNEAR / ZScreen ();
if (shift < 0)
	glFrustum (-w - shift, w - shift, -h, h, ZNEAR, ZFAR);
else 
	glFrustum (-w + shift, w + shift, -h, h, ZNEAR, ZFAR);
}

//------------------------------------------------------------------------------

void COGL::SetupProjection (void)
{
#if 0
gameStates.render.glAspect = 90.0 / gameStates.render.glFOV;
#else
if (m_states.bUseTransform)
	gameStates.render.glAspect = double (screen.Width ()) / double (screen.Height ());
else
	gameStates.render.glAspect = 1.0;
#endif
glMatrixMode (GL_PROJECTION);
glLoadIdentity ();//clear matrix
if (StereoSeparation () && (gameOpts->render.n3DMethod == STEREO_PARALLEL))
	SetupFrustum ();
else {
	gluPerspective (gameStates.render.glFOV * X2D (transformation.m_info.zoom),
		   			 double (CCanvas::Current ()->Width ()) / double (CCanvas::Current ()->Height ()), ZNEAR, ZFAR);
	}
if (gameStates.render.bRearView < 0)
	glScalef (-1.0f, 1.0f, 1.0f);
m_data.depthScale [X] = float (ZFAR / (ZFAR - ZNEAR));
m_data.depthScale [Y] = float (ZNEAR * ZFAR / (ZNEAR - ZFAR));
m_data.depthScale [Z] = float (ZFAR - ZNEAR);
m_data.screenScale.x = 1.0f / float (screen.Width ());
m_data.screenScale.y = 1.0f / float (screen.Height ());
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
if ((x != m_states.nLastX) || (y != m_states.nLastY) || (w != m_states.nLastW) || (h != m_states.nLastH)) {
#if !USE_IRRLICHT
	int t = screen.Canvas ()->Height ();
	if (!gameOpts->render.cameras.bHires)
		t >>= gameStates.render.cameras.bActive;
	glViewport ((GLint) x, (GLint) (t - y - h), (GLsizei) w, (GLsizei) h);
#endif
	m_states.nLastX = x;
	m_states.nLastY = y;
	m_states.nLastW = w;
	m_states.nLastH = h;
	}
}

//------------------------------------------------------------------------------

void COGL::ColorMask (GLboolean bRed, GLboolean bGreen, GLboolean bBlue, GLboolean bAlpha, GLboolean bEyeOffset) 
{
if (!bEyeOffset || !gameOpts->render.n3DGlasses || gameOpts->render.bEnhance3D)
	glColorMask (bRed, bGreen, bBlue, bAlpha);
else if (gameOpts->render.n3DGlasses == GLASSES_AMBER_BLUE) {	//colorcode 3-d (amber/blue)
	if ((m_data.xStereoSeparation <= 0) != gameOpts->render.bFlipFrames)
		glColorMask (bRed, bGreen, GL_FALSE, bAlpha);
	else
		glColorMask (GL_FALSE, GL_FALSE, bBlue, bAlpha);
	}
else if (gameOpts->render.n3DGlasses == GLASSES_RED_CYAN) {	//red/cyan
	if ((m_data.xStereoSeparation <= 0) != gameOpts->render.bFlipFrames)
		glColorMask (bRed, GL_FALSE, GL_FALSE, bAlpha);
	else
		glColorMask (GL_FALSE, bGreen, bBlue, bAlpha);
	}
else if (gameOpts->render.n3DGlasses == GLASSES_GREEN_MAGENTA) {	//blue/red
	if ((m_data.xStereoSeparation <= 0) != gameOpts->render.bFlipFrames)
		glColorMask (GL_FALSE, bGreen, GL_FALSE, bAlpha);
	else
		glColorMask (bRed, GL_FALSE, bBlue, bAlpha);
	}
else //GLASSES_SHUTTER or NONE
	glColorMask (bRed, bGreen, bBlue, bAlpha);
}

//------------------------------------------------------------------------------

void COGL::ChooseDrawBuffer (void)
{
if (gameStates.render.bBriefing) {
	gameStates.render.bRenderIndirect = 0;
	SetDrawBuffer (GL_BACK, 0);
	}
else if (gameStates.render.cameras.bActive)
	gameStates.render.bRenderIndirect = 0;
else {
	int i = Enhance3D ();
	if (i < 0) {
		ogl.ClearError (0);
		SetDrawBuffer ((m_data.xStereoSeparation < 0) ? GL_BACK_LEFT : GL_BACK_RIGHT, 0);
		if (ogl.ClearError (0))
			gameOpts->render.n3DGlasses = 0;
		}	
	else {
		gameStates.render.bRenderIndirect = m_data.xStereoSeparation && (i > 0);
		if (gameStates.render.bRenderIndirect)
			SelectDrawBuffer ((i > 0) && (m_data.xStereoSeparation > 0));
		else
			SetDrawBuffer (GL_BACK, 0);
		}
	}
}

//------------------------------------------------------------------------------

#define GL_INFINITY	0

void COGL::StartFrame (int bFlat, int bResetColorBuf, fix xStereoSeparation)
{
	GLint nError = glGetError ();

m_data.xStereoSeparation = xStereoSeparation;
ChooseDrawBuffer ();
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
			SetDepthTest (true);
			SetStencilTest (false);
			SetDepthMode (GL_LESS);
			SetFaceCulling (true);
			OglCullFace (0);
			if (!FAST_SHADOWS)
				ColorMask (0,0,0,0,0);
			}
		}
	else if (gameStates.render.nShadowPass == 2) {	//render occluders / shadow maps
		if (gameStates.render.bShadowMaps) {
			ColorMask (0,0,0,0,0);
			ogl.SetPolyOffsetFill (true);
			glPolygonOffset (1.0f, 2.0f);
			}
		else {
#	if DBG_SHADOWS
			if (bShadowTest) {
				ColorMask (1,1,1,1,0);
				SetDepthWrite (false);
				SetBlendMode (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				SetStencilTest (false);
				}
			else
#	endif
			 {
				ColorMask (0,0,0,0,0);
				SetDepthWrite (false);
				SetStencilTest (true);
				if (!glIsEnabled (GL_STENCIL_TEST)) {
					SetStencilTest (false);
					extraGameInfo [0].bShadows =
					extraGameInfo [1].bShadows = 0;
					}
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
				ogl.SetPolyOffsetFill (true);
				glPolygonOffset (1.0f, 1.0f);
#endif
				}
			}
		}
	else if (gameStates.render.nShadowPass == 3) { //render final lit scene
		if (gameStates.render.bShadowMaps) {
			ogl.SetPolyOffsetFill (false);
			SetDepthMode (GL_LESS);
			}
		else {
#if 0
			ogl.SetPolyOffsetFill (false);
#endif
			if (gameStates.render.nShadowBlurPass == 2)
				SetStencilTest (false);
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
			ogl.SetDepthMode (GL_LESS);
			ColorMask (1,1,1,1,1);
			}
		}
	else if (gameStates.render.nShadowPass == 4) {	//render unlit/final scene
		SetDepthTest (true);
		SetDepthMode (GL_LESS);
		SetFaceCulling (true);
		OglCullFace (0);
		}
#if GL_INFINITY
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
#endif
	}
else {
	r_polyc =
	r_tpolyc =
	r_tvertexc =
	r_bitmapc =
	r_ubitmapc =
	r_ubitbltc =
	r_upixelc = 0;

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	Viewport (CCanvas::Current ()->Left (), CCanvas::Current ()->Top (), CCanvas::Current ()->Width (), CCanvas::Current ()->Height ());
	if (m_states.bEnableScissor) {
		glScissor (
			CCanvas::Current ()->Left (),
			screen.Canvas ()->Height () - CCanvas::Current ()->Top () - CCanvas::Current ()->Height (),
			CCanvas::Current ()->Width (),
			CCanvas::Current ()->Height ());
		ogl.SetScissorTest (true);
		}
	else
		ogl.SetScissorTest (false);
	if (gameStates.render.nRenderPass < 0) {
		ogl.SetDepthWrite (true);
		ColorMask (1,1,1,1,1);
		glClearColor (0,0,0,1);
#if 1
		if (bResetColorBuf && (automap.Display () || gameStates.render.bRenderIndirect))
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		else
#endif
			glClear (GL_DEPTH_BUFFER_BIT);
		}
	else if (gameStates.render.nRenderPass) {
		SetDepthWrite (false);
		ColorMask (1,1,1,1,1);
		glClearColor (0,0,0,1);
		if (bResetColorBuf)
			glClear (GL_COLOR_BUFFER_BIT);
		}
	else { //make a depth-only render pass first to decrease bandwidth waste due to overdraw
		ogl.SetDepthWrite (true);
		ColorMask (0,0,0,0,0);
		glClear (GL_DEPTH_BUFFER_BIT);
		}
	if (m_states.bAntiAliasingOk && m_states.bAntiAliasing)
		glEnable (GL_MULTISAMPLE_ARB);
	if (bFlat) {
		SetDepthTest (false);
		SetAlphaTest (false);
		SetFaceCulling (false);
		}
	else {
		SetFaceCulling (true);
		glFrontFace (GL_CW);	//Weird, huh? Well, D2 renders everything reverse ...
		SetCullMode ((gameStates.render.bRearView < 0) ? GL_FRONT : GL_BACK);
		SetDepthTest (true);
		SetDepthMode (GL_LESS);
		SetAlphaTest (true);
		glAlphaFunc (GL_GEQUAL, (float) 0.01);
		}
	SetBlending (true);
	SetBlendMode (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	SetStencilTest (false);
	}
nError = glGetError ();
}

//------------------------------------------------------------------------------

void COGL::EndFrame (void)
{
//	Viewport (CCanvas::Current ()->Left (), CCanvas::Current ()->Top (), );
//	glViewport (0, 0, screen.Width (), screen.Height ());
//OglFlushDrawBuffer ();
//glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
if (!(gameStates.render.cameras.bActive || gameStates.render.bBriefing)) {
	if (gameStates.render.bRenderIndirect)
		SelectDrawBuffer (0);
	SetDrawBuffer (GL_BACK, gameStates.render.bRenderIndirect);
	}
if (m_states.bShadersOk)
	shaderManager.Deploy (-1);
#if 0
// There's a weird effect of string pooling causing the renderer to stutter every time a
// new string texture is uploaded to the OpenGL driver when depth textures are used.
DestroyGlareDepthTexture ();
#endif
ogl.SetTexturing (true);
DisableClientStates (1, 1, 1, GL_TEXTURE3);
ogl.BindTexture (0);
DisableClientStates (1, 1, 1, GL_TEXTURE2);
ogl.BindTexture (0);
DisableClientStates (1, 1, 1, GL_TEXTURE1);
ogl.BindTexture (0);
DisableClientStates (1, 1, 1, GL_TEXTURE0);
ogl.BindTexture (0);
SetBlendMode (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
Viewport (0, 0, screen.Width (), screen.Height ());
#ifndef NMONO
//	merge_textures_stats ();
//	ogl_texture_stats ();
#endif
glMatrixMode (GL_PROJECTION);
glLoadIdentity ();//clear matrix
glOrtho (0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
glMatrixMode (GL_MODELVIEW);
glLoadIdentity ();//clear matrix
SetScissorTest (false);
SetAlphaTest (false);
SetDepthTest (false);
SetFaceCulling (false);
SetStencilTest (false);
if (SHOW_DYN_LIGHT) {
	SetLighting (false);
	glDisable (GL_COLOR_MATERIAL);
	}
ogl.SetDepthWrite (true);
//SetStereoSeparation (0);
ColorMask (1,1,1,1,0);
if (m_states.bAntiAliasingOk && m_states.bAntiAliasing)
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

	SetLighting (true);
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
	SetLighting (false);
	glDisable (GL_COLOR_MATERIAL);
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
	++m_states.nTransformCalls;
	}
}

// -----------------------------------------------------------------------------------

void COGL::ResetTransform (int bForce)
{
if ((m_states.nTransformCalls > 0) && (m_states.bUseTransform || bForce) && !--m_states.nTransformCalls) {
	glMatrixMode (GL_MODELVIEW);
	glPopMatrix ();
	}
}

//------------------------------------------------------------------------------

void COGL::DrawArrays (GLenum mode, GLint first, GLsizei count)
{
if (count < 1)
	PrintLog ("glDrawArrays: invalid count\n");
else if (count > 100000)
	PrintLog ("glDrawArrays: suspiciously high count\n");
else if (mode > GL_POLYGON)
	PrintLog ("glDrawArrays: invalid mode\n");
#if TRACK_STATES || DBG_OGL
else if (!m_data.clientStates [m_data.nTMU [0]][0])
	PrintLog ("glDrawArrays: client data not enabled\n");
#endif
else
	glDrawArrays (mode, first, count);
}

//------------------------------------------------------------------------------

void COGL::RebuildContext (int bGame)
{
m_states.bRebuilding = 1;
m_data.Initialize ();
SetupExtensions ();
backgroundManager.Rebuild ();
if (!gameStates.app.bGameRunning)
	messageBox.Show (" Setting up renderer...");
ResetClientStates ();
ResetTextures (1, bGame);
if (bGame) {
	InitShaders ();
	ClearError (0);
	gameData.models.Destroy ();
	gameData.models.Prepare ();
	if (bGame && lightmapManager.HaveLightmaps ())
		lightmapManager.BindAll ();
#if GPGPU_VERTEX_LIGHTING
	gpgpuLighting.End ();
	gpgpuLighting.Begin ();
#endif
	SelectDrawBuffer (0);
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
m_data.Initialize ();
if (gameStates.video.nScreenMode == SCREEN_GAME)
	SetDrawBuffer (GL_BACK, 1);
else {
	SetDrawBuffer (GL_BACK, 1);
	if (!(gameStates.app.bGameRunning && gameOpts->menus.nStyle)) {
		glClearColor (0,0,0,1);
		glClear (GL_COLOR_BUFFER_BIT);
		glMatrixMode (GL_PROJECTION);
		glLoadIdentity ();//clear matrix
		glOrtho (0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
		glMatrixMode (GL_MODELVIEW);
		glLoadIdentity ();//clear matrix
		SetBlending (true);
		SetBlendMode (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		SetTexturing (false);
		SetDepthMode (GL_ALWAYS); //LEQUAL);
		SetDepthTest (false);
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

#if 0

GLuint COGL::CreateStencilTexture (int nTMU, int bFBO)
{
	GLuint	hBuffer;

if (nTMU > 0)
	SelectTMU (nTMU);
ogl.SetTexturing (true);
GenTextures (1, &hBuffer);
if (glGetError ())
	return hDepthBuffer = 0;
ogl.BindTexture (hBuffer);
glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
glTexImage2D (GL_TEXTURE_2D, 0, GL_STENCIL_COMPONENT8, m_states.nCurWidth, m_states.nCurHeight,
				  0, GL_STENCIL_COMPONENT, GL_UNSIGNED_BYTE, NULL);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
if (glGetError ()) {
	DeleteTextures (1, &hBuffer);
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
SetStencilTest (false);
m_states.nStencil--;
return 1;
}

//------------------------------------------------------------------------------

void COGL::StencilOn (int bStencil)
{
if (bStencil) {
	ogl.SetStencilTest (true);
	m_states.nStencil++;
	}
}

//------------------------------------------------------------------------------

#if DBG_OGL

void COGL::GenTextures (GLsizei n, GLuint *hTextures)
{
glGenTextures (n, hTextures);
#if 0
if ((*hTextures == DrawBuffer ()->ColorBuffer ()) &&
	 (hTextures != &DrawBuffer ()->ColorBuffer ()))
	DestroyDrawBuffers ();
#endif
}

//------------------------------------------------------------------------------

void COGL::DeleteTextures (GLsizei n, GLuint *hTextures)
{
if ((*hTextures == DrawBuffer ()->ColorBuffer ()) &&
	 (hTextures != &DrawBuffer ()->ColorBuffer ()))
	DestroyDrawBuffers ();
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
//eof
