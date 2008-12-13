// Copyright (c) Dietfrid Mali

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "inferno.h"
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

#define CORONA_OUTLINE	0
#if DBG
#	define SHADER_SOFT_CORONAS 1
#else
#	define SHADER_SOFT_CORONAS 1
#endif

float coronaIntensities [] = {0.25f, 0.5f, 0.75f, 1};

GLhandleARB hGlareShader = 0;
GLhandleARB hGlareVS = 0;
GLhandleARB hGlareFS = 0;

// -----------------------------------------------------------------------------------

int CoronaStyle (void)
{
switch (gameOpts->render.coronas.nStyle) {
	case 2:
		if (!gameStates.render.cameras.bActive)
			return 2;
	case 1:
		return 1;
	default:
		return 0;
	}
}

// -----------------------------------------------------------------------------------

void DestroyGlareDepthTexture (void)
{
if (gameStates.ogl.hDepthBuffer) {
	OglDeleteTextures (1, &gameStates.ogl.hDepthBuffer);
	gameStates.ogl.hDepthBuffer = 0;
	}
}

// -----------------------------------------------------------------------------------

GLuint CopyDepthTexture (void)
{
	GLenum nError = glGetError ();

#if DBG
if (nError)
	nError = nError;
#endif
glActiveTexture (GL_TEXTURE1);
glEnable (GL_TEXTURE_2D);
if (!gameStates.ogl.hDepthBuffer)
	gameStates.ogl.bHaveDepthBuffer = 0;
if (gameStates.ogl.hDepthBuffer || (gameStates.ogl.hDepthBuffer = OglCreateDepthTexture (-1, 0))) {
	glBindTexture (GL_TEXTURE_2D, gameStates.ogl.hDepthBuffer);
	if (!gameStates.ogl.bHaveDepthBuffer) {
#if 0
		glCopyTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 0, 0, screen.Width (), screen.Height (), 0);
#else
		glCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 0, 0, screen.Width (), screen.Height ());
#endif
		if (nError = glGetError ()) {
			glCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 0, 0, screen.Width (), screen.Height ());
			if (nError = glGetError ()) {
				DestroyGlareDepthTexture ();
				return gameStates.ogl.hDepthBuffer = 0;
				}
			}
		gameStates.ogl.bHaveDepthBuffer = 1;
		gameData.render.nStateChanges++;
		}
	}
return gameStates.ogl.hDepthBuffer;
}

// -----------------------------------------------------------------------------------

void CalcSpriteCoords (CFloatVector *vSprite, CFloatVector *vCenter, CFloatVector *vEye, float dx, float dy, CFloatMatrix *r)
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

