//------------------------------------------------------------------------------

#include <math.h>

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#include "inferno.h"
#include "error.h"
#include "network.h"
#include "sphere.h"
#include "render.h"
#include "maths.h"
#include "u_mem.h"
#include "glare.h"
#include "ogl_lib.h"
#include "objeffects.h"
#include "objrender.h"
#include "transprender.h"

#define SIMPLE_SPHERE	1
#define ADDITIVE_SPHERE_BLENDING 1
#define MAX_SPHERE_RINGS 256

tOOF_triangle baseSphereOcta [8] = {
	{{{-1,0,1},{1,0,1},{0,1,0}},{0,0,0}},
	{{{1,0,1},{1,0,-1},{0,1,0}},{0,0,0}},
	{{{1,0,-1},{-1,0,-1},{0,1,0}},{0,0,0}},
	{{{-1,0,-1},{-1,0,1},{0,1,0}},{0,0,0}},
	{{{1,0,1},{-1,0,1},{0,-1,0}},{0,0,0}},
	{{{1,0,-1},{1,0,1},{0,-1,0}},{0,0,0}},
	{{{-1,0,-1},{1,0,-1},{0,-1,0}},{0,0,0}},
	{{{-1,0,1},{-1,0,-1},{0,-1,0}},{0,0,0}}
};

tOOF_quad baseSphereCube [6] = {
	{{{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}},{0,0,0}},
	{{{1,-1,1},{1,-1,-1},{1,1,-1},{1,1,1}},{0,0,0}},
	{{{1,-1,-1},{-1,-1,-1},{-1,1,-1},{1,1,-1}},{0,0,0}},
	{{{-1,-1,1},{-1,1,1},{-1,1,-1},{-1,-1,-1}},{0,0,0}},
	{{{-1,1,1},{1,1,1},{1,1,-1},{-1,1,-1}},{0,0,0}},
	{{{-1,-1,-1},{1,-1,-1},{1,-1,1},{-1,-1,1}},{0,0,0}}
};

//------------------------------------------------------------------------------

tOOF_vector *OOF_TriangleCenter (tOOF_triangle *pt)
{
tOOF_vector c = pt->p [0];
OOF_VecInc (&c, pt->p + 1);
OOF_VecInc (&c, pt->p + 2);
OOF_VecScale (&c, 1.0f / 3.0f);
pt->c = c;
return &pt->c;
}

//------------------------------------------------------------------------------

int SplitTriangle (tOOF_triangle *pDest, tOOF_triangle *pSrc)
{
	int	i, j;
	tOOF_vector	c = pSrc->c;
	tOOF_vector	h [6];

for (i = 0; i < 3; i++)
	h [2 * i] = pSrc->p [i];
for (i = 1; i < 6; i += 2)
	OOF_VecAdd (h + i, h + i - 1, h + (i + 1) % 6);
for (i = 0; i < 6; i++, pDest++) {
	pDest->p [0] = h [i];
	pDest->p [1] = h [(i + 1) % 6];
	pDest->p [2] = c;
	for (j = 0; j < 3; j++)
		OOF_VecScale (pDest->p + j, 1.0f / OOF_VecMag (pDest->p + j));
	OOF_TriangleCenter (pDest);
	}
return 1;
}

//------------------------------------------------------------------------------

int TesselateSphereTri (tOOF_triangle *pDest, tOOF_triangle *pSrc, int nFaces)
{
	int	i;

for (i = 0; i < nFaces; i++, pDest += 6, pSrc++)
	SplitTriangle (pDest, pSrc);
return 1;
}

//------------------------------------------------------------------------------

int BuildSphereTri (tOOF_triangle **buf, int *pnFaces, int nTessDepth)
{
    int		i, j, nFaces = 0;
	 float	l;

l = (float) sqrt (2.0f) / 2.0f;
for (i = 0; i < 8; i++) {
	for (j = 0; j < 3; j++) {
		buf [0][i].p [j] = baseSphereOcta [i].p [j];
		OOF_VecScale (buf [0][i].p + j, l);
		}
	OOF_TriangleCenter (buf [0] + i);
	}
nFaces = 8;
for (i = 0, j = 1; i < nTessDepth; i++, nFaces *= 6, j = !j) {
	TesselateSphereTri (buf [j], buf [!j], nFaces);
	}
*pnFaces = nFaces;
return !j;
}

//------------------------------------------------------------------------------

