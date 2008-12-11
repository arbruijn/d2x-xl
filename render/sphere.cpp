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

void CSphereData::Init (void)
{
#if !RINGED_SPHERE
m_nTessDepth = 0;
m_nFaces = 0;
m_nFaceNodes = 4; //tesselate using quads
#endif
m_pulseP = NULL;
}

//------------------------------------------------------------------------------

#if !RINGED_SPHERE

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

static int SplitTriangle (tOOF_triangle *pDest, tOOF_triangle *pSrc)
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

static int TesselateSphereTri (tOOF_triangle *pDest, tOOF_triangle *pSrc, int nFaces)
{
	int	i;

for (i = 0; i < nFaces; i++, pDest += 6, pSrc++)
	SplitTriangle (pDest, pSrc);
return 1;
}

//------------------------------------------------------------------------------

static int BuildSphereTri (tOOF_triangle **buf, int *pnFaces, int nTessDepth)
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

static tOOF_vector *OOF_QuadCenter (tOOF_quad *pt)
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

static int SplitQuad (tOOF_quad *pDest, tOOF_quad *pSrc)
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

static int TesselateSphereQuad (tOOF_quad *pDest, tOOF_quad *pSrc, int nFaces)
{
	int	i;

for (i = 0; i < nFaces; i++, pDest += 4, pSrc++)
	SplitQuad (pDest, pSrc);
return 1;
}

//------------------------------------------------------------------------------

static int BuildSphereQuad (tOOF_quad **buf, int *pnFaces, int nTessDepth)
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

int TesselateSphere (void)
{
	int			nFaces, i, j;
	tOOF_vector	*buf [2];

PrintLog ("Creating shield sphere\n");
if (m_nFaceNodes == 3) {
	nFaces = 8;
	j = 6;
	}
else {
	nFaces = 6;
	j = 4;
	}
for (i = 0; i < m_nTessDepth; i++)
	nFaces *= j;
for (i = 0; i < 2; i++) {
	if (!(buf [i] = new tOOF_vector [nFaces * (m_nFaceNodes + 1)])) {
		if (i)
			delete[] buf [i - 1];
		return -1;
		}
	}
j = (m_nFaceNodes == 3) ?
	 BuildSphereTri (reinterpret_cast<tOOF_triangle **> (buf), &nFaces, m_nTessDepth) :
	 BuildSphereQuad (reinterpret_cast<tOOF_quad **> (buf), &nFaces, m_nTessDepth);
delete[] buf [!j];
if (!m_texCoord.Create (nFaces * m_nFaceNodes)) {
	delete[] buf [j];
	return -1;
	}
m_vertices.SetBuffer (buf [j]);
return nFaces;
}

//------------------------------------------------------------------------------

tOOF_triangle *RotateSphere (tOOF_vector *rotSphereP, tOOF_vector *vPosP, float xScale, float yScale, float zScale)
{
	tOOF_matrix	m;
	tOOF_vector	h, v, p,
					*vertP = m_vertices.Buffer (),
					*s = rotSphereP;
	int			nFaces;

OOF_MatVms2Oof (&m, viewInfo.view[0]);
OOF_VecVms2Oof (&p, viewInfo.pos);
for (nFaces = m_nFaces * (m_nFaceNodes + 1); nFaces; nFaces--, vertP++, rotSphereP++) {
	v = *vertP;
	v.x *= xScale;
	v.y *= yScale;
	v.z *= zScale;
	OOF_VecRot (rotSphereP, OOF_VecSub (&h, &v, &p), &m);
	}
return (tOOF_triangle *) s;
}

//------------------------------------------------------------------------------

tOOF_triangle *SortSphere (tOOF_triangle *sphereP, int left, int right)
{
	int	l = left,
			r = right;
	float	m = sphereP [(l + r) / 2].c.z;

do {
	while (sphereP [l].c.z < m)
		l++;
	while (sphereP [r].c.z > m)
		r--;
	if (l <= r) {
		if (l < r) {
			tOOF_triangle h = sphereP [l];
			sphereP [l] = sphereP [r];
			sphereP [r] = h;
			}
		}
	++l;
	--r;
	} while (l <= r);
if (right > l)
   Sort (sphereP, l, right);
if (r > left)
   Sort (sphereP, left, r);
return sphereP;
}

