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
#	pragma pack(push)
#	pragma pack(8)
#	include <windows.h>
#	pragma pack(pop)
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
#include "segmath.h"
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
#include "text.h"
#include "postprocessing.h"
#include "gpgpu_lighting.h"

//#define _WIN32_WINNT		0x0600

#define TRACK_STATES		0

#ifdef _WIN32

typedef struct tLibList {
	int32_t	nLibs;
	DWORD	*libs;
} tLibList;

tLibList libList = {0, NULL};

static DWORD nOglLibFlags [2] = {1680960820, (DWORD) -1};

#endif

COGL ogl;

#define ZSCREEN	(automap.Active () ? 20.0 : 10.0)

//------------------------------------------------------------------------------

#if DBG_SHADOWS
int32_t bShadowTest = 0;
#endif

int32_t bSingleStencil = 0;

extern int32_t bZPass;

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

int32_t r_polyc, r_tpolyc, r_bitmapc, r_ubitmapc, r_ubitbltc, r_upixelc, r_tvertexc;
int32_t r_texcount = 0;
int32_t gr_renderstats = 0;
int32_t gr_badtexture = 0;

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
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CViewport::Apply (int32_t t) 
{ 
if (t > 0)
	m_t = t;
else if (m_t < 0)
	m_t = m_h;
glViewport ((GLint) m_x, (GLint) (m_t - m_y - m_h), (GLsizei) m_w, (GLsizei) m_h); 
glScissor (m_x, m_t - m_y - m_h, m_w, m_h);
ogl.SetScissorTest (ogl.m_states.bEnableScissor != 0);
}

//------------------------------------------------------------------------------