tOOF_vector *OOF_QuadCenter (tOOF_quad *pt)
{
tOOF_vector c;
OOF_VecAdd (&c, pt->p, pt->p + 1);
OOF_VecInc (&c, pt->p + 2);
OOF_VecInc (&c, pt->p + 3);
OOF_VecScale (&c, 1.0f / 4.0f);
pt->c = c;
return &pt->c;
}

//------------------------------------------------------------------------------

int SplitQuad (tOOF_quad *pDest, tOOF_quad *pSrc)
{
	int	i, j;
	tOOF_vector	c = pSrc->c;
	tOOF_vector	h [8];

for (i = 0; i < 4; i++)
	h [2 * i] = pSrc->p [i];
for (i = 1; i < 8; i += 2)
	OOF_VecAdd (h + i, h + i - 1, h + (i + 1) % 8);
for (i = 0; i < 8; i += 2, pDest++) {
	pDest->p [0] = h [i ? i - 1 : 7];
	pDest->p [1] = h [i];
	pDest->p [2] = h [(i + 1) % 8];
	pDest->p [3] = c;
	for (j = 0; j < 4; j++)
		OOF_VecScale (pDest->p + j, 1.0f / OOF_VecMag (pDest->p + j));
	OOF_QuadCenter (pDest);
	}
return 1;
}

//------------------------------------------------------------------------------

int TesselateSphereQuad (tOOF_quad *pDest, tOOF_quad *pSrc, int nFaces)
{
	int	i;

for (i = 0; i < nFaces; i++, pDest += 4, pSrc++)
	SplitQuad (pDest, pSrc);
return 1;
}

//------------------------------------------------------------------------------

int BuildSphereQuad (tOOF_quad **buf, int *pnFaces, int nTessDepth)
{
    int		i, j, nFaces;
	 float	l;

l = (float) sqrt (2.0f) / 2.0f;
for (i = 0; i < 6; i++) {
	for (j = 0; j < 4; j++) {
		buf [0][i].p [j] = baseSphereCube [i].p [j];
		OOF_VecScale (buf [0][i].p + j, l);
		}
	OOF_QuadCenter (buf [0] + i);
	}
nFaces = 6;
for (i = 0, j = 1; i < nTessDepth; i++, nFaces *= 4, j = !j) {
	TesselateSphereQuad (buf [j], buf [!j], nFaces);
	}
*pnFaces = nFaces;
return !j;
}

//------------------------------------------------------------------------------

int CreateSphere (tSphereData *sdP)
{
	int			nFaces, i, j;
	tOOF_vector	*buf [2];

PrintLog ("Creating shield sphere\n");
if (sdP->nFaceNodes == 3) {
	nFaces = 8;
	j = 6;
	}
else {
	nFaces = 6;
	j = 4;
	}
for (i = 0; i < sdP->nTessDepth; i++)
	nFaces *= j;
for (i = 0; i < 2; i++) {
	if (!(buf [i] = (tOOF_vector *) D2_ALLOC (nFaces * (sdP->nFaceNodes + 1) * sizeof (tOOF_vector)))) {
		if (i)
			D2_FREE (buf [i - 1]);
		return -1;
		}
	}
j = (sdP->nFaceNodes == 3) ?
	 BuildSphereTri ((tOOF_triangle **) buf, &nFaces, sdP->nTessDepth) :
	 BuildSphereQuad ((tOOF_quad **) buf, &nFaces, sdP->nTessDepth);
D2_FREE (buf [!j]);
sdP->pSphere = buf [j];
return nFaces;
}

//------------------------------------------------------------------------------

tOOF_triangle *RotateSphere (tSphereData *sdP, tOOF_vector *pRotSphere, tOOF_vector *pPos, float xScale, float yScale, float zScale)
{
	tOOF_matrix	m;
	tOOF_vector	h, v, p,
					*pSphere = sdP->pSphere,
					*s = pRotSphere;
	int			nFaces;

OOF_MatVms2Oof (&m, viewInfo.view[0]);
OOF_VecVms2Oof (&p, viewInfo.pos);
for (nFaces = sdP->nFaces * (sdP->nFaceNodes + 1); nFaces; nFaces--, pSphere++, pRotSphere++) {
	v = *pSphere;
	v.x *= xScale;
	v.y *= yScale;
	v.z *= zScale;
	OOF_VecRot (pRotSphere, OOF_VecSub (&h, &v, &p), &m);
	}
return (tOOF_triangle *) s;
}

//------------------------------------------------------------------------------

