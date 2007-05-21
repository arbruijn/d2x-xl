//------------------------------------------------------------------------------

#include <math.h>

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#include "inferno.h"
#include "network.h"
#include "sphere.h"
#include "u_mem.h"

#define SPHERE_MAXLAT	100    /*max number of horiz and vert. divisions of sphere*/
#define SPHERE_MAXLONG	100

#define SPHERE_BIGNUM 999.0
#define TORAD(x)    ((x)*M_PI/180.0f)

#define	SQRT2 1.414213562f
#define  ASPECT 1.0f //(4.0f / 3.0f)

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

l = (float) sqrt (2) / 2.0f;
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

l = (float) sqrt (2) / 2.0f;
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
	if (!(buf [i] = (tOOF_vector *) d_malloc (nFaces * (sdP->nFaceNodes + 1) * sizeof (tOOF_vector)))) {
		if (i)
			d_free (buf [i - 1]);
		return -1;
		}
	}
j = (sdP->nFaceNodes == 3) ? 
	 BuildSphereTri ((tOOF_triangle **) buf, &nFaces, sdP->nTessDepth) : 
	 BuildSphereQuad ((tOOF_quad **) buf, &nFaces, sdP->nTessDepth);
d_free (buf [!j]);
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

OOF_MatVms2Oof (&m, viewInfo.view);
OOF_VecVms2Oof (&p, &viewInfo.pos);
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

int RenderSphere (tSphereData *sdP, tOOF_vector *pPos, float xScale, float yScale, float zScale,
					   float red, float green, float blue, float alpha, grsBitmap *bmP)
{
	static float fTexCoord [4][2] = {{0,0},{1,0},{1,1},{0,1}};

	int			i, j, nFaces = sdP->nFaces;
	tOOF_vector *ps,
					*pSphere = sdP->pSphere, 
					*pRotSphere = (tOOF_vector *) d_malloc (nFaces * (sdP->nFaceNodes + 1) * sizeof (tOOF_vector));
	float			fScale = 1.0;

if (!pRotSphere)
	return -1;
if (sdP->pPulse) {
	if (gameStates.app.tick40fps.bTick) {
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
#if 1
pSphere = (tOOF_vector *) RotateSphere (sdP, pRotSphere, pPos, xScale, yScale, zScale);
#else
pSphere = (tOOF_vector *) SortSphere (RotateSphere (pSphere, pRotSphere, pPos, nFaces, xScale, yScale, zScale), 0, nFaces - 1);
#endif
glDepthFunc (GL_LEQUAL);
glEnable (GL_BLEND);
OglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
if (sdP->nFaceNodes == 3)
	bmP = NULL;
else if (bmP) {
	OglActiveTexture (GL_TEXTURE0_ARB);
	glEnable (GL_TEXTURE_2D);
	if (OglBindBmTex (bmP, 1, 1))
		bmP = NULL;
	else if (BM_CURFRAME (bmP))
		bmP = BM_CURFRAME (bmP);
	}
if (!bmP) {
	glDisable (GL_TEXTURE_2D);
	glDepthMask (0);
	}
if (alpha < 0)
	alpha = (float) (1.0f - gameStates.render.grAlpha / (float) GR_ACTUAL_FADE_LEVELS);
if (sdP->pPulse && sdP->pPulse->fScale) {
	red *= fScale;
	green *= fScale;
	blue *= fScale; 
	}
glColor4f (red, green, blue, alpha / 2);
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
	if (bmP)
		glColor4f (fScale, fScale, fScale, 1.0f);
	else
		glColor4f (red, green, blue, alpha);
	glBegin (GL_QUADS);
	for (j = nFaces, ps = pSphere; j; j--, ps++)
		for (i = 0; i < 4; i++, ps++) {
			if (bmP)
				glTexCoord2f (fTexCoord [i][0], fTexCoord [i][1]);
			glVertex3fv ((GLfloat *) ps);
			}
	glEnd ();
	}
glDepthMask (1);
glDepthFunc (GL_LESS);
d_free (pRotSphere);
return 0;
}

//------------------------------------------------------------------------------

void DrawShieldSphere (tObject *objP, float red, float green, float blue, float alpha)
{
if (gameData.render.shield.nTessDepth != gameOpts->render.textures.nQuality + 2) {
	if (gameData.render.shield.pSphere)
		DestroySphere (&gameData.render.shield);
	gameData.render.shield.nTessDepth = gameOpts->render.textures.nQuality + 2;
	}
if (!gameData.render.shield.pSphere)
	gameData.render.shield.nFaces = CreateSphere (&gameData.render.shield);
if (gameData.render.shield.nFaces > 0) {
	tOOF_vector	p;
	float	r = f2fl (objP->size) * 1.05f;
	G3StartInstanceMatrix (&objP->position.vPos, &objP->position.mOrient);
	RenderSphere (&gameData.render.shield, (tOOF_vector *) OOF_VecVms2Oof (&p, &objP->position.vPos),
					  r, r, r, red, green, blue, alpha, NULL);
	G3DoneInstance ();
	}
}

//------------------------------------------------------------------------------

void DrawMonsterball (tObject *objP, float red, float green, float blue, float alpha)
{
if (!gameData.render.monsterball.pSphere) {
	gameData.render.monsterball.nTessDepth = 3;
	gameData.render.monsterball.nFaces = CreateSphere (&gameData.render.monsterball);
	}
if (gameData.render.monsterball.nFaces > 0) {
	tOOF_vector	p;
	float r = f2fl (objP->size);
	G3StartInstanceMatrix (&objP->position.vPos, &objP->position.mOrient);
	RenderSphere (&gameData.render.monsterball, (tOOF_vector *) OOF_VecVms2Oof (&p, &objP->position.vPos), 
					  r, r, r, red, green, blue, alpha, &gameData.hoard.monsterball.bm);
	G3DoneInstance ();
	}
}

//------------------------------------------------------------------------------

void DestroySphere (tSphereData *sdP)
{
if (sdP) {
	d_free (sdP->pSphere);
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
