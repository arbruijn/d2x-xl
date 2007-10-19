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
#include "gameseg.h"
#include "lighting.h"
#include "lightmap.h"
#include "renderlib.h"
#include "network.h"
#include "glare.h"

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
// The following code takes a face and renders a corona over it.
// The corona is rendered as a billboard, i.e. always facing the viewer.
// To do that, the center point of the corona's face is computed. Next, orthogonal
// vectors from the vector (eye,center) pointing up and left are computed. Finally,
// vectors orthogonal to the (eye,up) and (eye,left) vectors are computed. These
// all lie in a plane orthogonal to the viewer eye and are used to compute the billboard's 
// coordinates.
// To avoid the corona partially disappearing in the face it is rendered upon, it is 
// moved forward to the foremost z coordinate of that face.
// The corona's x and y dimensions are adjusted to the face's dimensions. To properly
// do that, the face coordinates are rotated so that the face normal has a 90 degree 
// angle towards the vertical (points up/down). Now the actual (unrotated) x and y 
// dimensions of the face are determined.
// The rotation matrix is determined by the face normal; the z axis is disregarded
// during rotation (rotation around z axis).
// With these x and y values and the orthogonal vectors the corona coordinates are computed.
// To make it match with the actual orientation of its face, it is rotated with another
// rotation matrix derived from the face's normal.

#define JAZ_CORONAS		2
#define ROTATE_CORONA	0
#define CORONA_OUTLINE	0

float coronaIntensities [] = {0.25f, 0.5f, 0.75f, 1};

#if JAZ_CORONAS == 1

void RenderCorona (short nSegment, short nSide)
{
	fVector		sprite [4];
	short			sideVerts [4];
	ushort		nWall;
	tTexCoord3f	uvlList [4] = {{{0,0,1}},{{1,0,1}},{{1,1,1}},{{0,1,1}}};
	fVector		vCenter = {{0,0,0}}, vEye;
	fVector		n, u, v, p, q, e, s, t;
	//fMatrix		r;
	int			i, nTexture, bAdditive;
	float			zMin = 1000000000.0f, zMax = -1000000000.0f;
	float			a, h, g, l = 0, dim = coronaIntensities [gameOpts->render.nCoronaIntensity];
	tFaceColor	*pf;
	tSide			*sideP = gameData.segs.segments [nSegment].sides + nSide;
	grsBitmap	*bmP;

#if 0//def _DEBUG
if (nSegment != 6)
	return;
#	if 1
if  (nSide != 2)
	return;
#	endif
#endif
if (IsMultiGame && extraGameInfo [1].bDarkness)
	return;
if (gameOpts->render.bDynamicLight) {
	i = FindDynLight (nSegment, nSide, -1);
	if ((i >= 0) && !gameData.render.lights.dynamic.lights [i].bOn)
		return;
	}
nWall = gameData.segs.segments [nSegment].sides [nSide].nWall;
if (IS_WALL (nWall)) {
	tWall *wallP = gameData.walls.walls + nWall;
	ubyte nType = wallP->nType;

	if ((nType == WALL_BLASTABLE) || 
		 (nType == WALL_DOOR) ||
		 (nType == WALL_OPEN) ||
		 (nType == WALL_CLOAKED))
		return;
	if (wallP->flags & (WALL_BLASTED | WALL_ILLUSION_OFF))
		return;
	}
// get and check the corona emitting texture
if (sideP->nOvlTex && IsLight (sideP->nOvlTex)) {
	nTexture = sideP->nOvlTex;
	bAdditive = gameOpts->render.bAdditiveCoronas;
	}
else {
	nTexture = sideP->nBaseTex;
	dim /= 2;
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
			return;
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
			return;
		case 404:
		case 405:
		case 406:
		case 407:
		case 408:
		case 409:
			dim *= 2;
			break;
		default:
			break;
		}
	}

GetSideVertIndex (sideVerts, nSegment, nSide);
// get the transformed face coordinates and compute their center
for (i = 0; i < 4; i++) {
	l += f2fl (sideP->uvls [i].l);
	sprite [i] = gameData.segs.fVertices [sideVerts [i]];	//already transformed
	if (gameOpts->render.nRenderPath)
		G3TransformPointf (sprite + i, sprite + i, 0);
	VmVecIncf (&vCenter, sprite + i);
	v = sprite [i];
	if (zMin > v.p.z)
		zMin = v.p.z;
	if (zMax < v.p.z)
		zMax = v.p.z;
	}