tOOF_triangle *SortSphere (tOOF_triangle *pSphere, int left, int right)
{
	int	l = left,
			r = right;
	float	m = pSphere [(l + r) / 2].c.z;

do {
	while (pSphere [l].c.z < m)
		l++;
	while (pSphere [r].c.z > m)
		r--;
	if (l <= r) {
		if (l < r) {
			tOOF_triangle h = pSphere [l];
			pSphere [l] = pSphere [r];
			pSphere [r] = h;
			}
		}
	++l;
	--r;
	} while (l <= r);
if (right > l)
   SortSphere (pSphere, l, right);
if (r > left)
   SortSphere (pSphere, left, r);
return pSphere;
}

//------------------------------------------------------------------------------

int InitSphereSurface (tSphereData *sdP, float red, float green, float blue, float alpha, grsBitmap *bmP, float *pfScale)
{
	float	fScale;
	int	bTextured = 0;

if (sdP->pPulse) {
	static time_t	t0 = 0;
	if (gameStates.app.nSDLTicks - t0 > 25) {
		t0 = gameStates.app.nSDLTicks;
		sdP->pPulse->fScale += sdP->pPulse->fDir;
		if (sdP->pPulse->fScale > 1.0f) {
			sdP->pPulse->fScale = 1.0f;
			sdP->pPulse->fDir = -sdP->pPulse->fSpeed;
			}
		else if (sdP->pPulse->fScale < sdP->pPulse->fMin) {
			sdP->pPulse->fScale = sdP->pPulse->fMin;
			sdP->pPulse->fDir = sdP->pPulse->fSpeed;
			}
		}
	fScale = sdP->pPulse->fScale;
	}
else
	fScale = 1;
#if 1
if (bmP && (bmP == bmpShield)) {
	static time_t t0 = 0;
	bTextured = 1;
	if ((gameStates.app.nSDLTicks - t0 > 40) && BM_CURFRAME (bmP)) {
		t0 = gameStates.app.nSDLTicks;
		BM_CURFRAME (bmP)++;
		if (BM_CURFRAME (bmP) >= BM_FRAMES (bmP) + BM_FRAMECOUNT (bmP))
			BM_CURFRAME (bmP) = BM_FRAMES (bmP);
		}
	}
#endif
if (bmP) {
	glActiveTexture (GL_TEXTURE0);
	glClientActiveTexture (GL_TEXTURE0);
	glEnable (GL_TEXTURE_2D);
	if (OglBindBmTex (bmP, 1, 1))
		bmP = NULL;
	else {
		if (BM_CURFRAME (bmP))
			bmP = BM_CURFRAME (bmP);
		if (!bTextured)
			bTextured = -1;
		}
	}
if (!bmP) {
	glDisable (GL_TEXTURE_2D);
	bTextured = 0;
	alpha /= 2;
	}
if (alpha < 0)
	alpha = (float) (1.0f - gameStates.render.grAlpha / (float) GR_ACTUAL_FADE_LEVELS);
if (alpha < 1.0f) {
#if ADDITIVE_SPHERE_BLENDING
	fScale *= coronaIntensities [gameOpts->render.coronas.nObjIntensity];
#endif
	if (sdP->pPulse && sdP->pPulse->fScale) {
		red *= fScale;
		green *= fScale;
		blue *= fScale;
		}
	}
glColor4f (red, green, blue, alpha);
*pfScale = fScale;
glDepthMask (0);
return bTextured;
}

//------------------------------------------------------------------------------

typedef struct tSphereCoord {
	fVector	vPos;
	tTexCoord3f		uvl;
} tSphereCoord;

tSphereCoord	*sphereCoordP = NULL;
int				nSphereCoord = 0;

//------------------------------------------------------------------------------

void FreeSphereCoord (void)
{
if (sphereCoordP)
	D2_FREE (sphereCoordP);
}

//------------------------------------------------------------------------------