int CalcFaceDimensions (short nSegment, short nSide, fix *w, fix *h, short *pSideVerts)
{
	short			sideVerts [4];
	fix			d, d1, d2, dMax = -1;
	int			i, j;

if (!pSideVerts) {
	GetSideVertIndex (sideVerts, nSegment, nSide);
	pSideVerts = sideVerts;
	}
for (i = j = 0; j < 4; j++) {
	d = CFixVector::Dist (gameData.segs.vertices [pSideVerts [j]], gameData.segs.vertices [pSideVerts [ (j + 1) % 4]]);
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
d1 = VmLinePointDist (gameData.segs.vertices [pSideVerts [i]],
                     gameData.segs.vertices [pSideVerts [j]],
                     gameData.segs.vertices [pSideVerts [ (j+1)%4]]);
d = CFixVector::Dist (gameData.segs.vertices [pSideVerts [i]], gameData.segs.vertices [pSideVerts [ (j + 1) % 4]]);
d = CFixVector::Dist (gameData.segs.vertices [pSideVerts [j]], gameData.segs.vertices [pSideVerts [ (j + 1) % 4]]);
d2 = VmLinePointDist (gameData.segs.vertices [pSideVerts [i]],
                     gameData.segs.vertices [pSideVerts [j]],
                     gameData.segs.vertices [pSideVerts [ (j+2)%4]]);
d = CFixVector::Dist (gameData.segs.vertices [pSideVerts [i]], gameData.segs.vertices [pSideVerts [ (j + 2) % 4]]);
d = CFixVector::Dist (gameData.segs.vertices [pSideVerts [j]], gameData.segs.vertices [pSideVerts [ (j + 2) % 4]]);
if (h)
	*h = d1 > d2 ? d1 : d2;
return i;
}

// -----------------------------------------------------------------------------------

int FaceHasCorona (short nSegment, short nSide, int *bAdditiveP, float *fIntensityP)
{
	ushort		nWall;
	tSide			*sideP;
	char			*pszName;
	int			i, bAdditive, nTexture, nBrightness;

if (IsMultiGame && extraGameInfo [1].bDarkness)
	return 0;
#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
sideP = gameData.segs.segments [nSegment].sides + nSide;
nWall = sideP->nWall;
if (IS_WALL (nWall)) {
	tWall *wallP = gameData.walls.walls + nWall;
	ubyte nType = wallP->nType;

	if ((nType == WALL_BLASTABLE) || (nType == WALL_DOOR) || (nType == WALL_OPEN) || (nType == WALL_CLOAKED))
		return 0;
	if (wallP->flags & (WALL_BLASTED | WALL_ILLUSION_OFF))
		return 0;
	}
// get and check the corona emitting texture
nBrightness = (nTexture = sideP->nOvlTex) ? IsLight (nTexture) : 0;
if (nBrightness >= F1_0 / 8) {
	bAdditive = gameOpts->render.coronas.bAdditive;
	}
else if ((nBrightness = IsLight (nTexture = sideP->nBaseTex))) {
	if (fIntensityP)
		*fIntensityP /= 2;
	bAdditive = 0;
	}
else
	return 0;
if (gameOpts->render.nLightingMethod) {
	i = FindDynLight (nSegment, nSide, -1);
	if ((i < 0) || !gameData.render.lights.dynamic.lights [i].info.bOn)
		return 0;
	}
#if 0
if (fIntensityP && (nBrightness < F1_0))
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

float ComputeCoronaSprite (CFloatVector *sprite, CFloatVector *vCenter, short nSegment, short nSide)
{
	tSide		*sideP = gameData.segs.segments [nSegment].sides + nSide;
	short		sideVerts [4];
	int		i;
	float		fLight = 0;
	CFloatVector	v;

GetSideVertIndex (sideVerts, nSegment, nSide);
for (i = 0; i < 4; i++) {
	fLight += X2F (sideP->uvls [i].l);
	if (RENDERPATH)
		G3TransformPoint (sprite [i], gameData.segs.fVertices [sideVerts [i]], 0);
	else
		sprite [i] = gameData.segs.fVertices [sideVerts [i]];	//already transformed
	}
v = SIDE_CENTER_I (nSegment, nSide)->ToFloat ();
G3TransformPoint (*vCenter, v, 0);
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

void ComputeSpriteZRange (CFloatVector *sprite, tIntervalf *zRangeP)
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

float MoveSpriteIn (CFloatVector *sprite, CFloatVector *vCenter, tIntervalf *zRangeP, float fIntensity)
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

void ComputeHardGlare (CFloatVector *sprite, CFloatVector *vCenter, CFloatVector *vNormal)
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

#if defined (_DEBUG) && CORONA_OUTLINE

void RenderCoronaOutline(CFloatVector *sprite, CFloatVector *vCenter)
{
	int	i;

glDisable (GL_TEXTURE_2D);
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

void RenderHardGlare (CFloatVector *sprite, CFloatVector *vCenter, int nTexture, float fLight,
							 float fIntensity, tIntervalf *zRangeP, int bAdditive, int bColored)
{
	tTexCoord2f	tcGlare [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};
	tRgbaColorf	color;
	CBitmap	*bmP;
	int			i;

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
color.alpha *= fIntensity;
if (color.alpha < 0.01f)
	return;
glEnable (GL_TEXTURE_2D);
bmP = bAdditive ? bmpGlare : bmpCorona;
if (bmP->Bind (1, -1))
	return;
bmP->Texture ()->Wrap (GL_CLAMP);
glDisable (GL_CULL_FACE);
if (bAdditive) {
	fLight *= color.alpha;
	glBlendFunc (GL_ONE, GL_ONE);
	}
glColor4fv (reinterpret_cast<GLfloat*> (&color));
glBegin (GL_QUADS);
for (i = 0; i < 4; i++) {
	glTexCoord2fv (reinterpret_cast<GLfloat*> (tcGlare + i));
	glVertex3fv (reinterpret_cast<GLfloat*> (sprite + i));
	}
glEnd ();
if (bAdditive)
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glEnable (GL_CULL_FACE);
RenderCoronaOutline (sprite, vCenter);
}

// -----------------------------------------------------------------------------------

float ComputeSoftGlare (CFloatVector *sprite, CFloatVector *vLight, CFloatVector *vEye)
{
	CFloatVector 		n, e, s, t, u, v;
	float 		ul, vl, h, cosine;
	int 			i;

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
return (float) sqrt (cosine) * coronaIntensities [gameOpts->render.coronas.nIntensity];
}

// -----------------------------------------------------------------------------------

void RenderSoftGlare (CFloatVector *sprite, CFloatVector *vCenter, int nTexture, float fIntensity, int bAdditive, int bColored)
{
	tRgbaColorf color;
	tTexCoord2f	tcGlare [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};
	int 			i;
	CBitmap	*bmP = NULL;

if (gameStates.render.bQueryCoronas) {
	glDisable (GL_TEXTURE_2D);
	glBlendFunc (GL_ONE, GL_ZERO);
	}
else {
	glEnable (GL_TEXTURE_2D);
	glDepthFunc (GL_ALWAYS);
	if (bAdditive)
		glBlendFunc (GL_ONE, GL_ONE);
	bmP = bAdditive ? bmpGlare : bmpCorona;
	}
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
if (G3EnableClientStates (gameStates.render.bQueryCoronas == 0, 0, 0, GL_TEXTURE0)) {
	if (gameStates.render.bQueryCoronas == 0) {
		bmP->Bind (1, -1);
		glTexCoordPointer (2, GL_FLOAT, 0, tcGlare);
		}
	glVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), sprite);
	glDrawArrays (GL_QUADS, 0, 4);
	G3DisableClientStates (gameStates.render.bQueryCoronas == 0, 0, 0, GL_TEXTURE0);
	}
else {
	if (gameStates.render.bQueryCoronas == 0)
		bmP->Bind (1, -1);
	glBegin (GL_QUADS);
	for  (i = 0; i < 4; i++) {
		glTexCoord2fv (reinterpret_cast<GLfloat*> (tcGlare + i));
		glVertex3fv (reinterpret_cast<GLfloat*> (sprite + i));
		}
	glEnd ();
	}
if (!gameStates.render.bQueryCoronas && bAdditive)
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
RenderCoronaOutline (sprite, vCenter);
if (gameStates.render.bQueryCoronas != 2) {
	glEnable (GL_DEPTH_TEST);
	if (gameStates.render.bQueryCoronas == 1)
		glDepthFunc (GL_LEQUAL);
	}
}

// -----------------------------------------------------------------------------------

void RenderCorona (short nSegment, short nSide, float fIntensity, float fSize)
{
	CFloatVector		sprite [4], vNormal, vCenter = CFloatVector::ZERO, vEye = CFloatVector::ZERO;
	int			nTexture, bAdditive;
	tIntervalf	zRange;
	float			fAngle, fLight;

if (fIntensity < 0.01f)
	return;
#if 0//def _DEBUG
if ((nSegment != 6) || (nSide != 2))
	return;
#endif
if (! (nTexture = FaceHasCorona (nSegment, nSide, &bAdditive, &fIntensity)))
	return;
#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
fLight = ComputeCoronaSprite (sprite, &vCenter, nSegment, nSide);
if (RENDERPATH && gameStates.ogl.bOcclusionQuery && CoronaStyle ()) {
	fIntensity *= ComputeSoftGlare (sprite, &vCenter, &vEye);
#if 1
	if (gameStates.ogl.bUseDepthBlending && !gameStates.render.automap.bDisplay && (CoronaStyle () == 2)) {
		fSize *= 2;
		if (fSize < 1)
			fSize = 1;
		else if (fSize > 20)
			fSize = 20;
		glUniform1f (glGetUniformLocation (hGlareShader, "dMax"), (GLfloat) fSize);
		}
#endif
	RenderSoftGlare (sprite, &vCenter, nTexture, fIntensity, bAdditive,
						  !gameStates.render.automap.bDisplay || gameData.render.mine.bAutomapVisited [nSegment]);
	glDepthFunc (GL_LESS);
#if DBG
	if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif
	}
else {
	vNormal = CFloatVector::Normal (sprite [0], sprite [1], sprite [2]);
	vEye = vCenter; CFloatVector::Normalize (vEye);
	//dim corona depending on viewer angle
	if ((fAngle = CFloatVector::Dot (vNormal, vEye)) > 0) {
		if (fAngle > 0.25f)
			return;
		fIntensity *= 1 - fAngle / 0.25f;
		}
	//move corona in towards viewer
	fIntensity *= coronaIntensities [gameOpts->render.coronas.nIntensity];
	if (0 == (fIntensity = MoveSpriteIn (sprite, &vCenter, &zRange, fIntensity)))
		return;
	ComputeHardGlare (sprite, &vCenter, &vNormal);
	RenderHardGlare (sprite, &vCenter, nTexture, fLight, fIntensity, &zRange, 0,	//bAdditive
						  !gameStates.render.automap.bDisplay || gameData.render.mine.bAutomapVisited [nSegment]);
	}
}

//------------------------------------------------------------------------------

float CoronaVisibility (int nQuery)
{
	GLuint	nSamples = 0;
	GLint		bAvailable = 0;
	int		nAttempts = 2;
	float		fIntensity;
#if DBG
	GLint		nError;
#endif

if (! (gameStates.ogl.bOcclusionQuery && nQuery) || (CoronaStyle () != 1))
	return 1;
if (! (gameStates.render.bQueryCoronas || gameData.render.lights.coronaSamples [nQuery - 1]))
	return 0;
for (;;) {
	glGetQueryObjectiv (gameData.render.lights.coronaQueries [nQuery - 1], GL_QUERY_RESULT_AVAILABLE_ARB, &bAvailable);
	if (glGetError ()) {
#if DBG
		glGetQueryObjectiv (gameData.render.lights.coronaQueries [nQuery - 1], GL_QUERY_RESULT_AVAILABLE_ARB, &bAvailable);
		if (nError = glGetError ())
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

void LoadGlareShader (float dMax)
{
	static float dMaxPrev = -1;

OglClearError (0);
gameStates.ogl.bUseDepthBlending = 0;
if (gameStates.ogl.bDepthBlending) {
	OglSetReadBuffer (GL_BACK, 1);
	if (CopyDepthTexture ()) {
		gameStates.ogl.bUseDepthBlending = 1;
		if (dMax < 1)
			dMax = 1;
		if (gameStates.render.history.nShader != 999) {
			glUseProgramObject (hGlareShader);
			gameStates.render.history.nShader = 999;
			glUniform1i (glGetUniformLocation (hGlareShader, "glareTex"), 0);
			glUniform1i (glGetUniformLocation (hGlareShader, "depthTex"), 1);
			glUniform2fv (glGetUniformLocation (hGlareShader, "screenScale"), 1, reinterpret_cast<GLfloat*> (&gameData.render.ogl.screenScale));
#if 0
			if (gameStates.render.automap.bDisplay)
				glUniform3fv (glGetUniformLocation (h, "depthScale"), 1, reinterpret_cast<GLfloat*> (&gameData.render.ogl.depthScale));
			else 
#endif
				{
#if 1
				//glUniform1f (glGetUniformLocation (h, "depthScale"), (GLfloat) (gameData.render.ogl.depthScale [Z]));
#else
				glUniform1f (glGetUniformLocation (h, "depthScale"), (GLfloat) X2F (gameData.render.zMax) - gameData.render.ogl.zNear);
#endif
				glUniform1f (glGetUniformLocation (hGlareShader, "dMax"), (GLfloat) dMax);
				}
			gameData.render.nShaderChanges++;
			}
		else {
			if (dMaxPrev != dMax) {
				glUniform1f (glGetUniformLocation (hGlareShader, "dMax"), (GLfloat) dMax);
				}
			}
		dMaxPrev = dMax;
		glDisable (GL_DEPTH_TEST);
		}
	glActiveTexture (GL_TEXTURE0);
	}
}

//-------------------------------------------------------------------------

void UnloadGlareShader (void)
{
if (gameStates.ogl.bDepthBlending) {
	glUseProgramObject (0);
	gameStates.render.history.nShader = -1;
	//DestroyGlareDepthTexture ();
	glEnable (GL_TEXTURE_2D);
	glActiveTexture (GL_TEXTURE1);
	glBindTexture (GL_TEXTURE_2D, 0);
	glActiveTexture (GL_TEXTURE2);
	glBindTexture (GL_TEXTURE_2D, 0);
	glActiveTexture (GL_TEXTURE0);
	glEnable (GL_DEPTH_TEST);
	}
}

//------------------------------------------------------------------------------

const char *glareFS =
	"uniform sampler2D glareTex, depthTex;\r\n" \
	"uniform float dMax;\r\n" \
	"uniform vec2 screenScale;\r\n" \
	"#define ZNEAR 1.0\r\n" \
	"#define ZFAR 5000.0\r\n" \
	"#define ZRANGE (ZFAR - ZNEAR)\r\n" \
	"#define HOM(_x) ZFAR / (ZFAR - (_x) * ZRANGE)\r\n" \
	"void main (void) {\r\n" \
	"float texZ = HOM (texture2D (depthTex, screenScale * gl_FragCoord.xy).r);\r\n" \
	"float fragZ = HOM (gl_FragCoord.z);\r\n" \
	"float dz = clamp (fragZ - texZ, 0.0, dMax);\r\n" \
	"dz = (dMax - dz) / dMax;\r\n" \
	"vec4 texColor = texture2D (glareTex, gl_TexCoord [0].xy);\r\n" \
	"gl_FragColor = vec4 (texColor.rgb * gl_Color.rgb, texColor.a * gl_Color.a * dz);\r\n" \
	"}\r\n";

const char *glareVS =
	"void main (void){\r\n" \
	"gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"gl_Position = ftransform () /*gl_ModelViewProjectionMatrix * gl_Vertex*/;\r\n" \
	"gl_FrontColor = gl_Color;}\r\n";

//-------------------------------------------------------------------------

void InitGlareShader (void)
{
	int	bOk;

gameStates.ogl.bDepthBlending = 0;
#if SHADER_SOFT_CORONAS
PrintLog ("building corona blending shader program\n");
DeleteShaderProg (NULL);
if (gameStates.ogl.bRender2TextureOk && gameStates.ogl.bShadersOk && RENDERPATH) {
	gameStates.ogl.bDepthBlending = 1;
	if (hGlareShader)
		DeleteShaderProg (&hGlareShader);
	bOk = CreateShaderProg (&hGlareShader) &&
			CreateShaderFunc (&hGlareShader, &hGlareFS, &hGlareVS, glareFS, glareVS, 1) &&
			LinkShaderProg (&hGlareShader);
	if (!bOk) {
		gameStates.ogl.bDepthBlending = 0;
		DeleteShaderProg (&hGlareShader);
		return;
		}
	OglClearError (0);
	}
#endif
}

//------------------------------------------------------------------------------
// eof