VmVecScalef (&vCenter, &vCenter, 0.25f);
VmVecNormalf (&n, sprite, sprite + 1, sprite + 2);
VmVecNormalizef (&vEye, &vCenter);
if ((a = VmVecDotf (&n, &vEye)) > 0) {
	if (a > 0.25f)
		return;
	dim *= 1 - a / 0.25f;
	}
#if 1
if (zMin > 0)
	VmVecScalef (&vCenter, &vCenter, zMin / vCenter.p.z);
else {
	if (zMin < -(zMax - zMin) / 2)
		return;
	dim *= 1 + zMin / (zMax - zMin) * 2;
	VmVecScalef (&vCenter, &vCenter, 1 / vCenter.p.z);
	}
#endif
VmVecAddf (&u, sprite + 2, sprite + 1);
VmVecDecf (&u, sprite);
VmVecDecf (&u, sprite + 3);
VmVecScalef (&u, &u, 0.25f);
VmVecAddf (&v, sprite, sprite + 1);
VmVecDecf (&v, sprite + 2);
VmVecDecf (&v, sprite + 3);
VmVecScalef (&v, &v, 0.25f);
#if 1
VmVecNormalizef (&n, VmVecCrossProdf (&n, &v, &u));
#else
VmVecNormalf (&n, &vCenter, &v, &u);
#endif
VmVecNormalizef (&e, &vCenter);
if (VmVecDotf (&e, &n) > 0.999f)
	p = v;
else
	VmVecNormalizef (&p, VmVecCrossProdf (&p, &e, &n));
VmVecCrossProdf (&q, &p, &e);
h = VmVecMagf (&u);
g = VmVecMagf (&v);
if (h > g)
	h = g;
g = 2 * (float) (fabs (VmVecDotf (&p, &v)) + fabs (VmVecDotf (&p, &u))) + h * VmVecDotf (&p, &n);
h = 2 * (float) (fabs (VmVecDotf (&q, &v)) + fabs (VmVecDotf (&q, &u))) + h * VmVecDotf (&q, &n);
#if 1
if (g / h > 8)
	h = g / 8;
else if (h / g > 8)
	g = h / 8;
#endif
VmVecScalef (&s, &p, g);
VmVecScalef (&t, &q, h);

VmVecAddf (sprite, &vCenter, &s);
sprite [1] = sprite [0];
VmVecIncf (sprite, &t);
VmVecDecf (sprite + 1, &t);
VmVecSubf (sprite + 3, &vCenter, &s);
sprite [2] = sprite [3];
VmVecIncf (sprite + 3, &t);
VmVecDecf (sprite + 2, &t);
#if 0
r.rVec.p.x =
r.uVec.p.y = q.p.y;
r.uVec.p.x = q.p.x;
r.rVec.p.y = -q.p.x;
r.rVec.p.z =
r.uVec.p.z =
r.fVec.p.x = 
r.fVec.p.y = 0;
r.fVec.p.z = 1;
for (i = 0; i < 4; i++) {
	VmVecDecf (sprite + i, &vCenter);
	VmVecRotatef (sprite + i, sprite + i, &r);
	VmVecIncf (sprite + i, &vCenter);
	}
#endif

a = VmVecMagf (&vCenter);
h = (zMax - zMin) / 2;
if (a < h) {
	dim *= a / h;
	}

pf = gameData.render.color.textures + nTexture;
a = (float) (pf->color.red * 3 + pf->color.green * 5 + pf->color.blue * 2) / 30 * 2;
a *= dim;
if (a < 0.01)
	return;
l /= 4;
if (l < 0.01)
	return;
glEnable (GL_TEXTURE_2D);
bmP = bAdditive ? bmpGlare : bmpCorona;
if (OglBindBmTex (bmP, 1, -1)) 
	return;
OglTexWrap (bmP->glTexture, GL_CLAMP);
glDisable (GL_CULL_FACE);
if (bAdditive) {
	l *= a;
	glBlendFunc (GL_ONE, GL_ONE);
	}