int CreateSphereSimple (int nRings)
{
	int				h, i, j;
	float				t1, t2, t3, a, sint1, cost1, sint2, cost2, sint3, cost3;
	tSphereCoord	*psc;

if (nRings > MAX_SPHERE_RINGS)
	nRings = MAX_SPHERE_RINGS;
h = nRings * (nRings + 1);
if (nSphereCoord == h)
	return (sphereCoordP != NULL);

FreeSphereCoord ();
if (!(sphereCoordP = (tSphereCoord *) D2_ALLOC (h * sizeof (tSphereCoord))))
	return 0;
	nSphereCoord = h;
h = nRings / 2;
a = (float) (2 * Pi / nRings);
psc = sphereCoordP;
for (j = 0; j < h; j++) {
	t1 = (float) (j * a - Pi / 2);
	t2 = t1 + a;
	sint1 = (float) sin (t1);
	cost1 = (float) cos (t1);
	sint2 = (float) sin (t2);
	cost2 = (float) cos (t2);
	for (i = 0; i <= nRings; i++) {
		t3 = i * a;
		sint3 = (float) sin (t3);
		cost3 = (float) cos (t3);
		psc->vPos[X] = cost2 * cost3;
		psc->vPos[Y] = sint2;
		psc->vPos[Z] = cost2 * sint3;
		psc->uvl.v.u = 1 - (float) i / nRings;
		psc->uvl.v.v = (float) (2 * j + 2) / nRings;
		psc++;
		psc->vPos[X] = cost1 * cost3;
		psc->vPos[Y] = sint1;
		psc->vPos[Z] = cost1 * sint3;
		psc->uvl.v.u = 1 - (float) i / nRings;
		psc->uvl.v.v = (float) (2 * j) / nRings;
		psc++;
		}
	}
return 1;
}

//------------------------------------------------------------------------------

inline void RenderSphereRing (fVector *vertexP, tTexCoord2f *texCoordP, int nItems, int bTextured, int nPrimitive)
{
if (G3EnableClientStates (bTextured, 0, 0, GL_TEXTURE0)) {
	if (bTextured)
		glTexCoordPointer (2, GL_FLOAT, 0, texCoordP);
	glVertexPointer (3, GL_FLOAT, sizeof (fVector), vertexP);
	glDrawArrays (nPrimitive, 0, nItems);
	G3DisableClientStates (bTextured, 0, 0, GL_TEXTURE0);
	}
else {
	int	i;

	glBegin (nPrimitive);
	for (i = 0; i < nItems; i++) {
		if (bTextured)
			glTexCoord2fv ((GLfloat *) (texCoordP + i));
		glVertex3fv ((GLfloat *) (vertexP + i));
		}
	glEnd ();
	}
}

//------------------------------------------------------------------------------

void RenderSphereSimple (float fRadius, int nRings, float red, float green, float blue, float alpha, int bTextured, int nTiles)
{
	int				nCull, h, i, j, nQuads;
	fVector			p [2 * MAX_SPHERE_RINGS + 2];
	tTexCoord2f		tc [2 * MAX_SPHERE_RINGS + 2];
	tSphereCoord	*psc [2];

if (nRings > MAX_SPHERE_RINGS)
	nRings = MAX_SPHERE_RINGS;
if (gameStates.ogl.bUseTransform)
	glScalef (fRadius, fRadius, fRadius);
if (!CreateSphereSimple (nRings))
	return;
h = nRings / 2;
nQuads = 2 * nRings + 2;
if (gameStates.ogl.bUseTransform) {
	for (nCull = 0; nCull < 2; nCull++) {
		psc [0] = psc [1] = sphereCoordP;
		glCullFace (nCull ? GL_FRONT : GL_BACK);
		for (j = 0; j < h; j++) {
			for (i = 0; i < nQuads; i++, psc [0]++) {
				p [i] = psc [0]->vPos;
				if (bTextured) {
					tc [i].v.u = psc [0]->uvl.v.u * nTiles;
					tc [i].v.v = psc [0]->uvl.v.v * nTiles;
					//glTexCoord2fv ((GLfloat *) (tc + i));
					}
				}
			RenderSphereRing (p, tc, nQuads, bTextured, GL_QUAD_STRIP);
#if 0
			if (!bTextured) {
				for (i = 0; i < nQuads; i++, psc [1]++) {
					p [i] = psc [1]->vPos;
					if (bTextured) {
						tc [i].v.u = psc [1]->uvl.v.u * nTiles;
						tc [i].v.v = psc [1]->uvl.v.v * nTiles;
						}
					}
				glLineWidth (2);
				RenderSphereRing (p, tc, nQuads, 0, GL_LINE_STRIP);
				glLineWidth (1);
				}
#endif
			}
		}
	}
else {
	for (nCull = 0; nCull < 2; nCull++) {
		glCullFace (nCull ? GL_FRONT : GL_BACK);
		psc [0] = psc [1] = sphereCoordP;
		for (j = 0; j < h; j++) {
			for (i = 0; i < nQuads; i++, psc [0]++) {
				p [i] = psc [0]->vPos;
				p[i] = p[i] * fRadius;
				G3TransformPoint(p[i], p[i], 0);
				if (bTextured) {
					tc [i].v.u = psc [0]->uvl.v.u * nTiles;
					tc [i].v.v = psc [0]->uvl.v.v * nTiles;
					}
				}
			RenderSphereRing (p, tc, nQuads, bTextured, GL_QUAD_STRIP);
#if 0
			if (!bTextured) {
				for (i = 0; i < nQuads; i++, psc [1]++) {
					p [i] = psc [1]->vPos;
					VmVecScale (p + i, p + i, fRadius);
					G3TransformPoint (p + i, p + i, 0);
					if (bTextured) {
						tc [i].v.u = psc [1]->uvl.v.u * nTiles;
						tc [i].v.v = psc [1]->uvl.v.v * nTiles;
						}
					}
				glLineWidth (2);
				RenderSphereRing (p, tc, nQuads, 0, GL_LINE_STRIP);
				glLineWidth (1);
				}
#endif
			}
		}
	}
	OglCullFace (0);
}

