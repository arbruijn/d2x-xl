// Copyright (c) Dietfrid Mali

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "descent.h"
#include "error.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "segmath.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "renderlib.h"
#include "network.h"
#include "glare.h"
#include "automap.h"
#include "addon_bitmaps.h"

#define CORONA_OUTLINE	0
#if DBG
#	define SHADER_SOFT_CORONAS 1
#else
#	define SHADER_SOFT_CORONAS 1
#endif

float coronaIntensities [] = {0.35f, 0.5f, 0.75f, 1};

int hGlareShader = -1;

CGlareRenderer glareRenderer;

// -----------------------------------------------------------------------------------

int CGlareRenderer::Style (void)
{
return (ogl.m_features.bDepthBlending > 0) && gameOpts->render.coronas.bUse && gameOpts->render.coronas.nStyle && !gameStates.render.cameras.bActive;
}

// -----------------------------------------------------------------------------------

int CGlareRenderer::CalcFaceDimensions (short nSegment, short nSide, fix *w, fix *h, ushort* corners)
{
	fix		d1, d2, dMax = -1;
	int		i, j;

if (!corners) 
	corners = SEGMENTS [nSegment].Corners (nSide);
m_nVertices = SEGMENTS [nSegment].Side (nSide)->CornerCount ();
for (i = j = 0; j < m_nVertices; j++) {
	fix d = CFixVector::Dist (gameData.segs.vertices [corners [j]], gameData.segs.vertices [corners [(j + 1) % m_nVertices]]);
	if (dMax < d) {
		dMax = d;
		i = j;
		}
	}
if (w)
	*w = dMax;
if (i > 2)
	i--;
j = i + 1;
d1 = VmLinePointDist (gameData.segs.vertices [corners [i]],
                      gameData.segs.vertices [corners [j]],
                      gameData.segs.vertices [corners [(j + 1) % m_nVertices]]);
if (m_nVertices == 4)
	d2 = VmLinePointDist (gameData.segs.vertices [corners [i]],
								 gameData.segs.vertices [corners [j]],
								 gameData.segs.vertices [corners [(j + 2) % m_nVertices]]);
if (h)
	*h = (m_nVertices < 3) ? d1 : (d1 > d2) ? d1 : d2;
return i;
}

// -----------------------------------------------------------------------------------

int CGlareRenderer::FaceHasCorona (short nSegment, short nSide, int *bAdditiveP, float *fIntensityP)
{
	CSide			*sideP;
	char			*pszName;
	int			i, bAdditive, nTexture, nBrightness;

if (IsMultiGame && extraGameInfo [1].bDarkness)
	return 0;
#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
sideP = SEGMENTS [nSegment].m_sides + nSide;
CWall* wallP = sideP->Wall ();
if (wallP) {
	ubyte nType = wallP->nType;

	if ((nType == WALL_BLASTABLE) || (nType == WALL_DOOR) || (nType == WALL_OPEN) || (nType == WALL_CLOAKED))
		return 0;
	if (wallP->flags & (WALL_BLASTED | WALL_ILLUSION_OFF))
		return 0;
	}
// get and check the corona emitting texture
nBrightness = (nTexture = sideP->m_nOvlTex) ? IsLight (nTexture) : 0;
if (nBrightness >= I2X (1) / 8) {
	bAdditive = gameOpts->render.coronas.bAdditive;
	}
else if ((nBrightness = IsLight (nTexture = sideP->m_nBaseTex))) {
	if (fIntensityP)
		*fIntensityP /= 2;
	bAdditive = gameOpts->render.coronas.bAdditive;
	//bAdditive = 0;
	}
else
	return 0;
//if (gameStates.render.nLightingMethod) 
	{
	i = lightManager.Find (nSegment, nSide, -1);
	if ((i < 0) || !lightManager.Lights ()[i].info.bOn)
		return 0;
	}
#if 0
if (fIntensityP && (nBrightness < I2X (1)))
	*fIntensityP *= X2F (nBrightness);
#endif
if (gameStates.app.bD1Mission) {
	switch (nTexture) {
		case 289:	//empty light
		case 328:	//energy sparks
		case 334:	//reactor
		case 335:
		case 336:
		case 337:
		case 338:	//robot generators
		case 339:
			return 0;
		default:
			break;
		}
	}
else {
	switch (nTexture) {
		case 302:	//empty light
		case 348:	//sliding walls
		case 349:
		case 353:	//energy sparks
		case 356:	//reactor
		case 357:
		case 358:
		case 359:
		case 360:	//robot generators
		case 361:
		case
0:	//force field
		case
6:	//teleport
		case 432:	//force field
		case 433:	//goals
		case 434:
			return 0;
		case 404:
		case 405:
		case 406:
		case 407:
		case 408:
		case 409:
			if (fIntensityP)
				*fIntensityP *= 2;
			break;
		default:
			break;
		}
	}
pszName = gameData.pig.tex.bitmapFiles [gameStates.app.bD1Mission][gameData.pig.tex.bmIndexP [nTexture].index].name;
if (strstr (pszName, "metl") || strstr (pszName, "rock") || strstr (pszName, "water"))
	return 0;
if (bAdditiveP)
	*bAdditiveP = bAdditive;
return nTexture;
}