glColor4f (pf->color.red * l, pf->color.green * l, pf->color.blue * l, a);
glBegin (GL_QUADS);
for (i = 0; i < 4; i++) {
	glTexCoord2fv ((GLfloat *) (uvlList + i));
	glVertex3fv ((GLfloat *) (sprite + i));
	}
glEnd ();
if (bAdditive)
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glEnable (GL_CULL_FACE);

#if defined(_DEBUG) && CORONA_OUTLINE
glDisable (GL_TEXTURE_2D);
glColor4d (1,1,1,1);
glLineWidth (2);
glBegin (GL_LINE_LOOP);
for (i = 0; i < 4; i++)
	glVertex3fv ((GLfloat *) (sprite + i));
glEnd ();
glBegin (GL_LINES);
vCenter.p.x += 5;
glVertex3fv ((GLfloat *) &vCenter);
vCenter.p.x -= 10;
glVertex3fv ((GLfloat *) &vCenter);
vCenter.p.x += 5;
vCenter.p.y += 5;
glVertex3fv ((GLfloat *) &vCenter);
vCenter.p.y -= 10;
glVertex3fv ((GLfloat *) &vCenter);
glEnd ();
glLineWidth (1);
#endif
}

#elif JAZ_CORONAS == 2

#define ENTRY(v, i) * ((float*)v + i)

void RenderGlare (fVector *light, fVector *eye, fVector *u, fVector *v, int nTexture, int bAdditive) 
{
	tRgbaColorf color;
	fVector 		n, e, s, t;
	float 		ul, vl, h,intensity, cosine;
	fVector3		sprite [4];
	tTexCoord2f	texCoords [4] = {{0,0},{1,0},{1,1},{0,1}};
	int 			i;
	grsBitmap	*bmP;

VmVecSubf (&e, eye, light);
VmVecNormalizef (&e, &e);
VmVecCrossProdf (&n, v, u);
VmVecNormalizef (&n, &n);
cosine = VmVecDotf (&e, &n);
ul = VmVecMagf (u);
vl = VmVecMagf (v);
h = (ul > vl) ? vl : ul;
VmVecScaleAddf (&s, u, &n, -h * VmVecDotf (&e, u) / ul);
VmVecScaleAddf (&t, v, &n, -h * VmVecDotf (&e, v) / vl);
VmVecScaleAddf (&s, &s, &e, VmVecDotf (&e, &s));
VmVecScaleAddf (&t, &t, &e, VmVecDotf (&e, &t));
VmVecScalef (&s, &s, 1.8f);
VmVecScalef (&t, &t, 1.8f);
for (i = 0; i < 3; i++) {
	sprite [0].v [i] = ENTRY (light, i) + ENTRY (&s, i) + ENTRY (&t, i);
	sprite [1].v [i] = ENTRY (light, i) + ENTRY (&s, i) - ENTRY (&t, i);
	sprite [2].v [i] = ENTRY (light, i) - ENTRY (&s, i) - ENTRY (&t, i);
	sprite [3].v [i] = ENTRY (light, i) - ENTRY (&s, i) + ENTRY (&t, i);
	}
color = gameData.render.color.textures [nTexture].color;
intensity = (float) sqrt (cosine) * coronaIntensities [gameOpts->render.nCoronaIntensity];
glEnable (GL_TEXTURE_2D);
bmP = bAdditive ? bmpGlare : bmpCorona;
OglBindBmTex (bmP, 1, -1);
if (bAdditive)
	glBlendFunc (GL_ONE, GL_ONE);		
glColor4f (intensity * color.red, intensity * color.green, intensity * color.blue, 1.0f);
glDisable (GL_DEPTH_TEST);
glBegin (GL_QUADS);
for  (i = 0; i < 4; i++) {
	glTexCoord2fv ((GLfloat *) (texCoords + i));
	glVertex3fv ((GLfloat *) (sprite + i));
	}
glEnd ();
if (bAdditive)
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#if CORONA_OUTLINE
glDisable (GL_LINE_SMOOTH);
glDisable (GL_TEXTURE_2D);
glColor4d (1, 1, 1, 1);
glLineWidth (2);
glBegin (GL_LINE_LOOP);
for (i = 0; i < 4; i++)
	glVertex3fv  (sprite + 3*i);
glEnd ();
VmVecAddf (&lpt, light, &t);
glBegin (GL_LINES);
glVertex3fv ((GLfloat*) light);	
glVertex3fv ((GLfloat*) &lpt);
glEnd ();
glLineWidth (1);
#endif //RENDER_OUTLINE
glEnable (GL_DEPTH_TEST);
}