//------------------------------------------------------------------------------

int RenderSphere (tSphereData *sdP, tOOF_vector *pPos, float xScale, float yScale, float zScale,
					   float red, float green, float blue, float alpha, grsBitmap *bmP, int nTiles, int bAdditive)
{
	//static float fTexCoord [4][2] = {{0,0},{1,0},{1,1},{0,1}};

	float			fScale = 1.0f;
	int			bTextured = 0;
#if !SIMPLE_SPHERE
	int			i, j, nFaces = sdP->nFaces;
	tOOF_vector *ps,
					*pSphere = sdP->pSphere,
					*pRotSphere = (tOOF_vector *) D2_ALLOC (nFaces * (sdP->nFaceNodes + 1) * sizeof (tOOF_vector));

if (!pRotSphere)
	return -1;
#endif
glEnable (GL_BLEND);
if (sdP->nFaceNodes == 3)
	bmP = NULL;
else
	bTextured = InitSphereSurface (sdP, red, green, blue, alpha, bmP, &fScale);
glDepthFunc (GL_LEQUAL);
#if ADDITIVE_SPHERE_BLENDING
if (bAdditive)
	glBlendFunc (GL_ONE, GL_ONE); //_MINUS_SRC_COLOR);
else
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#else
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
#if SIMPLE_SPHERE
glTranslatef (pPos->x, pPos->y, pPos->z);
RenderSphereSimple (xScale, 32, red, green, blue, alpha, bTextured, nTiles);
#else
#	if 1
pSphere = (tOOF_vector *) RotateSphere (sdP, pRotSphere, pPos, xScale, yScale, zScale);
#	else
pSphere = (tOOF_vector *) SortSphere (RotateSphere (pSphere, pRotSphere, pPos, nFaces, xScale, yScale, zScale), 0, nFaces - 1);
#	endif
if (sdP->nFaceNodes == 3) {
	glBegin (GL_LINES);
	for (j = nFaces, ps = pSphere; j; j--, ps++)
		for (i = 0; i < 3; i++, ps++)
			glVertex3fv ((GLfloat *) ps);
	glEnd ();
	if (bmP)
		glColor4f (fScale, fScale, fScale, 1.0f);
	else
		glColor4f (red, green, blue, alpha);
	glBegin (GL_TRIANGLES);
	for (j = nFaces, ps = pSphere; j; j--, ps++)
		for (i = 0; i < 3; i++, ps++) {
			glVertex3fv ((GLfloat *) ps);
			}
	glEnd ();
	}
else {
	glBegin (GL_LINES);
	for (j = nFaces, ps = pSphere; j; j--, ps++)
		for (i = 0; i < 4; i++, ps++) {
			glVertex3fv ((GLfloat *) ps);
			}
	glEnd ();
	if (bTextured)
		glColor4f (fScale, fScale, fScale, 1.0f);
	else
		glColor4f (red, green, blue, alpha);
	glBegin (GL_QUADS);
	for (j = nFaces, ps = pSphere; j; j--, ps++)
		for (i = 0; i < 4; i++, ps++) {
			if (bTextured)
				glTexCoord2f (fTexCoord [i][0], fTexCoord [i][1]);
			glVertex3fv ((GLfloat *) ps);
			}
	glEnd ();
	}
D2_FREE (pRotSphere);
#endif //SIMPLE_SPHERE
glDepthMask (1);
glDepthFunc (GL_LESS);
return 0;
}

//------------------------------------------------------------------------------