// -----------------------------------------------------------------------------------

float CGlareRenderer::ComputeCoronaSprite (short nSegment, short nSide)
{
	CSide*			sideP = SEGMENTS [nSegment].m_sides + nSide;
	ushort*			corners;
	int				i;
	float				fLight = 0;
	CFloatVector	v;

corners = SEGMENTS [nSegment].Corners (nSide);
m_nVertices = SEGMENTS [nSegment].Side (nSide)->CornerCount ();
for (i = 0; i < m_nVertices; i++) {
	fLight += X2F (sideP->m_uvls [i].l);
	transformation.Transform (m_sprite [i], gameData.segs.fVertices [corners [i]], 0);
	}
v.Assign (SEGMENTS [nSegment].SideCenter (nSide));
transformation.Transform (m_vCenter, v, 0);
return fLight;
}

// -----------------------------------------------------------------------------------

void CGlareRenderer::ComputeSpriteZRange (void)
{
m_zRange.fMin = 1000000000.0f;
m_zRange.fMax = -1000000000.0f;
for (int i = 0; i < 4; i++) {
	float z = m_sprite [i].v.coord.z;
	if (m_zRange.fMin > z)
		m_zRange.fMin = z;
	if (m_zRange.fMax < z)
		m_zRange.fMax = z;
	}
m_zRange.fSize = m_zRange.fMax - m_zRange.fMin + 1;
m_zRange.fRad = m_zRange.fSize / 2;
}

// -----------------------------------------------------------------------------------

float CGlareRenderer::MoveSpriteIn (float fIntensity)
{
ComputeSpriteZRange ();
if (m_zRange.fMin > 0)
	m_vCenter = m_vCenter * (m_zRange.fMin / m_vCenter.v.coord.z);
else {
	if (m_zRange.fMin < -m_zRange.fRad)
		return 0;
	fIntensity *= 1 + m_zRange.fMin / m_zRange.fRad * 2;
	m_vCenter.v.coord.x /= m_vCenter.v.coord.z;
	m_vCenter.v.coord.y /= m_vCenter.v.coord.y;
	m_vCenter.v.coord.z = 1;
	}
return fIntensity;
}

// -----------------------------------------------------------------------------------

#if DBG && CORONA_OUTLINE