void RenderCorona (short nSegment, short nSide)
{
	fVector			sprite [4];
	short				sideVerts [4];
	ushort			nWall;
	tTexCoord3f		uvlList [4] = {{{0, 0, 1}}, {{1, 0, 1}}, {{1, 1, 1}}, {{0, 1, 1}}};
	fVector			vCenter = {{0, 0, 0}}, vEye = {{0, 0, 0}};
	fVector			u, v;
	int				i, nTexture, bAdditive;
	float				zMin = 1000000000.0f, zMax = -1000000000.0f;
	float				l = 0, dim = coronaIntensities [gameOpts->render.nCoronaIntensity];
	tSide				*sideP = gameData.segs.segments [nSegment].sides + nSide;

#if 0//def _DEBUG
if  (nSegment != 6)
	return;
#	if 1
if   (nSide != 2)
	return;
#	endif
#endif
if  (IsMultiGame && extraGameInfo [1].bDarkness)
	return;
if  (gameOpts->render.bDynamicLight) {
	i = FindDynLight  (nSegment, nSide, -1);
	if  ( (i >= 0) && !gameData.render.lights.dynamic.lights [i].bOn)
		return;
	}
nWall = gameData.segs.segments [nSegment].sides [nSide].nWall;
if  (IS_WALL  (nWall)) {
	tWall *wallP = gameData.walls.walls + nWall;
	ubyte nType = wallP->nType;

	if  ((nType == WALL_BLASTABLE) || 
		  (nType == WALL_DOOR) ||
		  (nType == WALL_OPEN) ||
		  (nType == WALL_CLOAKED))
		return;
	if  (wallP->flags &  (WALL_BLASTED | WALL_ILLUSION_OFF))
		return;
	}
// get and check the corona emitting texture
if  (sideP->nOvlTex && IsLight  (sideP->nOvlTex)) {
	nTexture = sideP->nOvlTex;
	bAdditive = gameOpts->render.bAdditiveCoronas;
	}
else {
	nTexture = sideP->nBaseTex;
	dim /= 2;
	bAdditive = 0;
	}
if  (gameStates.app.bD1Mission) {
	switch  (nTexture) {
		case 289:	//empty light
		case 328:	//energy sparks
		case 334:	//reactor
		case 335:
		case 336:
		case 337:
		case 338:	//robot generators
		case 339:
			return;
		default:
			break;
		}
	}
else {
	switch  (nTexture) {
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
			return;
		case 404:
		case 405:
		case 406:
		case 407:
		case 408:
		case 409:
			dim *= 2;
			break;
		default: 
			break;
		}
	}

GetSideVertIndex  (sideVerts, nSegment, nSide);
// get the transformed face coordinates and compute their center
for  (i = 0; i < 4; i++) {
	l += f2fl  (sideP->uvls [i].l);
	if (gameOpts->render.nRenderPath)
		G3TransformPointf (sprite + i, gameData.segs.fVertices + sideVerts [i], 0);	//already transformed
	else
		sprite [i] = gameData.segs.fVertices [sideVerts [i]];	//already transformed
	if (zMin > sprite [i].p.z)
		zMin = sprite [i].p.z;
	}
VmsVecToFloat (&v, SIDE_CENTER_I (nSegment, nSide));
G3TransformPointf (&vCenter, &v, 0);	//already transformed
vCenter.p.z = zMin;
VmVecAddf (&u, sprite + 2, sprite + 1);
VmVecDecf (&u, sprite);
VmVecDecf (&u, sprite + 3);
VmVecScalef (&u, &u, 0.25f);
VmVecAddf (&v, sprite, sprite + 1);
VmVecDecf (&v, sprite + 2);
VmVecDecf (&v, sprite + 3);
VmVecScalef (&v, &v, 0.25f);
RenderGlare (&vCenter, &vEye, &u, &v, nTexture, bAdditive);
}

#else //JAZ_CORONAS

