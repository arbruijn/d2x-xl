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
#include "gameseg.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "renderlib.h"
#include "network.h"
#include "glare.h"
#include "automap.h"

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
return ogl.m_states.bDepthBlending && gameOpts->render.coronas.bUse && gameOpts->render.coronas.nStyle && !gameStates.render.cameras.bActive;
}

// -----------------------------------------------------------------------------------

void CGlareRenderer::CalcSpriteCoords (CFloatVector *vSprite, CFloatVector *vCenter, CFloatVector *vEye, float dx, float dy, CFloatMatrix *r)
{
	CFloatVector	v, h, vdx, vdy;
	float		d = vCenter->SqrMag ();
	int		i;

if (!vEye) {
	vEye = &h;
	//CFloatVector::Normalize (vEye, vCenter);
	*vEye = *vCenter; CFloatVector::Normalize (*vEye);
	}
v [X] = v [Z] = 0;
v [Y] = (*vCenter) [Y] ? d / (*vCenter) [Y] : 1;
v -= *vCenter;
CFloatVector::Normalize (v);
vdx = CFloatVector::Cross (v, *vEye);	//orthogonal vector in plane through face center and perpendicular to viewer
vdx = vdx * dx;
v [Y] = v [Z] = 0;
v [X] = (*vCenter) [X] ? d / (*vCenter) [X] : 1;
v -= *vCenter;
CFloatVector::Normalize (v);
vdy = CFloatVector::Cross (v, *vEye);
if (r) {
	if ((*vCenter) [X] >= 0) {
		vdy = vdy * dy;
		v [X] = +vdx [X] + vdy [X];
		v [Y] = +vdx [Y] + vdy [Y];
		v [Z] = -vdx [Z] - vdy [Z];
		*vSprite = *r * v;
		vSprite [0] += *vCenter;
		v [X] = v [Y] = v [Z] = 0;
		v [X] = -vdx [X] + vdy [X];
		v [Y] = +vdx [Y] + vdy [Y];
		v [Z] = +vdx [Z] - vdy [Z];
		vSprite [1] = *r * v;
		vSprite [1] += *vCenter;
		v [X] = v [Y] = v [Z] = 0;
		v [X] = -vdx [X] - vdy [X];
		v [Y] = -vdx [Y] - vdy [Y];
		v [Z] = +vdx [Z] + vdy [Z];
		vSprite [2] =  *r * v;
		vSprite [2] += *vCenter;
		v [X] = +vdx [X] - vdy [X];
		v [Y] = -vdx [Y] - vdy [Y];
		v [Z] = -vdx [Z] + vdy [Z];
		vSprite [3] = *r * v;
		vSprite [3] += *vCenter;
		}
	else {
		vdy = vdy * dy;
		v [X] = -vdx [X] - vdy [X];
		v [Y] = -vdx [Y] - vdy [Y];
		v [Z] = -vdx [Z] - vdy [Z];
		*vSprite = *r * v;
		vSprite [0] += *vCenter;
		v [X] = v [Y] = v [Z] = 0;
		v [X] = +vdx [X] - vdy [X];
		v [Y] = -vdx [Y] - vdy [Y];
		v [Z] = +vdx [Z] - vdy [Z];
		vSprite [1] = *r * v;
		vSprite [1] += *vCenter;
		v [X] = v [Y] = v [Z] = 0;
		v [X] = +vdx [X] + vdy [X];
		v [Y] = +vdx [Y] + vdy [Y];
		v [Z] = +vdx [Z] + vdy [Z];
		vSprite [2] = *r * v;
		vSprite [2] += *vCenter;
		v [X] = -vdx [X] + vdy [X];
		v [Y] = +vdx [Y] + vdy [Y];
		v [Z] = -vdx [Z] + vdy [Z];
		vSprite [3] = *r * v;
		vSprite [3] += *vCenter;
		}
	}
else {
	vSprite [0][X] = -vdx [X] - vdy [X];
	vSprite [0][Y] = -vdx [Y] - vdy [Y];
	vSprite [0][Z] = -vdx [Z] - vdy [Z];
	vSprite [1][X] = +vdx [X] - vdy [X];
	vSprite [1][Y] = +vdx [Y] - vdy [Y];
	vSprite [1][Z] = +vdx [Z] - vdy [Z];
	vSprite [2][X] = +vdx [X] + vdy [X];
	vSprite [2][Y] = +vdx [Y] + vdy [Y];
	vSprite [2][Z] = +vdx [Z] + vdy [Z];
	vSprite [3][X] = -vdx [X] + vdy [X];
	vSprite [3][Y] = -vdx [Y] + vdy [Y];
	vSprite [3][Z] = -vdx [Z] + vdy [Z];
	for (i = 0; i < 4; i++)
		vSprite [i] += *vCenter;
	}
}

