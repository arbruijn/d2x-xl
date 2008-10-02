/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

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
#ifdef _DEBUG
#	define SHADER_SOFT_CORONAS 1
#else
#	define SHADER_SOFT_CORONAS 1
#endif

float coronaIntensities [] = {0.25f, 0.5f, 0.75f, 1};

GLhandleARB hGlareShader [2] = {0,0};
GLhandleARB hGlareVS [2] = {0,0};
GLhandleARB hGlareFS [2] = {0,0};

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

#ifdef _DEBUG
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
		glCopyTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 0, 0, grdCurScreen->scWidth, grdCurScreen->scHeight, 0);
#else
		glCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 0, 0, grdCurScreen->scWidth, grdCurScreen->scHeight);
#endif
		if (nError = glGetError ()) {
			glCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 0, 0, grdCurScreen->scWidth, grdCurScreen->scHeight);
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

void CalcSpriteCoords (fVector *vSprite, fVector *vCenter, fVector *vEye, float dx, float dy, fMatrix *r)
{
	fVector	v, h, vdx, vdy;
	float		d = vCenter->SqrMag();
	int		i;

if (!vEye) {
	vEye = &h;
	//fVector::Normalize(vEye, vCenter);
	*vEye = *vCenter; fVector::Normalize(*vEye);
	}
v[X] = v[Z] = 0;
v[Y] = (*vCenter)[Y] ? d / (*vCenter)[Y] : 1;
v -= *vCenter;
fVector::Normalize(v);
vdx = fVector::Cross(v, *vEye);	//orthogonal vector in plane through face center and perpendicular to viewer
vdx = vdx * dx;
v[Y] = v[Z] = 0;
v[X] = (*vCenter)[X] ? d / (*vCenter)[X] : 1;
v -= *vCenter;
fVector::Normalize(v);
vdy = fVector::Cross(v, *vEye);
if (r) {
	if((*vCenter)[X] >= 0) {
		vdy = vdy * dy;
		v[X] = +vdx[X] + vdy[X];
		v[Y] = +vdx[Y] + vdy[Y];
		v[Z] = -vdx[Z] - vdy[Z];
		*vSprite = *r * v;
		vSprite[0] += *vCenter;
		v[X] = v[Y] = v[Z] = 0;
		v[X] = -vdx[X] + vdy[X];
		v[Y] = +vdx[Y] + vdy[Y];
		v[Z] = +vdx[Z] - vdy[Z];
		vSprite[1] = *r * v;
		vSprite[1] += *vCenter;
		v[X] = v[Y] = v[Z] = 0;
		v[X] = -vdx[X] - vdy[X];
		v[Y] = -vdx[Y] - vdy[Y];
		v[Z] = +vdx[Z] + vdy[Z];
		vSprite[2] =  *r * v;
		vSprite[2] += *vCenter;
		v[X] = +vdx[X] - vdy[X];
		v[Y] = -vdx[Y] - vdy[Y];
		v[Z] = -vdx[Z] + vdy[Z];
		vSprite[3] = *r * v;
		vSprite[3] += *vCenter;
		}
	else {
		vdy = vdy * dy;
		v[X] = -vdx[X] - vdy[X];
		v[Y] = -vdx[Y] - vdy[Y];
		v[Z] = -vdx[Z] - vdy[Z];
		*vSprite = *r * v;
		vSprite[0] += *vCenter;
		v[X] = v[Y] = v[Z] = 0;
		v[X] = +vdx[X] - vdy[X];
		v[Y] = -vdx[Y] - vdy[Y];
		v[Z] = +vdx[Z] - vdy[Z];
		vSprite[1] = *r * v;
		vSprite[1] += *vCenter;
		v[X] = v[Y] = v[Z] = 0;
		v[X] = +vdx[X] + vdy[X];
		v[Y] = +vdx[Y] + vdy[Y];
		v[Z] = +vdx[Z] + vdy[Z];
		vSprite[2] = *r * v;
		vSprite[2] += *vCenter;
		v[X] = -vdx[X] + vdy[X];
		v[Y] = +vdx[Y] + vdy[Y];
		v[Z] = -vdx[Z] + vdy[Z];
		vSprite[3] = *r * v;
		vSprite[3] += *vCenter;
		}
	}
else {
	vSprite [0][X] = -vdx[X] - vdy[X];
	vSprite [0][Y] = -vdx[Y] - vdy[Y];
	vSprite [0][Z] = -vdx[Z] - vdy[Z];
	vSprite [1][X] = +vdx[X] - vdy[X];
	vSprite [1][Y] = +vdx[Y] - vdy[Y];
	vSprite [1][Z] = +vdx[Z] - vdy[Z];
	vSprite [2][X] = +vdx[X] + vdy[X];
	vSprite [2][Y] = +vdx[Y] + vdy[Y];
	vSprite [2][Z] = +vdx[Z] + vdy[Z];
	vSprite [3][X] = -vdx[X] + vdy[X];
	vSprite [3][Y] = -vdx[Y] + vdy[Y];
	vSprite [3][Z] = -vdx[Z] + vdy[Z];
	for (i = 0; i < 4; i++)
		vSprite[i] += *vCenter;
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
	d = vmsVector::Dist(gameData.segs.vertices[pSideVerts[j]], gameData.segs.vertices[pSideVerts[(j + 1) % 4]]);
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
d1 = VmLinePointDist(gameData.segs.vertices[pSideVerts[i]],
                     gameData.segs.vertices[pSideVerts[j]],
                     gameData.segs.vertices[pSideVerts[(j+1)%4]]);
d = vmsVector::Dist(gameData.segs.vertices[pSideVerts[i]], gameData.segs.vertices[pSideVerts [(j + 1) % 4]]);
d = vmsVector::Dist(gameData.segs.vertices[pSideVerts[j]], gameData.segs.vertices[pSideVerts [(j + 1) % 4]]);
d2 = VmLinePointDist(gameData.segs.vertices[pSideVerts[i]],
                     gameData.segs.vertices[pSideVerts[j]],
                     gameData.segs.vertices[pSideVerts[(j+2)%4]]);
d = vmsVector::Dist(gameData.segs.vertices[pSideVerts[i]], gameData.segs.vertices[pSideVerts [(j + 2) % 4]]);
d = vmsVector::Dist(gameData.segs.vertices[pSideVerts[j]], gameData.segs.vertices[pSideVerts [(j + 2) % 4]]);
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
#ifdef _DEBUG
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
if (sideP->nOvlTex && (0 < (nBrightness = IsLight (sideP->nOvlTex)))) {
	nTexture = sideP->nOvlTex;
	bAdditive = gameOpts->render.coronas.bAdditive;
	}
else if ((nBrightness = IsLight (sideP->nBaseTex))) {
	nTexture = sideP->nBaseTex;
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
pszName = gameData.pig.tex.bitmapFiles [gameStates.app.bD1Mission][gameData.pig.tex.pBmIndex [nTexture].index].name;
if (strstr (pszName, "metl") || strstr (pszName, "rock") || strstr (pszName, "water"))
	return 0;
if (bAdditiveP)
	*bAdditiveP = bAdditive;
return nTexture;
}

// -----------------------------------------------------------------------------------

float ComputeCoronaSprite (fVector *sprite, fVector *vCenter, short nSegment, short nSide)
{
	tSide		*sideP = gameData.segs.segments [nSegment].sides + nSide;
	short		sideVerts [4];
	int		i;
	float		fLight = 0;
	fVector	v;

GetSideVertIndex (sideVerts, nSegment, nSide);
for (i = 0; i < 4; i++) {
	fLight += X2F (sideP->uvls [i].l);
	if (RENDERPATH)
		G3TransformPoint(sprite[i], gameData.segs.fVertices[sideVerts[i]], 0);
	else
		sprite [i] = gameData.segs.fVertices [sideVerts [i]];	//already transformed
	}
v = SIDE_CENTER_I (nSegment, nSide)->ToFloat();
G3TransformPoint(*vCenter, v, 0);
#if 0
if (gameStates.render.bQueryCoronas) {
	for (i = 0; i < 4; i++) {
		v = sprite[i] - *vCenter;
		VmVecScaleAdd (sprite + i, vCenter, &v, 0.5f);
	}
}
#endif
return fLight;
}

// -----------------------------------------------------------------------------------

void ComputeSpriteZRange (fVector *sprite, tIntervalf *zRangeP)
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

float MoveSpriteIn (fVector *sprite, fVector *vCenter, tIntervalf *zRangeP, float fIntensity)
{
	tIntervalf	zRange;

ComputeSpriteZRange (sprite, &zRange);
if (zRange.fMin > 0)
	*vCenter = *vCenter * (zRange.fMin / (*vCenter)[Z]);
else {
	if (zRange.fMin < -zRange.fRad)
		return 0;
	fIntensity *= 1 + zRange.fMin / zRange.fRad * 2;
	(*vCenter)[X] /= (*vCenter)[Z];
	(*vCenter)[Y] /= (*vCenter)[Y];
	(*vCenter)[Z] = 1;
	}
*zRangeP = zRange;
return fIntensity;
}

// -----------------------------------------------------------------------------------

void ComputeHardGlare (fVector *sprite, fVector *vCenter, fVector *vNormal)
{
	fVector	u, v, p, q, e, s, t;
	float		h, g;

u = sprite[2] + sprite[1];
u -= sprite[0];
u -= sprite[3];
u = u * 0.25f;
v = sprite[0] + sprite[1];
v -= sprite[2];
v -= sprite[3];
v = v * 0.25f;
*vNormal = fVector::Cross(v, u);
fVector::Normalize(*vNormal);
e = *vCenter; fVector::Normalize(e);
if (fVector::Dot(e, *vNormal) > 0.999f)
	p = v;
else {
	p = fVector::Cross(e, *vNormal);
	fVector::Normalize(p);
}

q = fVector::Cross(p, e);
h = u.Mag();
g = v.Mag();
if (h > g)
	h = g;
g = 2 * (float) (fabs (fVector::Dot(p, v)) + fabs (fVector::Dot(p, u))) + h * fVector::Dot(p, *vNormal);
h = 2 * (float) (fabs (fVector::Dot(q, v)) + fabs (fVector::Dot(q, u))) + h * fVector::Dot(q, *vNormal);
#if 1
if (g / h > 8)
	h = g / 8;
else if (h / g > 8)
	g = h / 8;
#endif
s = p * g;
t = q * h;

sprite[0] = *vCenter + s;
sprite[1] = sprite[0];
sprite[0] += t;
sprite[1] -= t;
sprite[3] = *vCenter - s;
sprite[2] = sprite[3];
sprite[3] += t;
sprite[2] -= t;
}

// -----------------------------------------------------------------------------------

#if defined(_DEBUG) && CORONA_OUTLINE

void RenderCoronaOutline (fVector *sprite, fVector *vCenter)
{
	int	i;

glDisable (GL_TEXTURE_2D);
glColor4d (1,1,1,1);
glLineWidth (2);
glBegin (GL_LINE_LOOP);
for (i = 0; i < 4; i++)
	glVertex3fv ((GLfloat *) (sprite + i));
glEnd ();
glBegin (GL_LINES);
vCenter->x() += 5;
glVertex3fv ((GLfloat *) &vCenter);
vCenter->x() -= 10;
glVertex3fv ((GLfloat *) &vCenter);
vCenter->x() += 5;
vCenter->y() += 5;
glVertex3fv ((GLfloat *) &vCenter);
vCenter->y() -= 10;
glVertex3fv ((GLfloat *) &vCenter);
glEnd ();
glLineWidth (1);
}

#else

#	define RenderCoronaOutline(_sprite,_center)

#endif

// -----------------------------------------------------------------------------------

void RenderHardGlare (fVector *sprite, fVector *vCenter, int nTexture, float fLight,
							 float fIntensity, tIntervalf *zRangeP, int bAdditive, int bColored)
{
	tTexCoord2f	tcGlare [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};
	tRgbaColorf	color;
	grsBitmap	*bmP;
	int			i;

fLight /= 4;
if (fLight < 0.01f)
	return;
color.alpha = vCenter->Mag();
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
if (OglBindBmTex (bmP, 1, -1))
	return;
OglTexWrap (bmP->glTexture, GL_CLAMP);
glDisable (GL_CULL_FACE);
if (bAdditive) {
	fLight *= color.alpha;
	glBlendFunc (GL_ONE, GL_ONE);
	}
glColor4fv ((GLfloat *) &color);
glBegin (GL_QUADS);
for (i = 0; i < 4; i++) {
	glTexCoord2fv ((GLfloat *) (tcGlare + i));
	glVertex3fv ((GLfloat *) (sprite + i));
	}
glEnd ();
if (bAdditive)
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glEnable (GL_CULL_FACE);
RenderCoronaOutline (sprite, vCenter);
}

// -----------------------------------------------------------------------------------

float ComputeSoftGlare (fVector *sprite, fVector *vLight, fVector *vEye)
{
	fVector 		n, e, s, t, u, v;
	float 		ul, vl, h, cosine;
	int 			i;

u = sprite[2] + sprite[1];
u -= sprite[0];
u -= sprite[3];
u = u * 0.25f;
v = sprite[0] + sprite[1];
v -= sprite[2];
v -= sprite[3];
v = v * 0.25f;
e = *vEye - *vLight;
fVector::Normalize(e);
n = fVector::Cross(v, u);
fVector::Normalize(n);
ul = u.Mag();
vl = v.Mag();
h = (ul > vl) ? vl : ul;
s = u + n * (-h * fVector::Dot(e, u) / ul);
t = v + n * (-h * fVector::Dot(e, v) / vl);
s = s + e * fVector::Dot(e, s);
t = t + e * fVector::Dot(e, t);
s = s * 1.8f;
t = t * 1.8f;
v = *vLight;
for (i = 0; i < 3; i++) {
	sprite[0][i] = v[i] + s[i] + t[i];
	sprite[1][i] = v[i] + s[i] - t[i];
	sprite[2][i] = v[i] - s[i] - t[i];
	sprite[3][i] = v[i] - s[i] + t[i];
	}
cosine = fVector::Dot(e, n);
return (float) sqrt (cosine) * coronaIntensities [gameOpts->render.coronas.nIntensity];
}

// -----------------------------------------------------------------------------------

void RenderSoftGlare (fVector *sprite, fVector *vCenter, int nTexture, float fIntensity, int bAdditive, int bColored)
{
	tRgbaColorf color;
	tTexCoord2f	tcGlare [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};
	int 			i;
	grsBitmap	*bmP = NULL;

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
if (gameStates.render.bQueryCoronas != 2) {
	glDisable (GL_DEPTH_TEST);
#if 0
	if (gameStates.render.bQueryCoronas == 1)
		glDepthFunc (GL_ALWAYS);
#endif
	}
if (G3EnableClientStates (gameStates.render.bQueryCoronas == 0, 0, 0, GL_TEXTURE0)) {
	if (gameStates.render.bQueryCoronas == 0) {
		OglBindBmTex (bmP, 1, -1);
		glTexCoordPointer (2, GL_FLOAT, 0, tcGlare);
		}
	glVertexPointer (3, GL_FLOAT, sizeof (fVector), sprite);
	glDrawArrays (GL_QUADS, 0, 4);
	G3DisableClientStates (gameStates.render.bQueryCoronas == 0, 0, 0, GL_TEXTURE0);
	}
else {
	if (gameStates.render.bQueryCoronas == 0)
		OglBindBmTex (bmP, 1, -1);
	glBegin (GL_QUADS);
	for  (i = 0; i < 4; i++) {
		glTexCoord2fv ((GLfloat *) (tcGlare + i));
		glVertex3fv ((GLfloat *) (sprite + i));
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
	fVector		sprite [4], vNormal, vCenter = fVector::ZERO, vEye = fVector::ZERO;
	int			nTexture, bAdditive;
	tIntervalf	zRange;
	float			fAngle, fLight;

if (fIntensity < 0.01f)
	return;
#if 0//def _DEBUG
if ((nSegment != 6) || (nSide != 2))
	return;
#endif
if (!(nTexture = FaceHasCorona (nSegment, nSide, &bAdditive, &fIntensity)))
	return;
#ifdef _DEBUG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
fLight = ComputeCoronaSprite (sprite, &vCenter, nSegment, nSide);
if (RENDERPATH && gameStates.ogl.bOcclusionQuery && (CoronaStyle ())) {
	fIntensity *= ComputeSoftGlare (sprite, &vCenter, &vEye);
#if 1
	if (gameStates.ogl.bUseDepthBlending && !gameStates.render.automap.bDisplay) {
		fSize *= 2;
		if (fSize < 1)
			fSize = 1;
		else if (fSize > 20)
			fSize = 20;
		glUniform1f (glGetUniformLocation (hGlareShader [0], "dMax"), (GLfloat) fSize);
		}
#endif
	RenderSoftGlare (sprite, &vCenter, nTexture, fIntensity, bAdditive,
						  !gameStates.render.automap.bDisplay || gameData.render.mine.bAutomapVisited [nSegment]);
	glDepthFunc (GL_LESS);
#ifdef _DEBUG
	if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif
	}
else {
	vNormal = fVector::Normal(sprite[0], sprite[1], sprite[2]);
	vEye = vCenter; fVector::Normalize(vEye);
	//dim corona depending on viewer angle
	if ((fAngle = fVector::Dot(vNormal, vEye)) > 0) {
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
	float		fIntensity;
#ifdef _DEBUG
	GLint		nError;
#endif

if (!(gameStates.ogl.bOcclusionQuery && nQuery) || (CoronaStyle () != 1))
	return 1;
if (!(gameStates.render.bQueryCoronas || gameData.render.lights.coronaSamples [nQuery - 1]))
	return 1;
do {
	glGetQueryObjectiv (gameData.render.lights.coronaQueries [nQuery - 1], GL_QUERY_RESULT_AVAILABLE_ARB, &bAvailable);
	if (glGetError ()) {
#ifdef _DEBUG
		glGetQueryObjectiv (gameData.render.lights.coronaQueries [nQuery - 1], GL_QUERY_RESULT_AVAILABLE_ARB, &bAvailable);
		if (nError = glGetError ())
#endif
			return 1;
		}
	} while (!bAvailable);
glGetQueryObjectuiv (gameData.render.lights.coronaQueries [nQuery - 1], GL_QUERY_RESULT_ARB, &nSamples);
if (glGetError ())
	return 1;
if (gameStates.render.bQueryCoronas == 1) {
#ifdef _DEBUG
	if (!nSamples) {
		GLint nBits;
		glGetQueryiv (GL_SAMPLES_PASSED, GL_QUERY_COUNTER_BITS, &nBits);
		}
#endif
	return (float) (gameData.render.lights.coronaSamples [nQuery - 1] = nSamples);
	}
fIntensity = (float) nSamples / (float) gameData.render.lights.coronaSamples [nQuery - 1];
#ifdef _DEBUG
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
	OglReadBuffer (GL_BACK, 1);
	if (CopyDepthTexture ()) {
		gameStates.ogl.bUseDepthBlending = 1;
		GLhandleARB	h = hGlareShader [0]; //gameStates.render.automap.bDisplay];
		if (gameStates.render.history.nShader != 999) {
			glUseProgramObject (h);
			gameStates.render.history.nShader = 999;
			glUniform1i (glGetUniformLocation (h, "glareTex"), 0);
			glUniform1i (glGetUniformLocation (h, "depthTex"), 1);
			glUniform2fv (glGetUniformLocation (h, "screenScale"), 1, (GLfloat *) &gameData.render.ogl.screenScale);
			if (gameStates.render.automap.bDisplay)
				glUniform3fv (glGetUniformLocation (h, "depthScale"), 1, (GLfloat *) &gameData.render.ogl.depthScale);
			else {
				glUniform1f (glGetUniformLocation (h, "depthScale"), (GLfloat) (gameData.render.ogl.depthScale [Z]));
				glUniform1f (glGetUniformLocation (h, "dMax"), (GLfloat) dMax);
				}
			gameData.render.nShaderChanges++;
			}
		else {
			if (!gameStates.render.automap.bDisplay && (dMaxPrev != dMax)) {
				glUniform1f (glGetUniformLocation (h, "dMax"), (GLfloat) dMax);
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

const char *glareFS [2] = {
	"uniform sampler2D glareTex;\r\n" \
	"uniform sampler2D depthTex;\r\n" \
	"uniform float depthScale;\r\n" \
	"uniform vec2 screenScale;\r\n" \
	"uniform float dMax;\r\n" \
	"void main (void) {\r\n" \
	"float dz = (gl_FragCoord.z - texture2D (depthTex, screenScale * gl_FragCoord.xy).r) * depthScale;\r\n" \
	"dz = clamp (dz, 0.0, dMax);\r\n" \
	"dz = ((dMax - dz) / dMax);\r\n" \
	"/*if (dz < 1.0) gl_FragColor = vec4 (0.0, 0.5, 1.0, 0.5); else*/\n" \
	"gl_FragColor = texture2D (glareTex, gl_TexCoord [0].xy) * gl_Color * dz;\r\n" \
	"}\r\n"
,
	"uniform sampler2D glareTex;\r\n" \
	"uniform sampler2D depthTex;\r\n" \
	"uniform vec3 depthScale;\r\n" \
	"uniform vec2 screenScale;\r\n" \
	"void main (void) {\r\n" \
	"float depthZ = depthScale.y / (depthScale.x - texture2D (depthTex, screenScale * gl_FragCoord.xy).r);\r\n" \
	"float fragZ = depthScale.y / (depthScale.x - gl_FragCoord.z);\r\n" \
	"gl_FragColor = texture2D (glareTex, gl_TexCoord [0].xy) * gl_Color / sqrt (max (1.0, (depthZ - fragZ) / 2.0));\r\n" \
	"}\r\n"
	};

const char *glareVS =
	"void main(void){\r\n" \
	"gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"gl_Position = ftransform() /*gl_ModelViewProjectionMatrix * gl_Vertex*/;\r\n" \
	"gl_FrontColor = gl_Color;}\r\n";

//-------------------------------------------------------------------------

void InitGlareShader (void)
{
	int	i, bOk;

gameStates.ogl.bDepthBlending = 0;
#if SHADER_SOFT_CORONAS
PrintLog ("building corona blending shader program\n");
DeleteShaderProg (NULL);
if (gameStates.ogl.bRender2TextureOk && gameStates.ogl.bShadersOk && RENDERPATH) {
	gameStates.ogl.bDepthBlending = 1;
	for (i = 0; i < 2; i++) {
		if (hGlareShader [i])
			DeleteShaderProg (hGlareShader + i);
		bOk = CreateShaderProg (hGlareShader + i) &&
				CreateShaderFunc (hGlareShader + i, hGlareFS + i, hGlareVS + i, glareFS [i], glareVS, 1) &&
				LinkShaderProg (hGlareShader + i);
		if (!bOk) {
			gameStates.ogl.bDepthBlending = 0;
			while (--i)
				DeleteShaderProg (hGlareShader + i);
			return;
			}
		}
	OglClearError (0);
	}
#endif
}

//------------------------------------------------------------------------------
// eof
