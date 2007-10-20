/* $Id: render.c, v 1.18 2003/10/10 09:36:35 btb Exp $ */
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
#include "gr.h"
#include "gameseg.h"
#include "lighting.h"
#include "lightmap.h"
#include "renderlib.h"
#include "network.h"
#include "glare.h"

#define CORONA_OUTLINE	0

float coronaIntensities [] = {0.25f, 0.5f, 0.75f, 1};

// -----------------------------------------------------------------------------------

void CalcSpriteCoords (fVector *vSprite, fVector *vCenter, fVector *vEye, float dx, float dy, fMatrix *r)
{
	fVector	v, h, vdx, vdy;
	float		d = vCenter->p.x * vCenter->p.x + vCenter->p.y * vCenter->p.y + vCenter->p.z * vCenter->p.z;
	int		i;

if (!vEye) {
	vEye = &h;
	VmVecNormalizef (vEye, vCenter);
	}
v.p.x = v.p.z = 0;
v.p.y = vCenter->p.y ? d / vCenter->p.y : 1;
VmVecDecf (&v, vCenter);
VmVecNormalizef (&v, &v);
VmVecCrossProdf (&vdx, &v, vEye);	//orthogonal vector in plane through face center and perpendicular to viewer
VmVecScalef (&vdx, &vdx, dx);
v.p.y = v.p.z = 0;
v.p.x = vCenter->p.x ? d / vCenter->p.x : 1;
VmVecDecf (&v, vCenter);
VmVecNormalizef (&v, &v);
VmVecCrossProdf (&vdy, &v, vEye);
if (r) {
	if (vCenter->p.x >= 0) {
		VmVecScalef (&vdy, &vdy, dy);
		v.p.x = +vdx.p.x + vdy.p.x;
		v.p.y = +vdx.p.y + vdy.p.y;
		v.p.z = -vdx.p.z - vdy.p.z;
		VmVecRotatef (vSprite, &v, r);
		VmVecIncf (vSprite, vCenter);
		v.p.x = v.p.y = v.p.z = 0;
		v.p.x = -vdx.p.x + vdy.p.x;
		v.p.y = +vdx.p.y + vdy.p.y;
		v.p.z = +vdx.p.z - vdy.p.z;
		VmVecRotatef (vSprite + 1, &v, r);
		VmVecIncf (vSprite + 1, vCenter);
		v.p.x = v.p.y = v.p.z = 0;
		v.p.x = -vdx.p.x - vdy.p.x;
		v.p.y = -vdx.p.y - vdy.p.y;
		v.p.z = +vdx.p.z + vdy.p.z;
		VmVecRotatef (vSprite + 2, &v, r);
		VmVecIncf (vSprite + 2, vCenter);
		v.p.x = +vdx.p.x - vdy.p.x;
		v.p.y = -vdx.p.y - vdy.p.y;
		v.p.z = -vdx.p.z + vdy.p.z;
		VmVecRotatef (vSprite + 3, &v, r);
		VmVecIncf (vSprite + 3, vCenter);
		}
	else {
		VmVecScalef (&vdy, &vdy, dy);
		v.p.x = -vdx.p.x - vdy.p.x;
		v.p.y = -vdx.p.y - vdy.p.y;
		v.p.z = -vdx.p.z - vdy.p.z;
		VmVecRotatef (vSprite, &v, r);
		VmVecIncf (vSprite, vCenter);
		v.p.x = v.p.y = v.p.z = 0;
		v.p.x = +vdx.p.x - vdy.p.x;
		v.p.y = -vdx.p.y - vdy.p.y;
		v.p.z = +vdx.p.z - vdy.p.z;
		VmVecRotatef (vSprite + 1, &v, r);
		VmVecIncf (vSprite + 1, vCenter);
		v.p.x = v.p.y = v.p.z = 0;
		v.p.x = +vdx.p.x + vdy.p.x;
		v.p.y = +vdx.p.y + vdy.p.y;
		v.p.z = +vdx.p.z + vdy.p.z;
		VmVecRotatef (vSprite + 2, &v, r);
		VmVecIncf (vSprite + 2, vCenter);
		v.p.x = -vdx.p.x + vdy.p.x;
		v.p.y = +vdx.p.y + vdy.p.y;
		v.p.z = -vdx.p.z + vdy.p.z;
		VmVecRotatef (vSprite + 3, &v, r);
		VmVecIncf (vSprite + 3, vCenter);
		}
	}
else {
	vSprite [0].p.x = -vdx.p.x - vdy.p.x;
	vSprite [0].p.y = -vdx.p.y - vdy.p.y;
	vSprite [0].p.z = -vdx.p.z - vdy.p.z;
	vSprite [1].p.x = +vdx.p.x - vdy.p.x;
	vSprite [1].p.y = +vdx.p.y - vdy.p.y;
	vSprite [1].p.z = +vdx.p.z - vdy.p.z;
	vSprite [2].p.x = +vdx.p.x + vdy.p.x;
	vSprite [2].p.y = +vdx.p.y + vdy.p.y;
	vSprite [2].p.z = +vdx.p.z + vdy.p.z;
	vSprite [3].p.x = -vdx.p.x + vdy.p.x;
	vSprite [3].p.y = -vdx.p.y + vdy.p.y;
	vSprite [3].p.z = -vdx.p.z + vdy.p.z;
	for (i = 0; i < 4; i++)
		VmVecIncf (vSprite + i, vCenter);
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
for (j = 0; j < 4; j++) {
	d = VmVecDist (gameData.segs.vertices + pSideVerts [j], gameData.segs.vertices + pSideVerts [(j + 1) % 4]);
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
d1 = VmLinePointDist (gameData.segs.vertices + pSideVerts [i], 
							 gameData.segs.vertices + pSideVerts [j], 
							 gameData.segs.vertices + pSideVerts [(j + 1) % 4]);
d = VmVecDist (gameData.segs.vertices + pSideVerts [i], gameData.segs.vertices + pSideVerts [(j + 1) % 4]);
d = VmVecDist (gameData.segs.vertices + pSideVerts [j], gameData.segs.vertices + pSideVerts [(j + 1) % 4]);
d2 = VmLinePointDist (gameData.segs.vertices + pSideVerts [i], 
							 gameData.segs.vertices + pSideVerts [j], 
							 gameData.segs.vertices + pSideVerts [(j + 2) % 4]);
d = VmVecDist (gameData.segs.vertices + pSideVerts [i], gameData.segs.vertices + pSideVerts [(j + 2) % 4]);
d = VmVecDist (gameData.segs.vertices + pSideVerts [j], gameData.segs.vertices + pSideVerts [(j + 2) % 4]);
if (h)
	*h = d1 > d2 ? d1 : d2;
return i;
}

// -----------------------------------------------------------------------------------

int FaceHasCorona (short nSegment, short nSide, int *bAdditiveP, float *fIntensityP)
{
	ushort		nWall;
	tSide			*sideP;
	int			i, bAdditive, nTexture;

if (IsMultiGame && extraGameInfo [1].bDarkness)
	return 0;
if (gameOpts->render.bDynamicLight) {
	i = FindDynLight (nSegment, nSide, -1);
	if ((i >= 0) && !gameData.render.lights.dynamic.lights [i].bOn)
		return 0;
	}
sideP = gameData.segs.segments [nSegment].sides + nSide;
nWall = sideP->nWall;
if (IS_WALL (nWall)) {
	tWall *wallP = gameData.walls.walls + nWall;
	ubyte nType = wallP->nType;

	if ((nType == WALL_BLASTABLE) || 
		 (nType == WALL_DOOR) ||
		 (nType == WALL_OPEN) ||
		 (nType == WALL_CLOAKED))
		return 0;
	if (wallP->flags & (WALL_BLASTED | WALL_ILLUSION_OFF))
		return 0;
	}
// get and check the corona emitting texture
if (sideP->nOvlTex && IsLight (sideP->nOvlTex)) {
	nTexture = sideP->nOvlTex;
	bAdditive = gameOpts->render.bAdditiveCoronas;
	}
else {
	nTexture = sideP->nBaseTex;
	if (fIntensityP)
		*fIntensityP /= 2;
	bAdditive = 0;
	}
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
		case 420:	//force field
		case 426:	//teleport
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
	float		zMin = 1000000000.0f, zMax = -1000000000.0f;

GetSideVertIndex (sideVerts, nSegment, nSide);
for (i = 0; i < 4; i++) {
	fLight += f2fl (sideP->uvls [i].l);
	if (gameOpts->render.nRenderPath)
		G3TransformPointf (sprite + i, gameData.segs.fVertices + sideVerts [i], 0);
	else
		sprite [i] = gameData.segs.fVertices [sideVerts [i]];	//already transformed
	}
VmsVecToFloat (&v, SIDE_CENTER_I (nSegment, nSide));
G3TransformPointf (vCenter, &v, 0);
if (gameStates.render.bQueryCoronas) {
	for (i = 0; i < 4; i++) {
		VmVecSubf (&v, sprite + i, vCenter);
		VmVecScaleAddf (sprite + i, vCenter, &v, 0.5f);
		}
	}
return fLight;
}

// -----------------------------------------------------------------------------------

void ComputeSpriteZRange (fVector *sprite, tIntervalf *zRangeP)
{
	float			z;
	tIntervalf	zRange = {1000000000.0f, -1000000000.0f};
	int			i;

for (i = 0; i < 4; i++) {
	z = sprite [i].p.z;
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
	VmVecScalef (vCenter, vCenter, zRange.fMin / vCenter->p.z);
else {
	if (zRange.fMin < -zRange.fRad)
		return 0;
	fIntensity *= 1 + zRange.fMin / zRange.fRad * 2;
	vCenter->p.x /= vCenter->p.z;
	vCenter->p.y /= vCenter->p.y;
	vCenter->p.z = 1;
	}
*zRangeP = zRange;
return fIntensity;
}

// -----------------------------------------------------------------------------------

void ComputeHardGlare (fVector *sprite, fVector *vCenter, fVector *vNormal)
{
	fVector	u, v, p, q, e, s, t;
	float		h, g;

VmVecAddf (&u, sprite + 2, sprite + 1);
VmVecDecf (&u, sprite);
VmVecDecf (&u, sprite + 3);
VmVecScalef (&u, &u, 0.25f);
VmVecAddf (&v, sprite, sprite + 1);
VmVecDecf (&v, sprite + 2);
VmVecDecf (&v, sprite + 3);
VmVecScalef (&v, &v, 0.25f);
VmVecNormalizef (vNormal, VmVecCrossProdf (vNormal, &v, &u));
VmVecNormalizef (&e, vCenter);
if (VmVecDotf (&e, vNormal) > 0.999f)
	p = v;
else
	VmVecNormalizef (&p, VmVecCrossProdf (&p, &e, vNormal));
VmVecCrossProdf (&q, &p, &e);
h = VmVecMagf (&u);
g = VmVecMagf (&v);
if (h > g)
	h = g;
g = 2 * (float) (fabs (VmVecDotf (&p, &v)) + fabs (VmVecDotf (&p, &u))) + h * VmVecDotf (&p, vNormal);
h = 2 * (float) (fabs (VmVecDotf (&q, &v)) + fabs (VmVecDotf (&q, &u))) + h * VmVecDotf (&q, vNormal);
#if 1
if (g / h > 8)
	h = g / 8;
else if (h / g > 8)
	g = h / 8;
#endif
VmVecScalef (&s, &p, g);
VmVecScalef (&t, &q, h);

VmVecAddf (sprite, vCenter, &s);
sprite [1] = sprite [0];
VmVecIncf (sprite, &t);
VmVecDecf (sprite + 1, &t);
VmVecSubf (sprite + 3, vCenter, &s);
sprite [2] = sprite [3];
VmVecIncf (sprite + 3, &t);
VmVecDecf (sprite + 2, &t);
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
vCenter->p.x += 5;
glVertex3fv ((GLfloat *) &vCenter);
vCenter->p.x -= 10;
glVertex3fv ((GLfloat *) &vCenter);
vCenter->p.x += 5;
vCenter->p.y += 5;
glVertex3fv ((GLfloat *) &vCenter);
vCenter->p.y -= 10;
glVertex3fv ((GLfloat *) &vCenter);
glEnd ();
glLineWidth (1);
}

#else

#	define RenderCoronaOutline(_sprite,_center)

#endif

// -----------------------------------------------------------------------------------

void RenderHardGlare (fVector *sprite, fVector *vCenter, int nTexture, float fLight, 
							 float fIntensity, tIntervalf *zRangeP, int bAdditive)
{
	tTexCoord2f	tcGlare [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};
	tFaceColor	*pf;
	grsBitmap	*bmP;
	float			a;
	int			i;

fLight /= 4;
if (fLight < 0.01f)
	return;
a = VmVecMagf (vCenter);
if (a < zRangeP->fRad)
	fIntensity *= a / zRangeP->fRad;
pf = gameData.render.color.textures + nTexture;
a = (float) (pf->color.red * 3 + pf->color.green * 5 + pf->color.blue * 2) / 30 * 2;
a *= fIntensity;
if (a < 0.01f)
	return;
glEnable (GL_TEXTURE_2D);
bmP = bAdditive ? bmpGlare : bmpCorona;
if (OglBindBmTex (bmP, 1, -1)) 
	return;
OglTexWrap (bmP->glTexture, GL_CLAMP);
glDisable (GL_CULL_FACE);
if (bAdditive) {
	fLight *= a;
	glBlendFunc (GL_ONE, GL_ONE);
	}
glColor4f (pf->color.red * fLight, pf->color.green * fLight, pf->color.blue * fLight, a);
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

VmVecAddf (&u, sprite + 2, sprite + 1);
VmVecDecf (&u, sprite);
VmVecDecf (&u, sprite + 3);
VmVecScalef (&u, &u, 0.25f);
VmVecAddf (&v, sprite, sprite + 1);
VmVecDecf (&v, sprite + 2);
VmVecDecf (&v, sprite + 3);
VmVecScalef (&v, &v, 0.25f);
VmVecSubf (&e, vEye, vLight);
VmVecNormalizef (&e, &e);
VmVecCrossProdf (&n, &v, &u);
VmVecNormalizef (&n, &n);
ul = VmVecMagf (&u);
vl = VmVecMagf (&v);
h = (ul > vl) ? vl : ul;
VmVecScaleAddf (&s, &u, &n, -h * VmVecDotf (&e, &u) / ul);
VmVecScaleAddf (&t, &v, &n, -h * VmVecDotf (&e, &v) / vl);
VmVecScaleAddf (&s, &s, &e, VmVecDotf (&e, &s));
VmVecScaleAddf (&t, &t, &e, VmVecDotf (&e, &t));
VmVecScalef (&s, &s, 1.8f);
VmVecScalef (&t, &t, 1.8f);
v = *vLight;
for (i = 0; i < 3; i++) {
	sprite [0].v [i] = v.v [i] + s.v [i] + t.v [i];
	sprite [1].v [i] = v.v [i] + s.v [i] - t.v [i];
	sprite [2].v [i] = v.v [i] - s.v [i] - t.v [i];
	sprite [3].v [i] = v.v [i] - s.v [i] + t.v [i];
	}
cosine = VmVecDotf (&e, &n);
return (float) sqrt (cosine) * coronaIntensities [gameOpts->render.nCoronaIntensity];
}

// -----------------------------------------------------------------------------------

void RenderSoftGlare (fVector *sprite, fVector *vCenter, int nTexture, float fIntensity, int bAdditive) 
{
	tRgbaColorf color;
	tTexCoord2f	tcGlare [4] = {{0,0},{1,0},{1,1},{0,1}};
	int 			i;
	grsBitmap	*bmP;

if (gameStates.render.bQueryCoronas)
	glDisable (GL_TEXTURE_2D);
else {
	glEnable (GL_TEXTURE_2D);
	if (bAdditive)
		glBlendFunc (GL_ONE, GL_ONE);		
	bmP = bAdditive ? bmpGlare : bmpCorona;
	}
color = gameData.render.color.textures [nTexture].color;
if (bAdditive)
	glColor4f (fIntensity * color.red, fIntensity * color.green, fIntensity * color.blue, 1);
else
	glColor4f (color.red, color.green, color.blue, fIntensity);
if (gameStates.render.bQueryCoronas != 2) {
	glDisable (GL_DEPTH_TEST);
	if (gameStates.render.bQueryCoronas == 1)
		glDepthFunc (GL_ALWAYS);
	}
if (G3EnableClientStates (!gameStates.render.bQueryCoronas, 0, GL_TEXTURE0)) {
	if (!gameStates.render.bQueryCoronas) {
		OglBindBmTex (bmP, 1, -1);
		glTexCoordPointer (2, GL_FLOAT, 0, tcGlare);
		}
	glVertexPointer (3, GL_FLOAT, sizeof (fVector), sprite);
	glDrawArrays (GL_QUADS, 0, 4);
	G3DisableClientStates (!gameStates.render.bQueryCoronas, 0, GL_TEXTURE0);
	}
else {
	if (!gameStates.render.bQueryCoronas)
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

void RenderCorona (short nSegment, short nSide, float fIntensity)
{
	fVector		sprite [4], vNormal, vCenter = {{0,0,0}}, vEye = {{0,0,0}};
	int			nTexture, bAdditive;
	tIntervalf	zRange;
	float			fAngle, fLight;

if (fIntensity < 0.01f)
	return;
if (!(nTexture = FaceHasCorona (nSegment, nSide, &bAdditive, &fIntensity)))
	return;
fLight = ComputeCoronaSprite (sprite, &vCenter, nSegment, nSide);
if (gameOpts->render.nRenderPath && gameOpts->render.nCoronaStyle && gameStates.ogl.bOcclusionQuery) {
	fIntensity *= ComputeSoftGlare (sprite, &vCenter, &vEye);
	RenderSoftGlare (sprite, &vCenter, nTexture, fIntensity, bAdditive);
	}
else {
	VmVecNormalf (&vNormal, sprite, sprite + 1, sprite + 2);
	VmVecNormalizef (&vEye, &vCenter);
	//dim corona depending on viewer angle
	if ((fAngle = VmVecDotf (&vNormal, &vEye)) > 0) {
		if (fAngle > 0.25f)
			return;
		fIntensity *= 1 - fAngle / 0.25f;
		}
	//move corona in towards viewer
	fIntensity *= coronaIntensities [gameOpts->render.nCoronaIntensity];
	if (0 == (fIntensity = MoveSpriteIn (sprite, &vCenter, &zRange, fIntensity)))
		return;
	ComputeHardGlare (sprite, &vCenter, &vNormal);
	RenderHardGlare (sprite, &vCenter, nTexture, fLight, fIntensity, &zRange, bAdditive);
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

if (!(gameStates.ogl.bOcclusionQuery && nQuery))
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
if (gameStates.render.bQueryCoronas == 1)
	return (float) (gameData.render.lights.coronaSamples [nQuery - 1] = nSamples);
fIntensity = gameData.render.lights.coronaSamples [nQuery - 1] ? (float) nSamples / (float) gameData.render.lights.coronaSamples [nQuery - 1] : 0;
#ifdef _DEBUG
if (fIntensity > 1)
	fIntensity = 1;
#endif
return (fIntensity > 1) ? 1 : (float) sqrt (fIntensity);
}

//------------------------------------------------------------------------------
// eof