void RenderCoronaOutline(CFloatVector *m_sprite, CFloatVector m_vCenter)
{
	int	i;

ogl.SetTexturing (false);
glColor4d (1,1,1,1);
glLineWidth (2);
glBegin (GL_LINE_LOOP);
for (i = 0; i < 4; i++)
	glVertex3fv (reinterpret_cast<GLfloat*> (m_sprite + i));
glEnd ();
glBegin (GL_LINES);
m_vCenter->x () += 5;
glVertex3fv (reinterpret_cast<GLfloat*> (&m_vCenter));
m_vCenter->x () -= 10;
glVertex3fv (reinterpret_cast<GLfloat*> (&m_vCenter));
m_vCenter->x () += 5;
m_vCenter->y () += 5;
glVertex3fv (reinterpret_cast<GLfloat*> (&m_vCenter));
m_vCenter->y () -= 10;
glVertex3fv (reinterpret_cast<GLfloat*> (&m_vCenter));
glEnd ();
glLineWidth (1);
}

#else

#	define RenderCoronaOutline(_sprite,_center)

#endif

// -----------------------------------------------------------------------------------

float CGlareRenderer::ComputeSoftGlare (void)
{
	CFloatVector 	n, e, s, t, u, v;
	float 			ul, vl, h, cosine;
	int 				i;

m_vEye = CFloatVector::ZERO;
u = m_sprite [2] + m_sprite [1];
u -= m_sprite [0];
u -= m_sprite [3];
u = u * 0.25f;
v = m_sprite [0] + m_sprite [1];
v -= m_sprite [2];
v -= m_sprite [3];
v = v * 0.25f;
e = m_vEye - m_vCenter;
CFloatVector::Normalize (e);
n = CFloatVector::Cross (v, u);
CFloatVector::Normalize (n);
ul = u.Mag ();
vl = v.Mag ();
h = (ul > vl) ? vl : ul;
s = u + n * (-h * CFloatVector::Dot (e, u) / ul);
t = v + n * (-h * CFloatVector::Dot (e, v) / vl);
s = s + e * CFloatVector::Dot (e, s);
t = t + e * CFloatVector::Dot (e, t);
s = s * 1.8f;
t = t * 1.8f;
v = m_vCenter;
for (i = 0; i < 3; i++) {
	m_sprite [0].v.vec [i] = v.v.vec [i] + s.v.vec [i] + t.v.vec [i];
	m_sprite [1].v.vec [i] = v.v.vec [i] + s.v.vec [i] - t.v.vec [i];
	m_sprite [2].v.vec [i] = v.v.vec [i] - s.v.vec [i] - t.v.vec [i];
	m_sprite [3].v.vec [i] = v.v.vec [i] - s.v.vec [i] + t.v.vec [i];
	}
cosine = CFloatVector::Dot (e, n);
return float (sqrt (cosine) * coronaIntensities [gameOpts->render.coronas.nIntensity]);
}

// -----------------------------------------------------------------------------------

void CGlareRenderer::RenderSoftGlare (int nTexture, float fIntensity, int bAdditive, int bColored)
{
	CFloatVector color;
	tTexCoord2f	tcGlare [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};
	CBitmap*		bmP = NULL;

if (!(bmP = (bAdditive ? glare.Bitmap () : corona.Bitmap ())))
	return;
if (gameStates.render.bAmbientColor)
	color = gameData.render.color.textures [nTexture];
else
	color.Red () = color.Green () = color.Blue () = X2F (IsLight (nTexture)) / 2;
if (!bColored)
	color.Red () = color.Green () = color.Blue () = (color.Red () + color.Green () + color.Blue ()) / 4;
if (bAdditive)
	glColor4f (fIntensity * color.Red (), fIntensity * color.Green (), fIntensity * color.Blue (), 1);
else
	glColor4f (color.Red (), color.Green (), color.Blue (), fIntensity);
bmP->Bind (1);
OglTexCoordPointer (2, GL_FLOAT, 0, tcGlare);
OglVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), m_sprite);
if (m_nVertices == 3)
	OglDrawArrays (GL_TRIANGLES, 0, 3);
else
	OglDrawArrays (GL_QUADS, 0, 4);
RenderCoronaOutline (m_sprite, m_vCenter);
}

// -----------------------------------------------------------------------------------