// -----------------------------------------------------------------------------------

int CGlareRenderer::CalcFaceDimensions (short nSegment, short nSide, fix *w, fix *h, short* corners)
{
	fix		d, d1, d2, dMax = -1;
	int		i, j;

if (!corners) {
	corners = SEGMENTS [nSegment].Corners (nSide);
	}
for (i = j = 0; j < 4; j++) {
	d = CFixVector::Dist (gameData.segs.vertices [corners [j]], gameData.segs.vertices [corners [(j + 1) % 4]]);
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
                      gameData.segs.vertices [corners [(j + 1) % 4]]);
d = CFixVector::Dist (gameData.segs.vertices [corners [i]], gameData.segs.vertices [corners [(j + 1) % 4]]);
d = CFixVector::Dist (gameData.segs.vertices [corners [j]], gameData.segs.vertices [corners [(j + 1) % 4]]);
d2 = VmLinePointDist (gameData.segs.vertices [corners [i]],
                      gameData.segs.vertices [corners [j]],
                      gameData.segs.vertices [corners [(j + 2) % 4]]);
d = CFixVector::Dist (gameData.segs.vertices [corners [i]], gameData.segs.vertices [corners [(j + 2) % 4]]);
d = CFixVector::Dist (gameData.segs.vertices [corners [j]], gameData.segs.vertices [corners [(j + 2) % 4]]);
if (h)
	*h = d1 > d2 ? d1 : d2;
return i;
}

// -----------------------------------------------------------------------------------

