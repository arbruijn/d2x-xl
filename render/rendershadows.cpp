#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "descent.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "segmath.h"
#include "shadows.h"
#include "objrender.h"
#include "rendermine.h"
#include "rendershadows.h"
#include "visibility.h"
#include "glow.h"

//------------------------------------------------------------------------------

void RenderFaceShadow (tFaceProps *propsP)
{
	int				i, nVertices = propsP->nVertices;
	CRenderPoint*	p;
	CFloatVector	v [9];

for (i = 0; i < nVertices; i++) {
	p = gameData.segs.points + propsP->vp [i];
	if (p->Index () < 0)
		v [i].Assign (p->ViewPos ());
	else
		memcpy (v + i, gameData.render.vertP + p->Index (), sizeof (CFloatVector));
	}
v [nVertices] = v [0];
ogl.EnableClientState (GL_VERTEX_ARRAY);
OglVertexPointer (3, GL_FLOAT, 0, v);
#if DBG_SHADOWS
if (bShadowTest) {
	if (bFrontCap)
		OglDrawArrays (GL_LINE_LOOP, 0, nVertices);
	}
else
#endif
OglDrawArrays (GL_TRIANGLE_FAN, 0, nVertices);
#if DBG_SHADOWS
if (!bShadowTest || bShadowVolume)
#endif
for (i = 0; i < nVertices; i++)
	RenderShadowVolumeFace (v + i);
ogl.DisableClientState (GL_VERTEX_ARRAY);
#if DBG_SHADOWS
if (!bShadowTest || bRearCap)
#endif
RenderFarShadowCapFace (v, nVertices);
}

//------------------------------------------------------------------------------

void RenderShadowQuad (void)
{
	static GLfloat shadowHue [2][4] = {{0.6f, 0.6f, 0.6f, 1.0f}, {0.25f, 0.25f, 0.25f, 1.0f}};

glMatrixMode (GL_MODELVIEW);
glPushMatrix ();
glLoadIdentity ();
glMatrixMode (GL_PROJECTION);
glPushMatrix ();
glLoadIdentity ();
glOrtho (0, 1, 1, 0, 0, 1);
ogl.SetTexturing (false);
ogl.SetDepthTest (false);
ogl.SetStencilTest (true);
ogl.SetDepthWrite (false);
if (gameStates.render.nShadowBlurPass)
	ogl.SetBlending (false);
else
	ogl.SetBlendMode (OGL_BLEND_MULTIPLY); 
glColor4fv (shadowHue [gameStates.render.nShadowBlurPass]); 
glBegin (GL_QUADS);
glVertex2f (0,0);
glVertex2f (1,0);
glVertex2f (1,1);
glVertex2f (0,1);
glEnd ();
if (gameStates.render.nShadowBlurPass)
	ogl.SetBlending (true);
else
	ogl.SetBlendMode (OGL_BLEND_ALPHA); //GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
ogl.SetDepthTest (true);
ogl.SetStencilTest (false);
ogl.SetDepthWrite (true);
glPopMatrix ();
glMatrixMode (GL_MODELVIEW);
glPopMatrix ();
}

//------------------------------------------------------------------------------

void RenderFastShadows (fix xStereoSeparation, int nWindow, short nStartSeg)
{
glowRenderer.End ();
gameStates.render.nShadowPass = 2;
ogl.StartFrame (0, 0, xStereoSeparation);
gameData.render.shadows.nFrame = !gameData.render.shadows.nFrame;
RenderMine (nStartSeg, xStereoSeparation, nWindow);

gameStates.render.nShadowPass = 3;
ogl.StartFrame (0, 0, xStereoSeparation);
if (glowRenderer.Available (BLUR_SHADOW)) {
	gameStates.render.nShadowBlurPass = 1;
	glowRenderer.Begin (BLUR_SHADOW, 2 /*gameOpts->render.ShadowQuality ()*/, true, 1.0f);
	}
RenderShadowQuad ();
if (gameStates.render.nShadowBlurPass) {
	glowRenderer.End ();
	gameStates.render.nShadowBlurPass = 0;
	}
}

//------------------------------------------------------------------------------

void RenderNeatShadows (fix xStereoSeparation, int nWindow, short nStartSeg)
{
	short			i, n;
	CDynLight*	lightP;

gameData.render.shadows.nLights = GatherShadowLightSources ();
n = lightManager.LightCount (1);
for (i = 0; i < n; i++) {
	lightP = lightManager.RenderLights (i);
	if (!lightP->render.bShadow)
		continue;
	gameData.render.shadows.lightP = lightP;
	lightP->render.bExclusive = 1;
#if 1
	gameStates.render.nShadowPass = 2;
	ogl.StartFrame (0, 0, xStereoSeparation);
	memcpy (&gameData.render.shadows.vLightPos, lightP->render.vPosf + 1, sizeof (CFloatVector));
	gameData.render.shadows.nFrame = !gameData.render.shadows.nFrame;
	RenderMine (nStartSeg, xStereoSeparation, nWindow);
#endif
	gameStates.render.nShadowPass = 3;
	ogl.StartFrame (0, 0, xStereoSeparation);
	gameData.render.shadows.nFrame = !gameData.render.shadows.nFrame;
	RenderMine (nStartSeg, xStereoSeparation, nWindow);
	lightP->render.bExclusive = 0;
	}
#if 0
gameStates.render.nShadowPass = 4;
RenderMine (nStartSeg, xStereoSeparation, nWindow);
#endif
}