#endif //!RINGED_SPHERE

//------------------------------------------------------------------------------

int CSphere::InitSurface (float red, float green, float blue, float alpha, CBitmap *bmP, float *pfScale)
{
	float	fScale;
	int	bTextured = 0;

if (m_pulseP) {
	static time_t	t0 = 0;
	if (gameStates.app.nSDLTicks - t0 > 25) {
		t0 = gameStates.app.nSDLTicks;
		m_pulseP->fScale += m_pulseP->fDir;
		if (m_pulseP->fScale > 1.0f) {
			m_pulseP->fScale = 1.0f;
			m_pulseP->fDir = -m_pulseP->fSpeed;
			}
		else if (m_pulseP->fScale < m_pulseP->fMin) {
			m_pulseP->fScale = m_pulseP->fMin;
			m_pulseP->fDir = m_pulseP->fSpeed;
			}
		}
	fScale = m_pulseP->fScale;
	}
else
	fScale = 1;
#if 1
if (bmP && (bmP == bmpShield)) {
	static time_t t0 = 0;
	bTextured = 1;
	if ((gameStates.app.nSDLTicks - t0 > 40) && bmP->CurFrame ()) {
		t0 = gameStates.app.nSDLTicks;
		bmP->NextFrame ();
		}
	}
#endif
if (bmP) {
	glActiveTexture (GL_TEXTURE0);
	glClientActiveTexture (GL_TEXTURE0);
	glEnable (GL_TEXTURE_2D);
	if (bmP->Bind (1, 1))
		bmP = NULL;
	else {
		if (bmP->CurFrame ())
			bmP = bmP->CurFrame ();
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
	alpha = (float) (1.0f - gameStates.render.grAlpha / (float) FADE_LEVELS);
if (alpha < 1.0f) {
#if ADDITIVE_SPHERE_BLENDING
	fScale *= coronaIntensities [gameOpts->render.coronas.nObjIntensity];
#endif
	if (m_pulseP && m_pulseP->fScale) {
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

#if RINGED_SPHERE

int CSphere::Create (int nRings, int nTiles)
{
	int				h, i, j;
	float				t1, t2, t3, a, sint1, cost1, sint2, cost2, sint3, cost3;
	tSphereVertex	*svP;

if (nRings > MAX_SPHERE_RINGS)
	nRings = MAX_SPHERE_RINGS;
if ((m_nRings == nRings) && (m_nTiles == nTiles))
	return (m_vertices.Buffer () != NULL);

m_nRings =
m_nTiles = 
m_nVertices = 0;
m_vertices.Destroy ();
h = nRings * (nRings + 1);
if (!m_vertices.Create (h))
	return 0;
m_nRings =
m_nTiles = 0;
m_nVertices = h;
h = nRings / 2;
a = (float) (2 * Pi / nRings);
svP = m_vertices.Buffer ();
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
		svP->vPos [X] = cost2 * cost3;
		svP->vPos [Y] = sint2;
		svP->vPos [Z] = cost2 * sint3;
		svP->uv.v.u = (1 - (float) i / nRings) * nTiles;
		svP->uv.v.v = ((float) (2 * j + 2) / nRings) * nTiles;
		svP++;
		svP->vPos [X] = cost1 * cost3;
		svP->vPos [Y] = sint1;
		svP->vPos [Z] = cost1 * sint3;
		svP->uv.v.u = (1 - (float) i / nRings) * nTiles;
		svP->uv.v.v = ((float) (2 * j) / nRings) * nTiles;
		svP++;
		}
	}
return 1;
}

//------------------------------------------------------------------------------

void CSphere::RenderRing (int nOffset, int nItems, int bTextured, int nPrimitive)
{
if (G3EnableClientStates (bTextured, 0, 0, GL_TEXTURE0)) {
	if (bTextured)
		glTexCoordPointer (2, GL_FLOAT, sizeof (tSphereVertex), reinterpret_cast<GLfloat*> (&m_vertices [nOffset * nItems].uv));
	glVertexPointer (3, GL_FLOAT, sizeof (tSphereVertex), reinterpret_cast<GLfloat*> (&m_vertices [nOffset * nItems].vPos));
	glDrawArrays (nPrimitive, 0, nItems);
	G3DisableClientStates (bTextured, 0, 0, GL_TEXTURE0);
	}
else {
	glBegin (nPrimitive);
	for (int i = 0, j = nOffset * nItems; i < nItems; i++, j++) {
		if (bTextured)
			glTexCoord2fv (reinterpret_cast<GLfloat*> (&m_vertices [j].uv));
		glVertex3fv (reinterpret_cast<GLfloat*> (&m_vertices [j].vPos));
		}
	glEnd ();
	}
}

//------------------------------------------------------------------------------

void CSphere::RenderRing (CFloatVector *vertexP, tTexCoord2f *texCoordP, int nItems, int bTextured, int nPrimitive)
{
if (G3EnableClientStates (bTextured, 0, 0, GL_TEXTURE0)) {
	if (bTextured)
		glTexCoordPointer (2, GL_FLOAT, 0, texCoordP);
	glVertexPointer (3, GL_FLOAT, 0, vertexP);
	glDrawArrays (nPrimitive, 0, nItems);
	G3DisableClientStates (bTextured, 0, 0, GL_TEXTURE0);
	}
else {
	glBegin (nPrimitive);
	for (int i = 0; i < nItems; i++) {
		if (bTextured)
			glTexCoord2fv (reinterpret_cast<GLfloat*> (texCoordP + i));
		glVertex3fv (reinterpret_cast<GLfloat*> (vertexP + i));
		}
	glEnd ();
	}
}

//------------------------------------------------------------------------------

void CSphere::RenderRings (float fRadius, int nRings, float red, float green, float blue, float alpha, int bTextured, int nTiles)
{
	int				nCull, h, i, j, nQuads;
	CFloatVector	p [2 * MAX_SPHERE_RINGS + 2];
	tTexCoord2f		tc [2 * MAX_SPHERE_RINGS + 2];
	tSphereVertex	*svP [2];

if (nRings > MAX_SPHERE_RINGS)
	nRings = MAX_SPHERE_RINGS;
if (gameStates.ogl.bUseTransform)
	glScalef (fRadius, fRadius, fRadius);
if (!Create (nRings, nTiles))
	return;
h = nRings / 2;
nQuads = 2 * nRings + 2;
if (gameStates.ogl.bUseTransform) {
	for (nCull = 0; nCull < 2; nCull++) {
		svP [0] = svP [1] = m_vertices.Buffer ();
		glCullFace (nCull ? GL_FRONT : GL_BACK);
		for (i = 0; i < h; i++) {
			RenderRing (i, nQuads, bTextured, GL_QUAD_STRIP);
#if 0
			if (!bTextured) {
				for (i = 0; i < nQuads; i++, svP [1]++) {
					p [i] = svP [1]->vPos;
					if (bTextured) {
						tc [i].v.u = svP [1]->uv.v.u * nTiles;
						tc [i].v.v = svP [1]->uv.v.v * nTiles;
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
		svP [0] = svP [1] = &m_vertices [0];
		for (j = 0; j < h; j++) {
			for (i = 0; i < nQuads; i++, svP [0]++) {
				p [i] = svP [0]->vPos * fRadius;
				G3TransformPoint (p[i], p[i], 0);
				if (bTextured)
					tc [i] = svP [0]->uv;
				}
			RenderRing (p, tc, nQuads, bTextured, GL_QUAD_STRIP);
#if 0
			if (!bTextured) {
				for (i = 0; i < nQuads; i++, svP [1]++) {
					p [i] = svP [1]->vPos;
					VmVecScale (p + i, p + i, fRadius);
					G3TransformPoint (p + i, p + i, 0);
					if (bTextured) {
						tc [i].v.u = svP [1]->uv.v.u * nTiles;
						tc [i].v.v = svP [1]->uv.v.v * nTiles;
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

#else //!RINGED_SPHERE

int CSphere::RenderTesselated (tOOF_vector *vPosP, float xScale, float yScale, float zScale,
										 float red, float green, float blue, float alpha, CBitmap *bmP)
{
	int			i, j, nFaces = m_nFaces;
	tOOF_vector *ps,
					*sphereP = m_sphere,
					*rotSphereP = new tOOF_vector [nFaces * (m_nFaceNodes + 1)];

if (!rotSphereP)
	return -1;
#	if 1
sphereP = reinterpret_cast<tOOF_vector*> (Rotate (rotSphereP, vPosP, xScale, yScale, zScale));
#	else
sphereP = reinterpret_cast<tOOF_vector*> (Sort (Rotate (rotSphereP, vPosP, nFaces, xScale, yScale, zScale), 0, nFaces - 1));
#	endif
if (m_nFaceNodes == 3) {
	glBegin (GL_LINES);
	for (j = nFaces, ps = sphereP; j; j--, ps++)
		for (i = 0; i < 3; i++, ps++)
			glVertex3fv (reinterpret_cast<GLfloat*> (ps));
	glEnd ();
	if (bmP)
		glColor4f (red, green, blue, 1.0f);
	else
		glColor4f (red, green, blue, alpha);
	glBegin (GL_TRIANGLES);
	for (j = nFaces, ps = sphereP; j; j--, ps++)
		for (i = 0; i < 3; i++, ps++) {
			glVertex3fv (reinterpret_cast<GLfloat*> (ps));
			}
	glEnd ();
	}
else {
	glBegin (GL_LINES);
	for (j = nFaces, ps = sphereP; j; j--, ps++)
		for (i = 0; i < 4; i++, ps++) {
			glVertex3fv (reinterpret_cast<GLfloat*> (ps));
			}
	glEnd ();
	if (bTextured)
		glColor4f (fScale, fScale, fScale, 1.0f);
	else
		glColor4f (red, green, blue, alpha);
	glBegin (GL_QUADS);
	for (j = nFaces, ps = sphereP; j; j--, ps++)
		for (i = 0; i < 4; i++, ps++) {
			if (bTextured)
				glTexCoord2f (fTexCoord [i][0], fTexCoord [i][1]);
			glVertex3fv (reinterpret_cast<GLfloat*> (ps));
			}
	glEnd ();
	}
delete[] rotSphereP;
}

#endif

//------------------------------------------------------------------------------

int CSphere::Render (tOOF_vector *vPosP, float xScale, float yScale, float zScale,
							float red, float green, float blue, float alpha, CBitmap *bmP, int nTiles, int bAdditive)
{
	float	fScale = 1.0f;
	int	bTextured = 0;

glEnable (GL_BLEND);
#if !RINGED_SPHERE
if (m_nFaceNodes == 3)
	bmP = NULL;
else
#endif
	bTextured = InitSurface (red, green, blue, alpha, bmP, &fScale);
glDepthFunc (GL_LEQUAL);
#if ADDITIVE_SPHERE_BLENDING
if (bAdditive)
	glBlendFunc (GL_ONE, GL_ONE); //_MINUS_SRC_COLOR);
else
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#else
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
#if RINGED_SPHERE
glTranslatef (vPosP->x, vPosP->y, vPosP->z);
RenderRings (xScale, 32, red, green, blue, alpha, bTextured, nTiles);
#else
RenderTesselated (vPosP, xScale, yScale, zScale, red, green, blue, alpha, bmP);
#endif //RINGED_SPHERE
glDepthMask (1);
glDepthFunc (GL_LESS);
return 0;
}

//------------------------------------------------------------------------------

void CSphere::Destroy (void)
{
m_vertices.Destroy ();
Init ();
}

//------------------------------------------------------------------------------

void CSphere::SetPulse (CPulseData* pulseP)
{
m_pulseP = pulseP;
}

//------------------------------------------------------------------------------

void CSphere::SetupPulse (float fSpeed, float fMin)
{
m_pulse.fScale =
m_pulse.fMin = fMin;
m_pulse.fSpeed =
m_pulse.fDir = fSpeed;
}

//------------------------------------------------------------------------------

void SetupSpherePulse (CPulseData *pulseP, float fSpeed, float fMin)
{
pulseP->fScale =
pulseP->fMin = fMin;
pulseP->fSpeed =
pulseP->fDir = fSpeed;
}

//------------------------------------------------------------------------------

int CreateShieldSphere (void)
{
if (!LoadShield ())
	return 0;
#if RINGED_SPHERE
gameData.render.shield.Destroy ();
gameData.render.shield.Create (32, 1);
#else
if (gameData.render.shield.nTessDepth != gameOpts->render.textures.nQuality + 2) {
	gameData.render.shield.Destroy ();
	gameData.render.shield.nTessDepth = gameOpts->render.textures.nQuality + 2;
	}
if (!gameData.render.shield.sphereP)
	gameData.render.shield.nFaces = gameData.render.shield.Create ();
#endif
return 1;
}

//------------------------------------------------------------------------------

void DrawShieldSphere (CObject *objP, float red, float green, float blue, float alpha)
{
if (!CreateShieldSphere ())
	return;
#if !RINGED_SPHERE
if (gameData.render.shield.nFaces > 0)
#endif
	{
	if ((gameOpts->render.bDepthSort > 0) || (RENDERPATH && !gameOpts->render.bDepthSort))
		TIAddSphere (riSphereShield, red, green, blue, alpha, objP);
	else {
		tOOF_vector	p = {0, 0, 0};
		fix nSize = gameData.models.polyModels [objP->rType.polyObjInfo.nModel].rad;
		float	fScale, r = X2F (nSize) /** 1.05f*/;
		tTransformation *posP = OBJPOS (objP);
		CFixVector vPos;
		//gameStates.ogl.bUseTransform = 1;
		glBlendFunc (GL_ONE, GL_ONE);
		G3StartInstanceMatrix (*PolyObjPos (objP, &vPos), posP->mOrient);
		gameData.render.shield.Render (&p, r, r, r, red, green, blue, alpha, bmpShield, 1, 1);
		G3DoneInstance ();
		gameStates.ogl.bUseTransform = 0;
		fScale = gameData.render.shield.Pulse ()->fScale;
		G3StartInstanceMatrix (vPos, posP->mOrient);
		vPos.SetZero();
		RenderObjectHalo (&vPos, 3 * nSize / 2, red * fScale, green * fScale, blue * fScale, alpha * fScale, 0);
		G3DoneInstance ();
		}
	}
}

//------------------------------------------------------------------------------

void DrawMonsterball (CObject *objP, float red, float green, float blue, float alpha)
{
#if !RINGED_SPHERE
if (!gameData.render.monsterball.sphereP) {
	gameData.render.monsterball.nTessDepth = 3;
	gameData.render.monsterball.nFaces = gameData.render.monsterball.Create ();
	}
if (gameData.render.monsterball.nFaces > 0)
#endif
	{
	if ((gameOpts->render.bDepthSort > 0) || (RENDERPATH && !gameOpts->render.bDepthSort))
		TIAddSphere (riMonsterball, red, green, blue, alpha, objP);
	else {
		static tOOF_vector p = {0,0,0};
		float r = X2F (objP->info.xSize);
		gameStates.ogl.bUseTransform = 1;
		OglSetupTransform (0);
		G3StartInstanceMatrix (objP->info.position.vPos, objP->info.position.mOrient);
		gameData.render.monsterball.Render (&p, r, r, r, red, green, blue, gameData.hoard.monsterball.bm.Buffer () ? 1.0f : alpha,
														&gameData.hoard.monsterball.bm, 4, 0);
		G3DoneInstance ();
		OglResetTransform (1);
		gameStates.ogl.bUseTransform = 0;
		}
	}
}

//------------------------------------------------------------------------------

void DestroyShieldSphere (void)
{
gameData.render.shield.Destroy ();
}

//------------------------------------------------------------------------------

void DestroyMonsterball (void)
{
gameData.render.monsterball.Destroy ();
}

//------------------------------------------------------------------------------

void InitSpheres (void)
{
PrintLog ("   creating spheres\n");
CreateShieldSphere ();
}

//------------------------------------------------------------------------------