int CGlareRenderer::FaceHasCorona (short nSegment, short nSide, int *bAdditiveP, float *fIntensityP)
{
	ushort		nWall;
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
nWall = sideP->m_nWall;
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

float CGlareRenderer::ComputeCoronaSprite (CFloatVector *sprite, CFloatVector *vCenter, short nSegment, short nSide)
{
	CSide*			sideP = SEGMENTS [nSegment].m_sides + nSide;
	short*			corners;
	int				i;
	float				fLight = 0;
	CFloatVector	v;

corners = SEGMENTS [nSegment].Corners (nSide);
for (i = 0; i < 4; i++) {
	fLight += X2F (sideP->m_uvls [i].l);
	transformation.Transform (sprite [i], gameData.segs.fVertices [corners [i]], 0);
	}
v.Assign (SEGMENTS [nSegment].SideCenter (nSide));
transformation.Transform (*vCenter, v, 0);
#if 0
if (gameStates.render.bQueryCoronas) {
	for (i = 0; i < 4; i++) {
		v = sprite [i] - *vCenter;
		VmVecScaleAdd (sprite + i, vCenter, &v, 0.5f);
	}
}
#endif
return fLight;
}

// -----------------------------------------------------------------------------------

void CGlareRenderer::ComputeSpriteZRange (CFloatVector *sprite, tIntervalf *zRangeP)
{
	float			z;
	tIntervalf	zRange = {1000000000.0f, -1000000000.0f};
	int			i;

for (i = 0; i < 4; i++) {
	z = sprite [i][Z];
	if (zRange.fMin > z)
		zRange.fMin = z;
	if (zRange.fMax < z)
		zRange.fMax = z;
	}
zRange.fSize = zRange.fMax - zRange.fMin + 1;
zRange.fRad = zRange.fSize / 2;
*zRangeP = zRange;
}

// -----------------------------------------------------------------------------------

float CGlareRenderer::MoveSpriteIn (CFloatVector *sprite, CFloatVector *vCenter, tIntervalf *zRangeP, float fIntensity)
{
	tIntervalf	zRange;

ComputeSpriteZRange (sprite, &zRange);
if (zRange.fMin > 0)
	*vCenter = *vCenter * (zRange.fMin / (*vCenter) [Z]);
else {
	if (zRange.fMin < -zRange.fRad)
		return 0;
	fIntensity *= 1 + zRange.fMin / zRange.fRad * 2;
	 (*vCenter) [X] /= (*vCenter) [Z];
	 (*vCenter) [Y] /= (*vCenter) [Y];
	 (*vCenter) [Z] = 1;
	}
*zRangeP = zRange;
return fIntensity;
}

// -----------------------------------------------------------------------------------

void CGlareRenderer::ComputeHardGlare (CFloatVector *sprite, CFloatVector *vCenter, CFloatVector *vNormal)
{
	CFloatVector	u, v, p, q, e, s, t;
	float		h, g;

u = sprite [2] + sprite [1];
u -= sprite [0];
u -= sprite [3];
u = u * 0.25f;
v = sprite [0] + sprite [1];
v -= sprite [2];
v -= sprite [3];
v = v * 0.25f;
*vNormal = CFloatVector::Cross (v, u);
CFloatVector::Normalize (*vNormal);
e = *vCenter; CFloatVector::Normalize (e);
if (CFloatVector::Dot (e, *vNormal) > 0.999f)
	p = v;
else {
	p = CFloatVector::Cross (e, *vNormal);
	CFloatVector::Normalize (p);
}

q = CFloatVector::Cross (p, e);
h = u.Mag ();
g = v.Mag ();
if (h > g)
	h = g;
g = 2 * (float) (fabs (CFloatVector::Dot (p, v)) + fabs (CFloatVector::Dot (p, u))) + h * CFloatVector::Dot (p, *vNormal);
h = 2 * (float) (fabs (CFloatVector::Dot (q, v)) + fabs (CFloatVector::Dot (q, u))) + h * CFloatVector::Dot (q, *vNormal);
#if 1
if (g / h > 8)
	h = g / 8;
else if (h / g > 8)
	g = h / 8;
#endif
s = p * g;
t = q * h;

sprite [0] = *vCenter + s;
sprite [1] = sprite [0];
sprite [0] += t;
sprite [1] -= t;
sprite [3] = *vCenter - s;
sprite [2] = sprite [3];
sprite [3] += t;
sprite [2] -= t;
}

// -----------------------------------------------------------------------------------

#if DBG && CORONA_OUTLINE

void RenderCoronaOutline(CFloatVector *sprite, CFloatVector *vCenter)
{
	int	i;

ogl.SetTexturing (false);
glColor4d (1,1,1,1);
glLineWidth (2);
glBegin (GL_LINE_LOOP);
for (i = 0; i < 4; i++)
	glVertex3fv (reinterpret_cast<GLfloat*> (sprite + i));
glEnd ();
glBegin (GL_LINES);
vCenter->x () += 5;
glVertex3fv (reinterpret_cast<GLfloat*> (&vCenter));
vCenter->x () -= 10;
glVertex3fv (reinterpret_cast<GLfloat*> (&vCenter));
vCenter->x () += 5;
vCenter->y () += 5;
glVertex3fv (reinterpret_cast<GLfloat*> (&vCenter));
vCenter->y () -= 10;
glVertex3fv (reinterpret_cast<GLfloat*> (&vCenter));
glEnd ();
glLineWidth (1);
}

#else

#	define RenderCoronaOutline(_sprite,_center)

#endif

// -----------------------------------------------------------------------------------

void CGlareRenderer::RenderHardGlare (CFloatVector *sprite, CFloatVector *vCenter, int nTexture, float fLight,
												  float fIntensity, tIntervalf *zRangeP, int bAdditive, int bColored)
{
	tTexCoord2f	tcGlare [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};
	tRgbaColorf	color;
	CBitmap*		bmP;

fLight /= 4;
if (fLight < 0.01f)
	return;
color.alpha = vCenter->Mag ();
if (color.alpha < zRangeP->fRad)
	fIntensity *= color.alpha / zRangeP->fRad;

if (gameStates.render.bAmbientColor) {
	color = gameData.render.color.textures [nTexture].color;
	color.alpha = (float) (color.red * 3 + color.green * 5 + color.blue * 2) / 30 * 2;
	}
else {
	color.alpha = X2F (IsLight (nTexture));
	color.red = color.green = color.blue = color.alpha / 2;
	color.alpha *= 2.0f / 3.0f;
	}
if (!bColored)
	color.red = color.green = color.blue = (color.red + color.green + color.blue) / 4;
color.alpha *= fIntensity * fIntensity;
if (color.alpha < 0.01f)
	return;
if (!(bmP = bAdditive ? bmpGlare : bmpCorona))
	return;
bmP->SetTranspType (-1);
ogl.SetFaceCulling (false);
if (bAdditive) {
	fLight *= color.alpha;
	ogl.SetBlendMode (GL_ONE, GL_ONE);
	}
bmP->SetColor (&color);
bmP->SetTexCoord (tcGlare);
ogl.RenderQuad (bmP, sprite, 3);
ogl.SetBlendMode (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
ogl.SetFaceCulling (true);
RenderCoronaOutline (sprite, vCenter);
}

// -----------------------------------------------------------------------------------

float CGlareRenderer::ComputeSoftGlare (CFloatVector *sprite, CFloatVector *vLight, CFloatVector *vEye)
{
	CFloatVector 	n, e, s, t, u, v;
	float 			ul, vl, h, cosine;
	int 				i;

u = sprite [2] + sprite [1];
u -= sprite [0];
u -= sprite [3];
u = u * 0.25f;
v = sprite [0] + sprite [1];
v -= sprite [2];
v -= sprite [3];
v = v * 0.25f;
e = *vEye - *vLight;
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
v = *vLight;
for (i = 0; i < 3; i++) {
	sprite [0][i] = v [i] + s [i] + t [i];
	sprite [1][i] = v [i] + s [i] - t [i];
	sprite [2][i] = v [i] - s [i] - t [i];
	sprite [3][i] = v [i] - s [i] + t [i];
	}
cosine = CFloatVector::Dot (e, n);
return float (sqrt (cosine) * coronaIntensities [gameOpts->render.coronas.nIntensity]);
}

// -----------------------------------------------------------------------------------

void CGlareRenderer::RenderSoftGlare (CFloatVector *sprite, CFloatVector *vCenter, int nTexture, float fIntensity, int bAdditive, int bColored)
{
	tRgbaColorf color;
	tTexCoord2f	tcGlare [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};
	CBitmap*		bmP = NULL;

//ogl.SetBlendMode (bAdditive);
if (!(bmP = bAdditive ? bmpGlare : bmpCorona))
	return;
if (gameStates.render.bAmbientColor)
	color = gameData.render.color.textures [nTexture].color;
else
	color.red = color.green = color.blue = X2F (IsLight (nTexture)) / 2;
if (!bColored)
	color.red = color.green = color.blue = (color.red + color.green + color.blue) / 4;
if (bAdditive)
	glColor4f (fIntensity * color.red, fIntensity * color.green, fIntensity * color.blue, 1);
else
	glColor4f (color.red, color.green, color.blue, fIntensity);
bmP->Bind (1);
OglTexCoordPointer (2, GL_FLOAT, 0, tcGlare);
OglVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), sprite);
OglDrawArrays (GL_QUADS, 0, 4);
RenderCoronaOutline (sprite, vCenter);
#if 0
if (gameStates.render.bQueryCoronas != 2) {
	ogl.SetDepthTest (true);
	if (gameStates.render.bQueryCoronas == 1)
		ogl.SetDepthMode (GL_LEQUAL);
	}
#endif
}