int CreateShieldSphere (void)
{
if (!LoadShield ())
	return 0;
#if !SIMPLE_SPHERE
if (gameData.render.shield.nTessDepth != gameOpts->render.textures.nQuality + 2) {
	if (gameData.render.shield.pSphere)
		DestroySphere (&gameData.render.shield);
	gameData.render.shield.nTessDepth = gameOpts->render.textures.nQuality + 2;
	}
if (!gameData.render.shield.pSphere)
	gameData.render.shield.nFaces = CreateSphere (&gameData.render.shield);
#endif
return 1;
}

//------------------------------------------------------------------------------

void DrawShieldSphere (tObject *objP, float red, float green, float blue, float alpha)
{
if (!CreateShieldSphere ())
	return;
#if !SIMPLE_SPHERE
if (gameData.render.shield.nFaces > 0)
#endif
	{
	if ((gameOpts->render.bDepthSort > 0) || (RENDERPATH && !gameOpts->render.bDepthSort))
		RIAddSphere (riSphereShield, red, green, blue, alpha, objP);
	else {
		tOOF_vector	p = {0, 0, 0};
		fix nSize = gameData.models.polyModels [objP->rType.polyObjInfo.nModel].rad;
		float	fScale, r = X2F (nSize) /** 1.05f*/;
		tTransformation *posP = OBJPOS (objP);
		vmsVector vPos;
		//gameStates.ogl.bUseTransform = 1;
		glBlendFunc (GL_ONE, GL_ONE);
		G3StartInstanceMatrix (*PolyObjPos (objP, &vPos), posP->mOrient);
		RenderSphere (&gameData.render.shield, &p, r, r, r, red, green, blue, alpha, bmpShield, 1, 1);
		G3DoneInstance ();
		gameStates.ogl.bUseTransform = 0;
		fScale = gameData.render.shield.pPulse->fScale;
		G3StartInstanceMatrix (vPos, posP->mOrient);
		vPos.SetZero();
		RenderObjectHalo (&vPos, 3 * nSize / 2, red * fScale, green * fScale, blue * fScale, alpha * fScale, 0);
		G3DoneInstance ();
		}
	}
}

//------------------------------------------------------------------------------

void DrawMonsterball (tObject *objP, float red, float green, float blue, float alpha)
{
#if !SIMPLE_SPHERE
if (!gameData.render.monsterball.pSphere) {
	gameData.render.monsterball.nTessDepth = 3;
	gameData.render.monsterball.nFaces = CreateSphere (&gameData.render.monsterball);
	}
if (gameData.render.monsterball.nFaces > 0)
#endif
	{
	if ((gameOpts->render.bDepthSort > 0) || (RENDERPATH && !gameOpts->render.bDepthSort))
		RIAddSphere (riMonsterball, red, green, blue, alpha, objP);
	else {
		static tOOF_vector p = {0,0,0};
		float r = X2F (objP->info.xSize);
		gameStates.ogl.bUseTransform = 1;
		OglSetupTransform (0);
		G3StartInstanceMatrix(objP->info.position.vPos, objP->info.position.mOrient);
		RenderSphere (&gameData.render.monsterball, &p,
						  r, r, r, red, green, blue, gameData.hoard.monsterball.bm.bmTexBuf ? 1.0f : alpha,
						  &gameData.hoard.monsterball.bm, 4, 0);
		G3DoneInstance ();
		OglResetTransform (1);
		gameStates.ogl.bUseTransform = 0;
		}
	}
}

//------------------------------------------------------------------------------

void DestroySphere (tSphereData *sdP)
{
if (sdP) {
	D2_FREE (sdP->pSphere);
	sdP->nFaces = 0;
	}
}

//------------------------------------------------------------------------------

void DestroyShieldSphere (void)
{
DestroySphere (&gameData.render.shield);
}

//------------------------------------------------------------------------------

void DestroyMonsterball (void)
{
DestroySphere (&gameData.render.monsterball);
}

//------------------------------------------------------------------------------

void UseSpherePulse (tSphereData *sdP, tPulseData *pPulse)
{
sdP->pPulse = pPulse;
}

//------------------------------------------------------------------------------

void SetSpherePulse (tPulseData *pPulse, float fSpeed, float fMin)
{
pPulse->fScale =
pPulse->fMin = fMin;
pPulse->fSpeed =
pPulse->fDir = fSpeed;
}

//------------------------------------------------------------------------------

void InitSpheres (void)
{
PrintLog ("   creating spheres\n");
CreateShieldSphere ();
#if SIMPLE_SPHERE
CreateSphereSimple (32);
#endif
}

//------------------------------------------------------------------------------