void CGlareRenderer::Render (short nSegment, short nSide, float fIntensity, float fSize)
{
if (!Style ())
	return;
if (fIntensity < 0.01f)
	return;

	int				nTexture, bAdditive;

#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif

if (!(nTexture = FaceHasCorona (nSegment, nSide, &bAdditive, &fIntensity)))
	return;
ComputeCoronaSprite (nSegment, nSide);
fIntensity *= ComputeSoftGlare ();
//shaderManager.Set ("dMax"), 20.0f);
RenderSoftGlare (nTexture, fIntensity, bAdditive, !automap.Display () || automap.m_visited [nSegment] || !gameOpts->render.automap.bGrayOut);
#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
}

//------------------------------------------------------------------------------

float CGlareRenderer::Visibility (int nQuery)
{
	GLuint	nSamples = 0;
	GLint		bAvailable = 0;
	int		nAttempts = 2;
	float		fIntensity;
#if DBG
	GLint		nError;
#endif

if (! (ogl.m_features.bOcclusionQuery && nQuery) || (Style () != 1))
	return 1;
if (!(gameStates.render.bQueryCoronas || gameData.render.lights.coronaSamples [nQuery - 1]))
	return 0;
for (;;) {
	glGetQueryObjectiv (gameData.render.lights.coronaQueries [nQuery - 1], GL_QUERY_RESULT_AVAILABLE_ARB, &bAvailable);
	if (glGetError ()) {
#if DBG
		glGetQueryObjectiv (gameData.render.lights.coronaQueries [nQuery - 1], GL_QUERY_RESULT_AVAILABLE_ARB, &bAvailable);
		if ((nError = glGetError ()))
#endif
			return 0;
		}
	if (bAvailable)
		break;
	if (!--nAttempts)
		return 0;
	G3_SLEEP (1);
	};
glGetQueryObjectuiv (gameData.render.lights.coronaQueries [nQuery - 1], GL_QUERY_RESULT_ARB, &nSamples);
if (glGetError ())
	return 0;
if (gameStates.render.bQueryCoronas == 1) {
#if DBG
	if (!nSamples) {
		GLint nBits;
		glGetQueryiv (GL_SAMPLES_PASSED, GL_QUERY_COUNTER_BITS, &nBits);
		glGetQueryObjectuiv (gameData.render.lights.coronaQueries [nQuery - 1], GL_QUERY_RESULT_ARB, &nSamples);
		}
#endif
	return (float) (gameData.render.lights.coronaSamples [nQuery - 1] = nSamples);
	}
fIntensity = (float) nSamples / (float) gameData.render.lights.coronaSamples [nQuery - 1];
#if DBG
if (fIntensity > 1)
	fIntensity = 1;
#endif
return (fIntensity > 1) ? 1 : (float) sqrt (fIntensity);
}

//-------------------------------------------------------------------------

bool CGlareRenderer::ShaderActive (void)
{
return (hGlareShader >= 0) && (shaderManager.Current () == hGlareShader);
}

//-------------------------------------------------------------------------

bool CGlareRenderer::LoadShader (float dMax, int nBlendMode)
{
	static float dMaxPrev = -1;
	static int nBlendPrev = -1;

ogl.ClearError (0);
if (!gameOpts->render.bUseShaders)
	return false;
if (ogl.m_features.bDepthBlending < 0)
	return false;
if (!ogl.CopyDepthTexture (0))
	return false;
if (dMax < 1)
	dMax = 1;
m_shaderProg = GLhandleARB (shaderManager.Deploy (hGlareShader, true));
if (!m_shaderProg)
	return false;
if (shaderManager.Rebuild (m_shaderProg)) {
	shaderManager.Set ("glareTex", 0);
	shaderManager.Set ("depthTex", 1);
	shaderManager.Set ("windowScale", ogl.m_data.windowScale.vec);
	shaderManager.Set ("dMax", dMax);
	shaderManager.Set ("blendMode", nBlendMode);
	}
else {
	if (dMaxPrev != dMax)
		shaderManager.Set ("dMax", dMax);
	if (nBlendPrev != nBlendMode)
		shaderManager.Set ("blendMode", nBlendMode);
	}
dMaxPrev = dMax;
nBlendPrev = nBlendMode;
ogl.SetDepthTest (false);
ogl.SelectTMU (GL_TEXTURE0);
return true;
}