void RenderCorona (short nSegment, short nSide)
{
	fVector		vertList [4], sprite [4];
	short			sideVerts [4];
	ushort		nWall;
	tTexCoord3f			uvlList [4] = {{{0,0,1}},{{1,0,1}},{{1,1,1}},{{0,1,1}}};
	fVector		d, o, n, v, vCenter = {{0,0,0}}, vEye;
#if ROTATE_CORONA
	fVector		vDeltaX, vDeltaY;
#endif
	fMatrix		r;
	int			i, j, nTexture, bAdditive;
	float			zMin = 1000000000.0f, zMax = -1000000000.0f;
	float			a, h, m = 0, l = 0, dx = 0, dy = 0, dim = coronaIntensities [gameOpts->render.nCoronaIntensity];
	tFaceColor	*pf;
	tSide			*sideP = gameData.segs.segments [nSegment].sides + nSide;
	grsBitmap	*bmP;

#if 0//def _DEBUG
if (nSegment != 16)
	return;
#	if 1
if  (nSide != 0)
	return;
#	endif
#endif
if (IsMultiGame && extraGameInfo [1].bDarkness)
	return;
if (gameOpts->render.bDynamicLight) {
	i = FindDynLight (nSegment, nSide, -1);
	if ((i >= 0) && !gameData.render.lights.dynamic.lights [i].bOn)
		return;
	}
nWall = gameData.segs.segments [nSegment].sides [nSide].nWall;
if (IS_WALL (nWall)) {
	tWall *wallP = gameData.walls.walls + nWall;
	ubyte nType = wallP->nType;

	if ((nType == WALL_BLASTABLE) || 
		 (nType == WALL_DOOR) ||
		 (nType == WALL_OPEN) ||
		 (nType == WALL_CLOAKED))
		return;
	if (wallP->flags & (WALL_BLASTED | WALL_ILLUSION_OFF))
		return;
	}
// get and check the corona emitting texture
#if 0
nTexture = sideP->nOvlTex;
#else
if (sideP->nOvlTex && IsLight (sideP->nOvlTex)) {
	nTexture = sideP->nOvlTex;
	bAdditive = gameOpts->render.bAdditiveCoronas;
	}
else {
	nTexture = sideP->nBaseTex;
	dim /= 2;
	bAdditive = 0;
	}
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
			return;
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
			return;
		case 404:
		case 405:
		case 406:
		case 407:
		case 408:
		case 409:
			dim *= 2;
			break;
		default:
			break;
		}
	}

GetSideVertIndex (sideVerts, nSegment, nSide);
// get the transformed face coordinates and compute their center
for (i = 0; i < 4; i++) {
	vertList [i] = gameData.segs.fVertices [sideVerts [i]];	//already transformed
	VmVecIncf (&vCenter, vertList + i);
	l += f2fl (sideP->uvls [i].l);
	}
VmVecScalef (&vCenter, &vCenter, 0.25);

#if 1

VmVecNormalf (&n, vertList, vertList + 1, vertList + 2);
VmVecNormalizef (&vEye, &vCenter);
if ((a = VmVecDotf (&n, &vEye)) > 0) {
	if (a > 0.25f)
		return;
	dim *= 1 - a / 0.25f;
	}
#if 0
else	//brighten if facing directly
	dim = 1.0f + 0.5f * -a / 1.0f;
#endif
// o serves to slightly displace the corona from its face to avoid z fighting
VmVecScalef (&o, &n, 1.0f / 10.0f);
VmVecIncf (&n, &vEye);
#if 1	//might remove z from normal 
n.p.z = 0;
#endif
VmVecNormalizef (&n, &n);
// compute rotation matrix to align transformed face
//n.p.x = 1 - n.p.y;
#if 0
i = CalcFaceDimensions (nSegment, nSide, NULL, NULL, sideVerts);
m = VmVecDistf (vertList + i, vertList + i + 1);
n.p.x = (vertList [i].p.y - vertList [i + 1].p.y) / m;
n.p.y = (vertList [i].p.x - vertList [i + 1].p.x) / m;
#endif
r.rVec.p.x =
r.uVec.p.y = n.p.y;
r.uVec.p.x = n.p.x;
r.rVec.p.y = -n.p.x;
r.rVec.p.z =
r.uVec.p.z =
r.fVec.p.x = 
r.fVec.p.y = 0;
r.fVec.p.z = 1;
for (i = 0; i < 4; i++) {
	VmVecSubf (&d, vertList + i, &vCenter);	//compute face coordinate relative to pivot
	VmVecRotatef (&v, &d, &r);	//align face coordinate
	sprite [i] = v;
	VmVecIncf (vertList + i, &d);
	VmVecIncf (vertList + i, &o);
	//determine depth extension of the face
	v = vertList [i];
	if (zMin > v.p.z)
		zMin = v.p.z;
	if (zMax < v.p.z)
		zMax = v.p.z;
	m += VmVecMagf (&d);	//accumulate face dimensions
	}

