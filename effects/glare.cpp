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
v.v.coord.x = v.v.coord.z = 0;
v.v.coord.y = (*vCenter).v.coord.y ? d / (*vCenter).v.coord.y : 1;
v -= *vCenter;
CFloatVector::Normalize (v);
vdx = CFloatVector::Cross (v, *vEye);	//orthogonal vector in plane through face center and perpendicular to viewer
vdx = vdx * dx;
v.v.coord.y = v.v.coord.z = 0;
v.v.coord.x = (*vCenter).v.coord.x ? d / (*vCenter).v.coord.x : 1;
v -= *vCenter;
CFloatVector::Normalize (v);
vdy = CFloatVector::Cross (v, *vEye);
if (r) {
	if ((*vCenter).v.coord.x >= 0) {
		vdy = vdy * dy;
		v.v.coord.x = +vdx.v.coord.x + vdy.v.coord.x;
		v.v.coord.y = +vdx.v.coord.y + vdy.v.coord.y;
		v.v.coord.z = -vdx.v.coord.z - vdy.v.coord.z;
		*vSprite = *r * v;
		vSprite [0] += *vCenter;
		v.v.coord.x = v.v.coord.y = v.v.coord.z = 0;
		v.v.coord.x = -vdx.v.coord.x + vdy.v.coord.x;
		v.v.coord.y = +vdx.v.coord.y + vdy.v.coord.y;
		v.v.coord.z = +vdx.v.coord.z - vdy.v.coord.z;
		vSprite [1] = *r * v;
		vSprite [1] += *vCenter;
		v.v.coord.x = v.v.coord.y = v.v.coord.z = 0;
		v.v.coord.x = -vdx.v.coord.x - vdy.v.coord.x;
		v.v.coord.y = -vdx.v.coord.y - vdy.v.coord.y;
		v.v.coord.z = +vdx.v.coord.z + vdy.v.coord.z;
		vSprite [2] =  *r * v;
		vSprite [2] += *vCenter;
		v.v.coord.x = +vdx.v.coord.x - vdy.v.coord.x;
		v.v.coord.y = -vdx.v.coord.y - vdy.v.coord.y;
		v.v.coord.z = -vdx.v.coord.z + vdy.v.coord.z;
		vSprite [3] = *r * v;
		vSprite [3] += *vCenter;
		}
	else {
		vdy = vdy * dy;
		v.v.coord.x = -vdx.v.coord.x - vdy.v.coord.x;
		v.v.coord.y = -vdx.v.coord.y - vdy.v.coord.y;
		v.v.coord.z = -vdx.v.coord.z - vdy.v.coord.z;
		*vSprite = *r * v;
		vSprite [0] += *vCenter;
		v.v.coord.x = v.v.coord.y = v.v.coord.z = 0;
		v.v.coord.x = +vdx.v.coord.x - vdy.v.coord.x;
		v.v.coord.y = -vdx.v.coord.y - vdy.v.coord.y;
		v.v.coord.z = +vdx.v.coord.z - vdy.v.coord.z;
		vSprite [1] = *r * v;
		vSprite [1] += *vCenter;
		v.v.coord.x = v.v.coord.y = v.v.coord.z = 0;
		v.v.coord.x = +vdx.v.coord.x + vdy.v.coord.x;
		v.v.coord.y = +vdx.v.coord.y + vdy.v.coord.y;
		v.v.coord.z = +vdx.v.coord.z + vdy.v.coord.z;
		vSprite [2] = *r * v;
		vSprite [2] += *vCenter;
		v.v.coord.x = -vdx.v.coord.x + vdy.v.coord.x;
		v.v.coord.y = +vdx.v.coord.y + vdy.v.coord.y;
		v.v.coord.z = -vdx.v.coord.z + vdy.v.coord.z;
		vSprite [3] = *r * v;
		vSprite [3] += *vCenter;
		}
	}