//------------------------------------------------------------------------------

#define STB_SIZE_X	2048
#define STB_SIZE_Y	2048

CBitmap	shadowBuf;
ubyte			shadowTexBuf [STB_SIZE_X * STB_SIZE_Y * 4];
static int	bHaveShadowBuf = 0;

void CreateShadowTexture (void)
{
	GLint	i;

if (!bHaveShadowBuf) {
	memset (&shadowBuf, 0, sizeof (shadowBuf));
	shadowBuf.SetWidth (STB_SIZE_X);
	shadowBuf.SetHeight (STB_SIZE_Y);
	shadowBuf.SetFlags ((char) BM_FLAG_TGA);
	shadowBuf.SetBuffer (shadowTexBuf);
	shadowBuf.SetTranspType (-1);
	shadowBuf.PrepareTexture (0, 0, NULL);
	bHaveShadowBuf = 1;
	}
#if 1
glCopyTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 0, 0,
						CCanvas::Current ()->Width (), 
						CCanvas::Current ()->Height (), 0);
#else
glCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 0, 0, 128, 128);
#endif
i = glGetError ();
}

//------------------------------------------------------------------------------

int shadowProg = -1;

#if DBG_SHADERS

const char *pszShadowFS = "shadows.frag";
const char *pszShadowVS = "shadows.vert";

#else

const char *pszShadowFS = "shadows.frag";
const char *pszShadowVS = "shadows.vert";

#endif

void RenderShadowTexture (void)
{
if ((shadowProg < 0) && !shaderManager.Build (shadowProg, pszShadowFS, pszShadowVS))
	return;
glMatrixMode (GL_MODELVIEW);
glPushMatrix ();
glLoadIdentity ();
glMatrixMode (GL_PROJECTION);
glPushMatrix ();
glLoadIdentity ();
glOrtho (0, 1, 1, 0, 0, 1);
ogl.SetDepthTest (false);
ogl.SetDepthWrite (false);
#if 1
ogl.SetBlending (false);
#else
ogl.SetBlending (true);
SetBlendMode (OGL_BLEND_ALPHA);
#endif
ogl.SetTexturing (true);
ogl.SelectTMU (GL_TEXTURE0);
shadowBuf.SetTranspType (0);
if (shadowBuf.Bind (0))
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
if (ogl.m_features.bShaders)
	shaderManager.Deploy (-1);
ogl.SetDepthTest (true);
ogl.SetDepthWrite (true);
glPopMatrix ();
glMatrixMode (GL_MODELVIEW);
glPopMatrix ();
}

//------------------------------------------------------------------------------

#if 0

int RenderShadowMap (CDynLight *pLight)
{
	CCamera	*cameraP;

if (pLight->shadow.nFrame == gameData.render.shadows.nFrame)
	return 0;
if (gameData.render.shadows.nShadowMaps == MAX_SHADOW_MAPS)
	return 0;
pLight->shadow.nFrame = !pLight->shadow.nFrame;
gameStates.render.nShadowPass = 2;
cameraP = cameraManager.ShadowMap (gameData.render.shadows.nShadowMaps);
//cameraP->Create (gameData.render.shadows.nShadowMaps++, pLight->info.nSegment, 
//					  pLight->info.nSide, pLight->info.nSegment, pLight->info.nSide, NULL, 1, 0);
cameraP->Render ();
gameStates.render.nShadowPass = 2;
return 1;
}

#endif

//------------------------------------------------------------------------------
//The following code is an attempt to find all objects that cast a shadow visible
//to the player. To accomplish that, for each robot the line of sight to each
//CSegment visible to the player is computed. If there is a los to any of these 
//segments, the CObject's shadow is rendered. Far from perfect solution though. :P