void CViewport::Setup (int32_t x, int32_t y, int32_t w, int32_t h) 
{
CRectangle::Setup (x, y, w, h);
m_t = gameData.renderData.screen.Height ();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

double COGL::ZScreen (void)
{
return double (nScreenDists [gameOpts->render.stereo.nScreenDist] * (automap.Active () + 1));
}

//------------------------------------------------------------------------------

void COGL::SetRenderQuality (int32_t nQuality)
{
	static int32_t nCurQual = -1;

if (nQuality < 0)
	nQuality = gameOpts->render.nImageQuality;
if (nCurQual == nQuality)
	return;
nCurQual = nQuality;
m_states.texMagFilter = renderQualities [nQuality].texMagFilter;
m_states.texMinFilter = renderQualities [nQuality].texMinFilter;
m_states.bNeedMipMaps = renderQualities [nQuality].bNeedMipmap;
m_features.bAntiAliasing = renderQualities [nQuality].bAntiAliasing;
if (m_features.bAntiAliasing/*.Apply ()*/)
	glEnable (GL_MULTISAMPLE);
else
	glDisable (GL_MULTISAMPLE);
ResetTextures (1, gameStates.app.bGameRunning);
}

//------------------------------------------------------------------------------

int32_t COGL::StereoDevice (void) 
{ 
if (!m_features.bShaders)
	return 0;
if ((gameOpts->render.stereo.nGlasses == GLASSES_SHUTTER_NVIDIA) && !m_states.nStereo)
	return 0;
return (gameOpts->render.stereo.nGlasses < DEVICE_STEREO_PHYSICAL) ? gameOpts->render.stereo.nGlasses : -gameOpts->render.stereo.nGlasses;
}

//------------------------------------------------------------------------------

void COGL::StartStereoFrame (void)
{
	gameData.renderData.vr.StartFrame ();
}

//------------------------------------------------------------------------------

void COGL::FinishStereoPart (void)
{
	//gameData.renderData.vr.StartFrame ();
	FlushVRBuffers ();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void OglDeleteLists (GLuint *lp, int32_t n)
{
for (; n; n--, lp++) {
	if (*lp) {
		glDeleteLists (*lp, 1);
		*lp = 0;
		}
	}
}

//------------------------------------------------------------------------------

void ComputeSinCosTable (int32_t nSides, tSinCosf *pSinCos)
{
	double	ang;

for (int32_t i = 0; i < nSides; i++, pSinCos++) {
	ang = 2.0 * PI * i / nSides;
	pSinCos->fSin = (float) sin (ang);
	pSinCos->fCos = (float) cos (ang);
	}
}

//------------------------------------------------------------------------------

int32_t CircleListInit (int32_t nSides, int32_t nType, int32_t mode)
{
	int32_t h = glGenLists (1);

glNewList (h, mode);
/* draw a unit radius circle in xy plane centered on origin */
OglDrawCircle (nSides, nType);
glEndList ();
return h;
}

//------------------------------------------------------------------------------

void G3Normal (CRenderPoint **pointList, CFixVector *pvNormal)
{
CFixVector	vNormal;

#if 1
if (pvNormal) {
	if (ogl.UseTransform ())
		glNormal3f ((GLfloat) X2F ((*pvNormal).v.coord.x),
						(GLfloat) X2F ((*pvNormal).v.coord.y),
						(GLfloat) X2F ((*pvNormal).v.coord.z));
	else {
		transformation.Rotate (vNormal, *pvNormal, 0);
		glNormal3f ((GLfloat) X2F (vNormal.v.coord.x),
						(GLfloat) X2F (vNormal.v.coord.y),
						(GLfloat) X2F (vNormal.v.coord.z));
		}
	}
else
#endif
 {
	uint16_t v [4], vSorted [4];

	v [0] = pointList [0]->Index ();
	v [1] = pointList [1]->Index ();
	v [2] = pointList [2]->Index ();
	if ((v [0] < 0) || (v [1] < 0) || (v [2] < 0)) {
		vNormal = CFixVector::Normal (pointList [0]->ViewPos (), pointList [1]->ViewPos (), pointList [2]->ViewPos ());
		glNormal3f ((GLfloat) X2F (vNormal.v.coord.x), (GLfloat) X2F (vNormal.v.coord.y), (GLfloat) X2F (vNormal.v.coord.z));
		}
	else {
		int32_t bFlip = SortVertsForNormal (v [0], v [1], v [2], 0xFFFF, vSorted);
		vNormal = CFixVector::Normal (gameData.segData.vertices [vSorted [0]],
												gameData.segData.vertices [vSorted [1]],
												gameData.segData.vertices [vSorted [2]]);
		if (bFlip)
			vNormal.Neg ();
		if (!ogl.UseTransform ())
			transformation.Rotate (vNormal, vNormal, 0);
		glNormal3f ((GLfloat) X2F (vNormal.v.coord.x), (GLfloat) X2F (vNormal.v.coord.y), (GLfloat) X2F (vNormal.v.coord.z));
		}
	}
}

//------------------------------------------------------------------------------

void G3CalcNormal (CRenderPoint **pointList, CFloatVector *pvNormal)
{
	CFixVector	vNormal;

if (gameStates.render.nState == 1)	// an object polymodel
	vNormal = CFixVector::Normal (pointList [0]->ViewPos (), pointList [1]->ViewPos (), pointList [2]->ViewPos ());
else {
	uint16_t v [4], vSorted [4];

	v [0] = pointList [0]->Index ();
	v [1] = pointList [1]->Index ();
	v [2] = pointList [2]->Index ();

	if ((v [0] < 0) || (v [1] < 0) || (v [2] < 0)) {
		vNormal = CFixVector::Normal (pointList [0]->ViewPos (), pointList [1]->ViewPos (), pointList [2]->ViewPos ());
		}
	else {
		int32_t bFlip = SortVertsForNormal (v [0], v [1], v [2], 0xFFFF, vSorted);
		vNormal = CFixVector::Normal (gameData.segData.vertices [vSorted [0]],
												gameData.segData.vertices [vSorted [1]],
												gameData.segData.vertices [vSorted [2]]);
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
viewport [0] = CViewport (),
viewport [1] = CViewport (),
nCurWidth =
nCurHeight = -1;
bCurFullScreen = -1;
bCurBorderless = -1;
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
nTMU [0] = 
nTMU [1] = 0;
if (/*gameStates.app.bInitialized &&*/ ogl.m_states.bInitialized) {
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
	glGetFloatv (GL_SMOOTH_LINE_WIDTH_RANGE, lineWidthRange);
	}
zNear = 1.0f;
zFar = 5000.0f;
depthScale.SetZero ();
windowScale.dim.x =
windowScale.dim.y = 0;
CLEAR (nPerPixelLights);
CLEAR (lightRads);
CLEAR (lightPos);
bLightmaps = 0;
nHeadlights = 0;
pDrawBuffer = &drawBuffers [0];
#if DBG_OGL
memset (clientBuffers, sizeof (clientBuffers), 0);
#endif
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//static SDL_mutex* semaphore = NULL;

void COGL::Initialize (void)
{
m_features.bPerPixelLighting = 2;
m_features.bRenderToTexture = 1;
#if DBG_OGL
m_bLocked = 0;
#endif
//m_states.Initialize ();
//m_data.Initialize ();
//if (!semaphore)
//	semaphore = SDL_CreateMutex ();
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
shaderManager.Deploy (-1);
ColorMask (1,1,1,1,1);
if (m_features.bAntiAliasing/*.Apply ()*/)
	glDisable (GL_MULTISAMPLE_ARB);
}

//------------------------------------------------------------------------------

inline double DegToRad (double d)
{
return d * (PI / 180.0);
}


void COGL::SetupFrustum (fix xStereoSeparation)
{
double h = ZNEAR * tan (DegToRad (gameStates.render.glFOV * X2D (transformation.m_info.zoom) * 0.5));
double w = h * CCanvas::Current ()->AspectRatio ();
double shift = X2D (xStereoSeparation) / 2.0 * ZNEAR / ZScreen ();
if (shift < 0)
	glFrustum (-w - shift, w - shift, -h, h, ZNEAR, ZFAR);
else 
	glFrustum (-w + shift, w + shift, -h, h, ZNEAR, ZFAR);
}

//------------------------------------------------------------------------------

void COGL::SetupProjection (CTransformation& transformation)
{
ogl.ClearError (0);
gameStates.render.glAspect = m_states.bUseTransform ? gameData.renderData.frame.AspectRatio () : 1.0;
glMatrixMode (GL_PROJECTION);
glLoadIdentity ();//clear matrix
float aspectRatio = VRActive () ? 0.8f : 1.0f; 
float canvasAspectRatio = CCanvas::Current()->AspectRatio ();
#if 1
#	if 1 //DBG
gameStates.render.glFOV = VRActive () ? gameData.renderData.vr.m_fov + gameOpts->render.stereo.nVRFOV * 5 : gameStates.render.nShadowMap ? 90.0 : 105.0; 
#	else
gameStates.render.glFOV = VRActive () ? gameData.renderData.vr.m_fov : gameStates.render.nShadowMap ? 90.0 : 105.0; 
#	endif
ZFAR = gameStates.render.nShadowMap ? 400.0f : 5000.0f;
#else
gameStates.render.glFOV = 180.0;
#endif
if (!StereoSeparation ())
	gluPerspective (gameStates.render.glFOV * X2D (transformation.m_info.zoom), canvasAspectRatio, ZNEAR, ZFAR);
else if (VRActive ()) {
#if 0
	glLoadMatrixf ((GLfloat*) gameData.renderData.vr.m_eyes [StereoSeparation () > 0].Projection.M);
#elif 0
	SetupFrustum (2 * StereoSeparation ());
#else
	gluPerspective (gameStates.render.glFOV * X2D (transformation.m_info.zoom) / VR_DEFAULT_ZOOM, canvasAspectRatio, ZNEAR, ZFAR);
#endif
	}
else if (gameOpts->render.stereo.nMethod == STEREO_PARALLEL)
	SetupFrustum (StereoSeparation ());
else
	gluPerspective (gameStates.render.glFOV * X2D (transformation.m_info.zoom), canvasAspectRatio, ZNEAR, ZFAR);
ogl.ClearError (0);
if (gameStates.render.bRearView < 0)
	glScalef (-1.0f, 1.0f, 1.0f);
m_data.depthScale.v.coord.x = float (ZFAR / (ZFAR - ZNEAR));
m_data.depthScale.v.coord.y = float (ZNEAR * ZFAR / (ZNEAR - ZFAR));
m_data.depthScale.v.coord.z = float (ZFAR - ZNEAR);
#if 1
m_data.windowScale.dim.x = 1.0f / float (gameData.renderData.screen.Width ());
m_data.windowScale.dim.y = 1.0f / float (gameData.renderData.screen.Height ());
#else
m_data.windowScale.dim.x = 1.0f / float (CCanvas::Current ()->Width ());
m_data.windowScale.dim.y = 1.0f / float (CCanvas::Current ()->Height ());
#endif
glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
transformation.SetupProjection (aspectRatio);
glMatrixMode (GL_MODELVIEW);
}

//------------------------------------------------------------------------------

void COGL::SaveViewport (void)
{
m_states.viewport [1] = m_states.viewport [0];
}

//------------------------------------------------------------------------------

void COGL::RestoreViewport (void)
{
m_states.viewport [0] = m_states.viewport [1];
m_states.viewport [0].Apply ();
}

//------------------------------------------------------------------------------

void COGL::SetViewport (int32_t x, int32_t y, int32_t w, int32_t h)
{
if (!gameOpts->render.cameras.bHires) {
	x >>= gameStates.render.cameras.bActive;
	y >>= gameStates.render.cameras.bActive;
	w >>= gameStates.render.cameras.bActive;
	h >>= gameStates.render.cameras.bActive;
	}
int32_t t = gameData.renderData.screen.Height ();
if (!gameOpts->render.cameras.bHires)
	t >>= gameStates.render.cameras.bActive;
CViewport vp = CViewport (x, y, w, h, t);
if (m_states.viewport [0] != vp) {
	m_states.viewport [1] = m_states.viewport [0];
	m_states.viewport [0] = vp;
	m_states.viewport [0].Apply ();
	}
#if DBG
else {
	int32_t v [4];
	glGetIntegerv (GL_VIEWPORT, v);
	y = t - y - h;
	if ((x != v [0]) || (y != v [1]) || (w != v [2]) || (h != v [3])) {
		m_states.viewport [1] = m_states.viewport [0];
		m_states.viewport [0] = vp;
		m_states.viewport [0].Apply ();
		}
	}
#endif
}

//------------------------------------------------------------------------------

void COGL::GetViewport (vec4& viewport)
{
viewport [0] = (float) m_states.viewport [0].m_x / (float) gameData.renderData.screen.Width ();
viewport [1] = (float) m_states.viewport [0].m_y / (float) gameData.renderData.screen.Height ();
viewport [2] = (float) m_states.viewport [0].m_w / (float) gameData.renderData.screen.Width ();
viewport [3] = (float) m_states.viewport [0].m_h / (float) gameData.renderData.screen.Height ();
}

//------------------------------------------------------------------------------

void COGL::ColorMask (GLboolean bRed, GLboolean bGreen, GLboolean bBlue, GLboolean bAlpha, GLboolean bEyeOffset) 
{
if (!bEyeOffset || !ogl.IsAnaglyphDevice () || gameOpts->render.stereo.bEnhance || gameStates.render.nWindow [0])
	glColorMask (bRed, bGreen, bBlue, bAlpha);
else if (gameOpts->render.stereo.nGlasses == GLASSES_AMBER_BLUE) {	//colorcode 3-d (amber/blue)
	if ((m_data.xStereoSeparation <= 0) != gameOpts->render.stereo.bFlipFrames)
		glColorMask (bRed, bGreen, GL_FALSE, bAlpha);
	else
		glColorMask (GL_FALSE, GL_FALSE, bBlue, bAlpha);
	}
else if (gameOpts->render.stereo.nGlasses == GLASSES_RED_CYAN) {	//red/cyan
	if ((m_data.xStereoSeparation <= 0) != gameOpts->render.stereo.bFlipFrames)
		glColorMask (bRed, GL_FALSE, GL_FALSE, bAlpha);
	else
		glColorMask (GL_FALSE, bGreen, bBlue, bAlpha);
	}
else if (gameOpts->render.stereo.nGlasses == GLASSES_GREEN_MAGENTA) {	//blue/red
	if ((m_data.xStereoSeparation <= 0) != gameOpts->render.stereo.bFlipFrames)
		glColorMask (GL_FALSE, bGreen, GL_FALSE, bAlpha);
	else
		glColorMask (bRed, GL_FALSE, bBlue, bAlpha);
	}
else //GLASSES_SHUTTER_NVIDIA, GLASSES_VR or NONE
	glColorMask (bRed, bGreen, bBlue, bAlpha);
}

//------------------------------------------------------------------------------

#define GL_INFINITY	0

void COGL::StartFrame (int32_t bFlat, int32_t bResetColorBuf, fix xStereoSeparation)
{
SetStereoSeparation (xStereoSeparation);
m_states.bEnableScissor = 1;
if (!gameStates.render.nWindow [0])
	ChooseDrawBuffer ();
ogl.SetPolyOffsetFill (false);
#if !MAX_SHADOWMAPS
if (gameStates.render.nShadowPass) {

	SetDepthMode (GL_LESS);
	SetDepthWrite (true);
	if (gameStates.render.nShadowPass == 1) {	//render unlit/final scene
		if (!gameStates.render.nShadowMap) {
#if GL_INFINITY
			glMatrixMode (GL_PROJECTION);
			float	infProj [4][4];	//projection to infinity
			memset (infProj, 0, sizeof (infProj));
			infProj [1][1] = 1.0f / (float) tan (gameStates.render.glFOV);
			infProj [0][0] = infProj [1][1] / (float) gameStates.render.glAspect;
			infProj [3][2] = -0.02f;	// -2 * near
			infProj [2][2] =
			infProj [2][3] = -1.0f;
			glLoadMatrixf (reinterpret_cast<float*> (infProj));
#endif
			SetDepthTest (true);
			SetStencilTest (false);
			SetFaceCulling (true);
			OglCullFace (0);
			if (!FAST_SHADOWS)
				ColorMask (0,0,0,0,0);
			}
		}
	else if (gameStates.render.nShadowPass == 2) {	//render occluders / shadow maps
		if (gameStates.render.nShadowMap) {
			ColorMask (0,0,0,0,0);
			ogl.SetPolyOffsetFill (true);
			glPolygonOffset (1.0f, 2.0f);
			}
		else {
#	if DBG_SHADOWS
			if (bShadowTest) {
				ColorMask (1,1,1,1,0);
				SetDepthWrite (false);
				SetBlendMode (OGL_BLEND_ALPHA);
				SetStencilTest (false);
				}
			else
#	endif
			 {
				ColorMask (0,0,0,0,0);
				SetDepthWrite (false);
				glClearStencil (0);
				glClear (GL_STENCIL_BUFFER_BIT);
					bSingleStencil = 1;
#	if DBG_SHADOWS
				if (bShadowTest)
#	endif
					{
					glStencilMask (~0);
					glStencilFunc (GL_ALWAYS, 0, ~0);
					}
				}
			}
		}
	else if (gameStates.render.nShadowPass == 3) { //render final lit scene
		if (gameStates.render.nShadowMap) {
			ogl.SetPolyOffsetFill (false);
			SetDepthMode (GL_LESS);
			}
		else {
			if (gameStates.render.nShadowBlurPass == 2)
				SetStencilTest (false);
         else if (FAST_SHADOWS) {
				glStencilFunc (GL_NOTEQUAL, 0, ~0);
				glStencilOp (GL_REPLACE, GL_KEEP, GL_KEEP);
				}
			else
			 {
				glStencilFunc (GL_EQUAL, 0, ~0);
				glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);
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
else 
#endif
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
	if (gameStates.render.nRenderPass < 0) {
		ogl.SetDepthWrite (true);
		glClearDepth (/*gameStates.render.nWindow [0] ? 1.0 : 0.0*/ 1.0);
#if MAX_SHADOWMAPS > 0
		if (gameStates.render.nShadowMap) {
			ColorMask (0, 0, 0, 0, 0);
			glClear (GL_DEPTH_BUFFER_BIT);
			}
		else 
#endif
			{
			ColorMask (1, 1, 1, 1, 1);
			//SetViewport (0, 0, gameData.renderData.screen.Width (), gameData.renderData.screen.Height ());
			GLbitfield mask = 0;
			if (!bResetColorBuf)
				mask = GL_DEPTH_BUFFER_BIT;
			else /*if (automap.Active () || (gameStates.render.bRenderIndirect > 0))*/ {
				glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
				mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
				}
			if (mask) {
				gameData.renderData.frame.Activate ("StartFrame (frame)");
				glClear (mask);
				gameData.renderData.frame.Deactivate ();
				}
			}
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
	if (m_features.bAntiAliasing/*.Apply ()*/)
		glEnable (GL_MULTISAMPLE_ARB);
	if (bFlat) {
		SetDepthTest (false);
		SetAlphaTest (false);
		SetFaceCulling (false);
		}
	else {
		SetFaceCulling (true);
		glFrontFace (GL_CW);	//Weird, huh? Well, D2 renders everything reverse ...
#if MAX_SHADOWMAPS > 0
		if (gameStates.render.nShadowMap) {
#	if 1
			ogl.SetPolyOffsetFill (true);
			glPolygonOffset (1.0f, 2.0f);
#	endif
			SetCullMode ((gameStates.render.bRearView < 0) ? GL_BACK : GL_FRONT);
			}
		else
#endif
			SetCullMode ((gameStates.render.bRearView < 0) ? GL_FRONT : GL_BACK);
		SetDepthTest (true);
		SetDepthMode (GL_LESS);
		SetAlphaTest (true);
		glAlphaFunc (GL_GEQUAL, (float) 0.005);
		}
	SetBlending (true);
	SetBlendMode (OGL_BLEND_ALPHA);
	SetStencilTest (false);
	CCanvas::Current ()->SetViewport ();
	}
}

//------------------------------------------------------------------------------

void COGL::EndFrame (int32_t nWindow)
{
//SetViewport (0, 0, gameData.renderData.screen.Width (), gameData.renderData.screen.Height ());
#if 1
if ((nWindow == 0) && (ogl.StereoSeparation () <= 0)) 
	postProcessManager.Update ();
#endif

CCanvas::Current ()->Deactivate ();
if ((nWindow >= 0) && !(gameStates.render.cameras.bActive || gameStates.render.bBriefing)) {
	if (gameStates.render.bRenderIndirect > 0)
		SelectDrawBuffer (0);
	SetDrawBuffer (GL_BACK, gameStates.render.bRenderIndirect > 0);
	}
if (m_features.bShaders)
	shaderManager.Deploy (-1);
ogl.SetTexturing (true);
DisableClientStates (1, 1, 1, GL_TEXTURE3);
ogl.BindTexture (0);
DisableClientStates (1, 1, 1, GL_TEXTURE2);
ogl.BindTexture (0);
DisableClientStates (1, 1, 1, GL_TEXTURE1);
ogl.BindTexture (0);
DisableClientStates (1, 1, 1, GL_TEXTURE0);
ogl.BindTexture (0);
SetBlendMode (OGL_BLEND_ALPHA);
glMatrixMode (GL_PROJECTION);
glLoadIdentity ();//clear matrix
glOrtho (0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
glMatrixMode (GL_MODELVIEW);
glLoadIdentity ();//clear matrix
//SetScissorTest (false);
SetAlphaTest (false);
SetDepthTest (false);
SetFaceCulling (false);
SetStencilTest (false);
if (SHOW_DYN_LIGHT) {
	SetLighting (false);
	glDisable (GL_COLOR_MATERIAL);
	}
ogl.SetDepthWrite (true);
ColorMask (1,1,1,1,0);
if (m_features.bAntiAliasing/*.Apply ()*/)
	glDisable (GL_MULTISAMPLE_ARB);
}

//------------------------------------------------------------------------------

void COGL::EnableLighting (int32_t bSpecular)
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

void COGL::SetupTransform (int32_t bForce)
{
if (!m_states.nTransformCalls && (m_states.bUseTransform || bForce)) {
	glMatrixMode (GL_MODELVIEW);
	glPushMatrix ();
	glLoadIdentity ();
	glScalef (1, 1, -1);
	glMultMatrixf (reinterpret_cast<GLfloat*> (transformation.m_info.viewf [2].m.vec));
	glTranslatef (-transformation.m_info.posf [1].v.coord.x, -transformation.m_info.posf [1].v.coord.y, -transformation.m_info.posf [1].v.coord.z);
	++m_states.nTransformCalls;
	}
}

// -----------------------------------------------------------------------------------

void COGL::ResetTransform (int32_t bForce)
{
if ((m_states.nTransformCalls > 0) && (m_states.bUseTransform || bForce) && !--m_states.nTransformCalls) {
	glMatrixMode (GL_MODELVIEW);
	glPopMatrix ();
	}
}

//------------------------------------------------------------------------------

void COGL::DrawArrays (GLenum mode, GLint first, GLsizei count)
{
#if 0 //DBG
if (count < 1)
	PrintLog (0, "glDrawArrays: invalid count\n");
else if (count > 100000)
	PrintLog (0, "glDrawArrays: suspiciously high count\n");
else if (mode > GL_POLYGON)
	PrintLog (0, "glDrawArrays: invalid mode\n");
#if TRACK_STATES || DBG_OGL
else if (m_data.bUseTextures [m_data.nTMU [0]] && !m_data.clientStates [m_data.nTMU [0]][0])
	PrintLog (0, "glDrawArrays: client data not enabled\n");
#endif
else
#endif
#if 0 //DBG_OGL
if (!m_data.clientBuffers [/*m_data.nTMU [0]*/0][0].buffer)
	PrintLog (0, "glDrawArrays: client data not enabled\n");
else
#endif
	glDrawArrays (mode, first, count);
}

//------------------------------------------------------------------------------

void COGL::RebuildContext (int32_t bGame)
{
m_states.bRebuilding = 1;
cameraManager.Destroy ();
m_data.Initialize ();
SetupExtensions ();
backgroundManager.Rebuild ();
#if 0
if (!gameStates.app.bGameRunning)
	messageBox.Show (TXT_PREPARE_FOR_DESCENT);
#endif
ResetClientStates ();
ResetTextures (1, bGame);
if (bGame) {
	InitShaders ();
	ClearError (0);
	gameData.modelData.Destroy ();
	gameData.modelData.Prepare ();
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
else {
	shaderManager.Destroy (true);
	ogl.InitEnhanced3DShader ();
	}
//gameData.modelData.Prepare ();
if (!gameStates.app.bGameRunning)
	messageBox.Clear ();
SetDrawBuffer (m_states.nDrawBuffer, 1);
m_states.bRebuilding = 0;
}

//------------------------------------------------------------------------------

void COGL::SetScreenMode (void)
{
if (/*(gameStates.video.nLastScreenMode == gameStates.video.nScreenMode) &&*/
	 (m_states.bLastFullScreen == m_states.bFullScreen) &&
	 (gameStates.app.bGameRunning || (gameStates.video.nScreenMode == SCREEN_GAME) /*|| (m_states.nDrawBuffer == GL_FRONT)*/))
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
		SetBlendMode (OGL_BLEND_ALPHA);
		SetTexturing (false);
		SetDepthMode (GL_ALWAYS); //LEQUAL);
		SetDepthTest (false);
		}
	}
if (gameStates.app.bInitialized)
	RebuildContext (gameStates.video.nScreenMode == SCREEN_GAME);
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

int32_t COGL::StencilOff (void)
{
if (!(ogl.m_data.bStencilTest && SHOW_SHADOWS && (gameStates.render.nShadowPass == 3)))
	return 0;
SetStencilTest (false);
m_states.nStencil--;
return 1;
}

//------------------------------------------------------------------------------

void COGL::StencilOn (int32_t bStencil)
{
if (bStencil && !ogl.m_data.bStencilTest) {
	ogl.SetStencilTest (true);
	m_states.nStencil++;
	}
}

//------------------------------------------------------------------------------

GLenum COGL::ClearError (int32_t bTrapError) 
{
GLenum nError = glGetError ();
#if DBG_OGL
if (nError) {
	const char* pszError = reinterpret_cast<const char*> (gluErrorString (nError));
	PrintLog (0, "%s\n", pszError);
	if (bTrapError)
		BRP;
	}
#endif
return nError;
}

//------------------------------------------------------------------------------

void COGL::SwapInterval (int32_t nInterval)
{
#ifdef WIN32
	wglSwapIntervalEXT (nInterval);
#else
	glXSwapIntervalEXT (glXGetCurrentDisplay (), glXGetCurrentDrawable (), nInterval);
#endif
}

//------------------------------------------------------------------------------
//eof