else {
	vSprite [0].v.coord.x = -vdx.v.coord.x - vdy.v.coord.x;
	vSprite [0].v.coord.y = -vdx.v.coord.y - vdy.v.coord.y;
	vSprite [0].v.coord.z = -vdx.v.coord.z - vdy.v.coord.z;
	vSprite [1].v.coord.x = +vdx.v.coord.x - vdy.v.coord.x;
	vSprite [1].v.coord.y = +vdx.v.coord.y - vdy.v.coord.y;
	vSprite [1].v.coord.z = +vdx.v.coord.z - vdy.v.coord.z;
	vSprite [2].v.coord.x = +vdx.v.coord.x + vdy.v.coord.x;
	vSprite [2].v.coord.y = +vdx.v.coord.y + vdy.v.coord.y;
	vSprite [2].v.coord.z = +vdx.v.coord.z + vdy.v.coord.z;
	vSprite [3].v.coord.x = -vdx.v.coord.x + vdy.v.coord.x;
	vSprite [3].v.coord.y = -vdx.v.coord.y + vdy.v.coord.y;
	vSprite [3].v.coord.z = -vdx.v.coord.z + vdy.v.coord.z;
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
return fLight;
}

// -----------------------------------------------------------------------------------

void CGlareRenderer::ComputeSpriteZRange (CFloatVector *sprite, tIntervalf *zRangeP)
{
	float			z;
	tIntervalf	zRange = {1000000000.0f, -1000000000.0f};
	int			i;

for (i = 0; i < 4; i++) {
	z = sprite [i].v.coord.z;
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
	*vCenter = *vCenter * (zRange.fMin / (*vCenter).v.coord.z);
else {
	if (zRange.fMin < -zRange.fRad)
		return 0;
	fIntensity *= 1 + zRange.fMin / zRange.fRad * 2;
	 (*vCenter).v.coord.x /= (*vCenter).v.coord.z;
	 (*vCenter).v.coord.y /= (*vCenter).v.coord.y;
	 (*vCenter).v.coord.z = 1;
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
	tTexCoord2f		tcGlare [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};
	CFloatVector	color;
	CBitmap*			bmP;

fLight /= 4;
if (fLight < 0.01f)
	return;
color.Alpha () = vCenter->Mag ();
if (color.Alpha () < zRangeP->fRad)
	fIntensity *= color.Alpha () / zRangeP->fRad;

if (gameStates.render.bAmbientColor) {
	color = gameData.render.color.textures [nTexture];
	color.Alpha () = (float) (color.Red () * 3 + color.Green () * 5 + color.Blue () * 2) / 30 * 2;
	}
else {
	color.Alpha () = X2F (IsLight (nTexture));
	color.Red () = color.Green () = color.Blue () = color.Alpha () / 2;
	color.Alpha () *= 2.0f / 3.0f;
	}
if (!bColored)
	color.Red () = color.Green () = color.Blue () = (color.Red () + color.Green () + color.Blue ()) / 4;
color.Alpha () *= fIntensity * fIntensity;
if (color.Alpha () < 0.01f)
	return;
if (!(bmP = (bAdditive ? glare.Bitmap () : corona.Bitmap ())))
	return;
bmP->SetTranspType (-1);
ogl.SetFaceCulling (false);
if (bAdditive) {
	fLight *= color.Alpha ();
	ogl.SetBlendMode (OGL_BLEND_ADD);
	}
bmP->SetColor (&color);
bmP->SetTexCoord (tcGlare);
ogl.RenderQuad (bmP, sprite, 3);
ogl.SetBlendMode (OGL_BLEND_ALPHA);
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
	sprite [0].v.vec [i] = v.v.vec [i] + s.v.vec [i] + t.v.vec [i];
	sprite [1].v.vec [i] = v.v.vec [i] + s.v.vec [i] - t.v.vec [i];
	sprite [2].v.vec [i] = v.v.vec [i] - s.v.vec [i] - t.v.vec [i];
	sprite [3].v.vec [i] = v.v.vec [i] - s.v.vec [i] + t.v.vec [i];
	}
cosine = CFloatVector::Dot (e, n);
return float (sqrt (cosine) * coronaIntensities [gameOpts->render.coronas.nIntensity]);
}

// -----------------------------------------------------------------------------------

void CGlareRenderer::RenderSoftGlare (CFloatVector *sprite, CFloatVector *vCenter, int nTexture, float fIntensity, int bAdditive, int bColored)
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
OglVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), sprite);
OglDrawArrays (GL_QUADS, 0, 4);
RenderCoronaOutline (sprite, vCenter);
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
//shaderManager.Set ("dMax"), 20.0f);
RenderSoftGlare (sprite, &vCenter, nTexture, fIntensity, bAdditive,
					  !automap.Display () || automap.m_visited [nSegment] || !gameOpts->render.automap.bGrayOut);
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