m /= 4;	//compute average face dimension
// compute x and y dimensions of aligned face
for (i = 0; i < 4; i++) {
	j = (i + 1) % 4;
	h = (float) fabs (sprite [i].p.x - sprite [j].p.x);
	if (h > dx)
		dx = h;
	h = (float) fabs (sprite [i].p.y - sprite [j].p.y);
	if (h > dy)
		dy = h;
	}
m = (float) sqrt (dx * dx + dy * dy) / 4;	//basic corona size to make it visible regardless of dx and dy

a = VmVecMagf (&vCenter);
// determine whether viewer has passed foremost z coordinate of corona's face
// if so, push corona back
if (zMin > 0) 
	VmVecScalef (&vCenter, &vCenter, zMin / vCenter.p.z);
else {
	if (zMin < -(zMax - zMin) / 2)
		return;
	dim *= 1 + zMin / (zMax - zMin) * 2;
	a /= vCenter.p.z;
	VmVecScalef (&vCenter, &vCenter, 1 / vCenter.p.z);
	}
if (m > a)
	m = a;

//create rotation matrix to match corona with face
#if 1
r.rVec.p.x =
r.uVec.p.y = n.p.x;
r.rVec.p.y = -n.p.y;
r.uVec.p.x = n.p.y;
r.rVec.p.z =
r.uVec.p.z = 0;
#endif
#if !ROTATE_CORONA
CalcSpriteCoords (sprite, &vCenter, &vEye, m + dy / 2, m + dx / 2, &r);
#else
// compute orthogonal vectors for calculation of billboard coordinates
h = vCenter.p.x * vCenter.p.x + vCenter.p.y * vCenter.p.y + vCenter.p.z * vCenter.p.z;
v.p.x = v.p.z = 0;
v.p.y = (vCenter.p.y == 0) ? 1: h / vCenter.p.y;
VmVecDecf (&v, &vCenter);
VmVecNormalizef (&v, &v);
VmVecCrossProdf (&vDeltaX, &v, &vEye);	//orthogonal vector in plane through face center and perpendicular to viewer
VmVecScalef (&vDeltaX, &vDeltaX, dx / 2);
v.p.y = v.p.z = 0;
v.p.x = (vCenter.p.x == 0) ? 1: h / vCenter.p.x;
VmVecDecf (&v, &vCenter);
VmVecNormalizef (&v, &v);
VmVecCrossProdf (&vDeltaY, &v, &vEye);
VmVecScalef (&vDeltaY, &vDeltaY, dy / 2);

//compute corona coordinates
v.p.x = -vDeltaX.p.x - vDeltaY.p.x;
v.p.y = +vDeltaX.p.y + vDeltaY.p.y;
v.p.z = -vDeltaX.p.z - vDeltaY.p.z;
#if 0
VmVecRotatef (sprite, &v, &r);
VmVecIncf (sprite, &vCenter);
#else
VmVecAddf (sprite, &v, &vCenter);
#endif
v.p.x = v.p.y = v.p.z = 0;
v.p.x = +vDeltaX.p.x - vDeltaY.p.x;
v.p.y = +vDeltaX.p.y + vDeltaY.p.y;
v.p.z = +vDeltaX.p.z - vDeltaY.p.z;
#if 0
VmVecRotatef (sprite + 1, &v, &r);
VmVecIncf (sprite + 1, &vCenter);
#else
VmVecAddf (sprite + 1, &v, &vCenter);
#endif
v.p.x = v.p.y = v.p.z = 0;
v.p.x = +vDeltaX.p.x + vDeltaY.p.x;
v.p.y = -vDeltaX.p.y - vDeltaY.p.y;
v.p.z = +vDeltaX.p.z + vDeltaY.p.z;
#if 0
VmVecRotatef (sprite + 2, &v, &r);
VmVecIncf (sprite + 2, &vCenter);
#else
VmVecAddf (sprite + 2, &v, &vCenter);
#endif
v.p.x = -vDeltaX.p.x + vDeltaY.p.x;
v.p.y = -vDeltaX.p.y - vDeltaY.p.y;
v.p.z = -vDeltaX.p.z + vDeltaY.p.z;
#if 0
VmVecRotatef (sprite + 3, &v, &r);
VmVecIncf (sprite + 3, &vCenter);
#else
VmVecAddf (sprite + 3, &v, &vCenter);
#endif
#endif

