//------------------------------------------------------------------------------

#include <math.h>

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#include "inferno.h"
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

static int bWireSphere = 0;

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

int BuildSphereTri (tOOF_triangle **buf, int *pnFaces)
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
for (i = 0, j = 1; i < gameData.render.sphere.nTessDepth; i++, nFaces *= 6, j = !j) {
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

int BuildSphereQuad (tOOF_quad **buf, int *pnFaces)
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
for (i = 0, j = 1; i < gameData.render.sphere.nTessDepth; i++, nFaces *= 4, j = !j) {
	TesselateSphereQuad (buf [j], buf [!j], nFaces);
	}
*pnFaces = nFaces;
return !j;
}

//------------------------------------------------------------------------------

int CreateSphere (tOOF_vector **pSphere)
{
	int			nFaces, i, j;
	tOOF_vector	*buf [2];

if (gameData.render.sphere.nFaceNodes == 3) {
	nFaces = 8;
	j = 6;
	}
else {
	nFaces = 6;
	j = 4;
	}
for (i = 0; i < gameData.render.sphere.nTessDepth; i++)
	nFaces *= j;
for (i = 0; i < 2; i++) {
	if (!(buf [i] = (tOOF_vector *) d_malloc (nFaces * (gameData.render.sphere.nFaceNodes + 1) * sizeof (tOOF_vector)))) {
		if (i)
			d_free (buf [i - 1]);
		return -1;
		}
	}
j = (gameData.render.sphere.nFaceNodes == 3) ? 
	 BuildSphereTri ((tOOF_triangle **) buf, &nFaces) : 
	 BuildSphereQuad ((tOOF_quad **) buf, &nFaces);
d_free (buf [!j]);
*pSphere = buf [j];
return nFaces;
}

//------------------------------------------------------------------------------

tOOF_triangle *RotateSphere (tOOF_vector *pSphere, tOOF_vector *pRotSphere, tOOF_vector *pPos, 
									  int nFaces, float fRad)
{
	tOOF_matrix	m;
	tOOF_vector	h, v, p, *s = pRotSphere;

OOF_MatVms2Oof (&m, &viewInfo.view);
OOF_VecVms2Oof (&p, &viewInfo.position);
for (nFaces *= (gameData.render.sphere.nFaceNodes + 1); nFaces; nFaces--, pSphere++, pRotSphere++) {
	v = *pSphere;
	OOF_VecScale (&v, fRad);
	OOF_VecRot (pRotSphere, OOF_VecInc (OOF_VecSub (&h, &v, &p), pPos), &m);
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

int RenderSphere (tOOF_vector *pPos, tOOF_vector *pSphere, int nFaces, float fRad,
					   float red, float green, float blue, float alpha)
{
	int			i;
	tOOF_vector *pRotSphere = (tOOF_vector *) d_malloc (nFaces * (gameData.render.sphere.nFaceNodes + 1) * sizeof (tOOF_vector));

if (!pRotSphere)
	return -1;
if (gameData.render.sphere.pPulse && gameStates.app.b40fpsTick) {
	gameData.render.sphere.pPulse->fScale += gameData.render.sphere.pPulse->fDir;
	if (gameData.render.sphere.pPulse->fScale > 1.0f) {
		gameData.render.sphere.pPulse->fScale = 1.0f;
		gameData.render.sphere.pPulse->fDir = -gameData.render.sphere.pPulse->fSpeed;
		}
	else if (gameData.render.sphere.pPulse->fScale < gameData.render.sphere.pPulse->fMin) {
		gameData.render.sphere.pPulse->fScale = gameData.render.sphere.pPulse->fMin;
		gameData.render.sphere.pPulse->fDir = gameData.render.sphere.pPulse->fSpeed;
		}
	}
#if 1
pSphere = (tOOF_vector *) RotateSphere (pSphere, pRotSphere, pPos, nFaces, fRad);
#else
pSphere = (tOOF_vector *) SortSphere (RotateSphere (pSphere, pRotSphere, pPos, nFaces, fRad), 0, nFaces - 1);
#endif
glDisable (GL_TEXTURE_2D);
glEnable (GL_BLEND);
if (alpha < 0)
	alpha = (float) (1.0f - (float) gameStates.render.grAlpha / (float) GR_ACTUAL_FADE_LEVELS);
if (bWireSphere)
	glColor4f (1,1,1,1);
else 
	if (gameData.render.sphere.pPulse && gameData.render.sphere.pPulse->fScale)
		glColor4f (red * gameData.render.sphere.pPulse->fScale, 
					  green * gameData.render.sphere.pPulse->fScale, 
					  blue * gameData.render.sphere.pPulse->fScale, 
					  alpha);
	else
		glColor4f (red, green, blue, alpha);
if (gameData.render.sphere.nFaceNodes == 3) {
	if (bWireSphere)
		glBegin (GL_LINES);
	else
		glBegin (GL_TRIANGLES);
	for (; nFaces; nFaces--, pSphere++) {
		for (i = 0; i < 3; i++, pSphere++) 
			glVertex3f (pSphere->x, pSphere->y, -pSphere->z);
		}
	glEnd ();
	}
else {
	if (bWireSphere)
		glBegin (GL_LINES);
	else
		glBegin (GL_QUADS);
	for (; nFaces; nFaces--, pSphere++) {
		for (i = 0; i < 4; i++, pSphere++) 
			glVertex3f (pSphere->x, pSphere->y, -pSphere->z);
		}
	glEnd ();
	}
d_free (pRotSphere);
return 0;
}

//------------------------------------------------------------------------------

void DrawObjectSphere (object *objP, float red, float green, float blue, float alpha)
{
if (gameData.render.sphere.nTessDepth != gameOpts->render.textures.nQuality + 2) {
	if (gameData.render.sphere.pSphere)
		DestroySphere ();
	gameData.render.sphere.nTessDepth = gameOpts->render.textures.nQuality + 2;
	}
if (!gameData.render.sphere.pSphere)
	gameData.render.sphere.nFaces = CreateSphere (&gameData.render.sphere.pSphere);
if (gameData.render.sphere.nFaces > 0) {
	tOOF_vector	p;
	RenderSphere ((tOOF_vector *) OOF_VecVms2Oof (&p, &objP->pos), gameData.render.sphere.pSphere, 
					  gameData.render.sphere.nFaces, f2fl (objP->size) * 1.1f, 
					  red, green, blue, alpha);
	}
}

//------------------------------------------------------------------------------

void DestroySphere (void)
{
if (gameData.render.sphere.pSphere) {
	d_free (gameData.render.sphere.pSphere);
	gameData.render.sphere.pSphere = NULL;
	gameData.render.sphere.nFaces = 0;
	}
}

//------------------------------------------------------------------------------

void UseSpherePulse (tPulseData *pPulse)
{
gameData.render.sphere.pPulse = pPulse;
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