void RenderObjectShadows (void)
{
	CObject		*objP;
	//int			i; 
	int			j, bSee;
	CObject		fakePlayerPos = *gameData.objs.viewerP;

FORALL_ACTOR_OBJS (objP, i)
	if (objP == gameData.objs.consoleP)
		RenderObject (objP, 0, 0);
	else if ((objP->info.nType == OBJ_PLAYER) || 
				(gameOpts->render.shadows.bRobots && (objP->info.nType == OBJ_ROBOT))) {
		for (j = gameData.render.mine.nRenderSegs [0]; j--;) {
			fakePlayerPos.info.nSegment = gameData.render.mine.visibility [0].segments [j];
			fakePlayerPos.info.position.vPos = SEGMENTS [fakePlayerPos.info.nSegment].Center ();
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
#if MAX_SHADOWMAPS
for (; gameData.render.shadows.nShadowMaps;)
	cameraManager.DestroyShadowMap (--gameData.render.shadows.nShadowMaps);
#endif
}

//------------------------------------------------------------------------------

#if MAX_SHADOWMAPS

void ApplyShadowMaps (short nStartSeg, fix xStereoSeparation, int nWindow)
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

	CFloatMatrix mProjection;
	CFloatMatrix mModelView;

	int			i;
	CCamera		*cameraP;

#if 1
ogl.SelectTMU (GL_TEXTURE0);
ogl.SetTexturing (true); 

glEnable (GL_TEXTURE_GEN_S);
glEnable (GL_TEXTURE_GEN_T);
glEnable (GL_TEXTURE_GEN_R);
glEnable (GL_TEXTURE_GEN_Q);

for (i = 0; i < 4; i++)
	glTexGeni (nTexCoord [i], GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
for (i = 0; i < 4; i++)
	glTexGenfv (nTexCoord [i], GL_EYE_PLANE, mPlanef + 4 * i);

glGetFloatv (GL_PROJECTION_MATRIX, mProjection.m.vec);
glMatrixMode (GL_TEXTURE);
for (i = 0; i < lightManager.LightCount (2); i++) {
	cameraP = cameraManager.ShadowMap (0);
	ogl.BindTexture (cameraP->FrameBuffer ().DepthBuffer ());
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
	glLoadMatrixf (mTexBiasf);
	glMultMatrixf (mProjection.m.vec);
	CFixMatrix::Transpose (mModelView, cameraP->GetObject ()->info.position.mOrient);
	glMultMatrixf (mModelView.m.vec);
	}
glMatrixMode (GL_MODELVIEW);
#endif
RenderMine (nStartSeg, xStereoSeparation, nWindow);
#if 1
glMatrixMode (GL_TEXTURE);
glLoadIdentity ();
glMatrixMode (GL_MODELVIEW);
glDisable (GL_TEXTURE_GEN_S);
glDisable (GL_TEXTURE_GEN_T);
glDisable (GL_TEXTURE_GEN_R);
glDisable (GL_TEXTURE_GEN_Q);
ogl.SelectTMU (GL_TEXTURE0);	
ogl.SetTexturing (false);
#endif
}

#endif

//------------------------------------------------------------------------------

int GatherShadowLightSources (void)
{
	CObject*			objP;
	CDynLight*		lightP;
	CFixVector		vLightDir;
	short*			nearestLightP;
	int				h, i, j, k, l, n, m = gameOpts->render.shadows.nLights;

n = lightManager.LightCount (1);
for (h = l = 0; l < n; l++) {
	lightP = lightManager.RenderLights (h);
	lightP->render.bShadow =
	lightP->render.bExclusive = 0;
	}

FORALL_OBJS (objP, h) {
	h = objP->Index ();
	if (gameData.render.mine.bObjectRendered [h] != gameStates.render.nFrameFlipFlop)
		continue;
	nearestLightP = lightManager.NearestSegLights () + objP->info.nSegment * MAX_NEAREST_LIGHTS;
	k = h * MAX_SHADOW_LIGHTS;
	for (i = n = 0; (n < m) && (*nearestLightP >= 0); i++, nearestLightP++) {
		lightP = lightManager.RenderLights (*nearestLightP);
		if (lightP->info.nObject >= 0) // only use moving dynamic lights
			continue;
		if (!lightP->render.bState)
			continue;
		if (!CanSeePoint (objP, &objP->info.position.vPos, &lightP->info.vPos, objP->info.nSegment))
			continue;
		vLightDir = objP->info.position.vPos - lightP->info.vPos;
		CFixVector::Normalize (vLightDir);
		if (n) {
			for (j = 0; j < n; j++)
				if (abs (CFixVector::Dot (vLightDir, gameData.render.shadows.vLightDir[j])) > I2X (2) / 3) // 60 deg
					break;
			if (j < n)
				continue;
			}
		gameData.render.shadows.vLightDir [n] = vLightDir;
		gameData.render.shadows.objLights [k + n++] = *nearestLightP;
		lightP->render.bShadow = 1;
		}
	gameData.render.shadows.objLights [k + n] = -1;
	}
n = lightManager.LightCount (1);
for (h = i = 0; i < h; i++)
	if (lightManager.RenderLights (i)->render.bShadow)
		h++;
return h;
}

//------------------------------------------------------------------------------
// eof