//-------------------------------------------------------------------------

void CGlareRenderer::UnloadShader (void)
{
if (ogl.m_features.bDepthBlending > 0) {
	shaderManager.Deploy (-1);
	//DestroyGlareDepthTexture ();
	ogl.SetTexturing (true);
	ogl.SelectTMU (GL_TEXTURE1);
	ogl.BindTexture (0);
	ogl.SelectTMU (GL_TEXTURE2);
	ogl.BindTexture (0);
	ogl.SelectTMU (GL_TEXTURE0);
	ogl.SetDepthTest (true);
	}
}

//------------------------------------------------------------------------------

const char *glareFS =
	"uniform sampler2D glareTex, depthTex;\r\n" \
	"uniform float dMax;\r\n" \
	"uniform vec2 windowScale;\r\n" \
	"uniform int blendMode;\r\n" \
	"uniform int bSuspended;\r\n" \
	"#define ZNEAR 1.0\r\n" \
	"#define ZFAR 5000.0\r\n" \
	"#define NDC(z) (2.0 * z - 1.0)\r\n" \
	"#define A (ZNEAR + ZFAR)\r\n" \
	"#define B (ZNEAR - ZFAR)\r\n" \
	"#define C (2.0 * ZNEAR * ZFAR)\r\n" \
	"#define D(z) (NDC (z) * B)\r\n" \
	"#define ZEYE(z) (C / (A + D (z)))\r\n" \
	"void main (void) {\r\n" \
	"if (bSuspended != 0)\r\n" \
	"   gl_FragColor = texture2D (glareTex, gl_TexCoord [0].xy) * gl_Color;\r\n" \
	"else {\r\n" \
	"   float dz = clamp (ZEYE (gl_FragCoord.z) - ZEYE (texture2D (depthTex, gl_FragCoord.xy * windowScale).r), 0.0, dMax);\r\n" \
	"   dz = (dMax - dz) / dMax;\r\n" \
	"   vec4 glareColor = texture2D (glareTex, gl_TexCoord [0].xy);\r\n" \
	"   if (blendMode > 0) //additive\r\n" \
	"      gl_FragColor = vec4 (glareColor.rgb * gl_Color.rgb * dz, 1.0);\r\n" \
	"   else if (blendMode < 0) //multiplicative\r\n" \
	"      gl_FragColor = vec4 (max (glareColor.rgb, 1.0 - dz), 1.0);\r\n" \
	"   else //alpha\r\n" \
	"      gl_FragColor = vec4 (glareColor.rgb * gl_Color.rgb, glareColor.a * gl_Color.a * dz);\r\n" \
	"   }\r\n" \
	"}\r\n"
	;

const char *glareVS =
	"void main (void){\r\n" \
	"gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"gl_Position = ftransform (); //gl_ModelViewProjectionMatrix * gl_Vertex;\r\n" \
	"gl_FrontColor = gl_Color;}\r\n"
	;

//-------------------------------------------------------------------------

void CGlareRenderer::InitShader (void)
{
if (ogl.m_features.bRenderToTexture && ogl.m_features.bShaders && (ogl.m_features.bDepthBlending > -1)) {
	PrintLog (0, "building corona blending shader program\n");
	m_shaderProg = 0;
	if (shaderManager.Build (hGlareShader, glareFS, glareVS)) {
		ogl.m_features.bDepthBlending.Available (1);
		ogl.m_features.bDepthBlending = 1;
		}
	else {
		ogl.ClearError (0);
		ogl.m_features.bDepthBlending.Available (0);
		}
	}
}

//------------------------------------------------------------------------------
// eof