#if 0
v.p.x = v.p.y = 0;
v.p.z = 1;
VmVecNormalizef (&n, &vCenter);
a = VmVecDeltaAngf (&v, &n, NULL);
if (n.p.x > 0)
	a = -a;
#if 0
if (n.p.x != 0)
	a /= (float) fabs (n.p.x);
#endif
#if 1
if (n.p.y != 0) {
	a *= 1 - sqrt (fabs (n.p.y)); //+= (1 - a) * (float) sqrt (fabs (n.p.y));
	}
#endif
//a = 0.5f - a;
r.rVec.p.x =
r.uVec.p.y = (float) cos (a);
r.uVec.p.x = (float) sin (a);
r.rVec.p.y = -r.uVec.p.x;
r.rVec.p.z =
r.uVec.p.z =
r.fVec.p.x = 
r.fVec.p.y = 0;
r.fVec.p.z = 1;
for (i = 0; i < 4; i++) {
	VmVecDecf (sprite + i, &vCenter);
	VmVecRotatef (sprite + i, sprite + i, &r);
	VmVecIncf (sprite + i, &vCenter);
	}
#endif

VmVecNormalf (&o, sprite, sprite + 1, sprite + 2);
VmVecScalef (&o, &o, 0.1f);
for (i = 0; i < 4; i++)
	VmVecIncf (sprite + i, &o);
pf = gameData.render.color.textures + nTexture;
a = (float) (pf->color.red * 3 + pf->color.green * 5 + pf->color.blue * 2) / 30 * 2;
a *= dim;
if (a < 0.01)
	return;
l /= 4;
if (l < 0.01)
	return;
//render the corona
VmVecNormalf (&n, sprite, sprite + 1, sprite + 2);

#endif

glEnable (GL_TEXTURE_2D);
if (!(bAdditive ? LoadGlare () : LoadCorona ()))
	return;
bmP = bAdditive ? bmpGlare : bmpCorona;
if (OglBindBmTex (bmP, 1, -1)) 
	return;
OglTexWrap (bmP->glTexture, GL_CLAMP);
glDisable (GL_CULL_FACE);
if (bAdditive) {
	l *= coronaIntensities [gameOpts->render.nCoronaIntensity] / 2;
	glBlendFunc (GL_ONE, GL_ONE);
	}
glColor4f (pf->color.red * l, pf->color.green * l, pf->color.blue * l, a);
glBegin (GL_QUADS);
for (i = 0; i < 4; i++) {
	glTexCoord2fv ((GLfloat *) (uvlList + i));
	glVertex3fv ((GLfloat *) (sprite + i));
	}
glEnd ();
if (bAdditive)
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glEnable (GL_CULL_FACE);

#if defined(_DEBUG) && CORONA_OUTLINE
glDisable (GL_TEXTURE_2D);
glColor4d (1,1,1,1);
glLineWidth (2);
glBegin (GL_LINE_LOOP);
for (i = 0; i < 4; i++)
	glVertex3fv ((GLfloat *) (sprite + i));
glEnd ();
glBegin (GL_LINES);
vCenter.p.x += 5;
glVertex3fv ((GLfloat *) &vCenter);
vCenter.p.x -= 10;
glVertex3fv ((GLfloat *) &vCenter);
vCenter.p.x += 5;
vCenter.p.y += 5;
glVertex3fv ((GLfloat *) &vCenter);
vCenter.p.y -= 10;
glVertex3fv ((GLfloat *) &vCenter);
glEnd ();
glLineWidth (1);
#endif
}

#endif //JAZ_CORONAS

//------------------------------------------------------------------------------
// eof