// -----------------------------------------------------------------------------------

void CGlareRenderer::Render (short nSegment, short nSide, float fIntensity, float fSize)
{
if (!Style ())
	return;
if (fIntensity < 0.01f)
	return;

	CFloatVector	sprite [4], vCenter = CFloatVector::ZERO, vEye = CFloatVector::ZERO;
	int				nTexture, bAdditive;
	float				fLight;

#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif

if (!(nTexture = FaceHasCorona (nSegment, nSide, &bAdditive, &fIntensity)))
	return;
fLight = ComputeCoronaSprite (sprite, &vCenter, nSegment, nSide);
fIntensity *= ComputeSoftGlare (sprite, &vCenter, &vEye);
//glUniform1f (glGetUniformLocation (m_shaderProg, "dMax"), 20.0f);
RenderSoftGlare (sprite, &vCenter, nTexture, fIntensity, bAdditive,
					  !automap.Display () || automap.m_visited [0][nSegment] || !gameOpts->render.automap.bGrayOut);
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

if (! (ogl.m_states.bOcclusionQuery && nQuery) || (Style () != 1))
	return 1;
if (! (gameStates.render.bQueryCoronas || gameData.render.lights.coronaSamples [nQuery - 1]))
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
ogl.m_states.bUseDepthBlending = 0;
if (!ogl.m_states.bDepthBlending) 
	return false;
if (!ogl.CopyDepthTexture ())
	return false;
ogl.m_states.bUseDepthBlending = 1;
if (dMax < 1)
	dMax = 1;
m_shaderProg = GLhandleARB (shaderManager.Deploy (hGlareShader));
if (!m_shaderProg)
	return false;
if (shaderManager.Rebuild (m_shaderProg)) {
	glUniform1i (glGetUniformLocation (m_shaderProg, "glareTex"), 0);
	glUniform1i (glGetUniformLocation (m_shaderProg, "depthTex"), 1);
	glUniform2fv (glGetUniformLocation (m_shaderProg, "screenScale"), 1, reinterpret_cast<GLfloat*> (&ogl.m_data.screenScale));
	glUniform1f (glGetUniformLocation (m_shaderProg, "dMax"), (GLfloat) dMax);
	glUniform1i (glGetUniformLocation (m_shaderProg, "blendMode"), (GLint) nBlendMode);
	}
else {
	if (dMaxPrev != dMax)
		glUniform1f (glGetUniformLocation (m_shaderProg, "dMax"), (GLfloat) dMax);
	if (nBlendPrev != nBlendMode)
		glUniform1i (glGetUniformLocation (m_shaderProg, "blendMode"), (GLint) nBlendMode);
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
if (ogl.m_states.bDepthBlending) {
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
	"uniform vec2 screenScale;\r\n" \
	"uniform int blendMode;\r\n" \
	"//#define ZNEAR 1.0\r\n" \
	"//#define ZFAR 5000.0\r\n" \
	"//#define LinearDepth(_z) (2.0 * ZFAR) / (ZFAR + ZNEAR - (_z) * (ZFAR - ZNEAR))\r\n" \
	"#define LinearDepth(_z) 10000.0 / (5001.0 - (_z) * 4999.0)\r\n" \
	"void main (void) {\r\n" \
	"//float texZ = LinearDepth (texture2D (depthTex, screenScale * gl_FragCoord.xy).r);\r\n" \
	"//float fragZ = LinearDepth (gl_FragCoord.z);\r\n" \
	"//float dz = clamp (fragZ - texZ, 0.0, dMax);\r\n" \
	"float dz = clamp (LinearDepth (gl_FragCoord.z) - LinearDepth (texture2D (depthTex, screenScale * gl_FragCoord.xy).r), 0.0, dMax);\r\n" \
	"dz = (dMax - dz) / dMax;\r\n" \
	"vec4 texColor = texture2D (glareTex, gl_TexCoord [0].xy);\r\n" \
	"//gl_FragColor = vec4 (texColor.rgb * gl_Color.rgb, texColor.a * gl_Color.a * dz);\r\n" \
	"if (blendMode > 0) //additive\r\n" \
	"   gl_FragColor = vec4 (texColor.rgb * gl_Color.rgb * dz, 1.0);\r\n" \
	"else if (blendMode < 0) //multiplicative\r\n" \
	"   gl_FragColor = vec4 (max (texColor.rgb, 1.0 - dz), 1.0);\r\n" \
	"else //alpha\r\n" \
	"   gl_FragColor = vec4 (texColor.rgb * gl_Color.rgb, texColor.a * gl_Color.a * dz);\r\n" \
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
ogl.m_states.bDepthBlending = 0;
#if SHADER_SOFT_CORONAS
PrintLog ("building corona blending shader program\n");
//DeleteShaderProg (NULL);
if (ogl.m_states.bRender2TextureOk && ogl.m_states.bShadersOk) {
	ogl.m_states.bDepthBlending = 1;
	m_shaderProg = 0;
	if (!shaderManager.Build (hGlareShader, glareFS, glareVS)) {
		ogl.ClearError (0);
		ogl.m_states.bDepthBlending = 0;
		}
	}
#endif
}

//------------------------------------------------------------------------------
// eof
