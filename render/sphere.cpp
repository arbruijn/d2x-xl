// -----------------------------------------------------------------------------

#include <math.h>

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#include "descent.h"
#include "error.h"
#include "network.h"
#include "sphere.h"
#include "rendermine.h"
#include "maths.h"
#include "u_mem.h"
#include "glare.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "objeffects.h"
#include "objrender.h"
#include "transprender.h"
#include "oof.h"

#define ADDITIVE_SPHERE_BLENDING 1
#define MAX_SPHERE_RINGS 256

#if !RINGED_SPHERE

// TODO: Create a c-tor for the two tables

OOF::CTriangle baseSphereOcta [8] = {
 {{{-1,0,1},{1,0,1},{0,1,0}},{0,0,0}},
 {{{1,0,1},{1,0,-1},{0,1,0}},{0,0,0}},
 {{{1,0,-1},{-1,0,-1},{0,1,0}},{0,0,0}},
 {{{-1,0,-1},{-1,0,1},{0,1,0}},{0,0,0}},
 {{{1,0,1},{-1,0,1},{0,-1,0}},{0,0,0}},
 {{{1,0,-1},{1,0,1},{0,-1,0}},{0,0,0}},
 {{{-1,0,-1},{1,0,-1},{0,-1,0}},{0,0,0}},
 {{{-1,0,1},{-1,0,-1},{0,-1,0}},{0,0,0}}
};

OOF::CQuad baseSphereCube [6] = {
 {{{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}},{0,0,0}},
 {{{1,-1,1},{1,-1,-1},{1,1,-1},{1,1,1}},{0,0,0}},
 {{{1,-1,-1},{-1,-1,-1},{-1,1,-1},{1,1,-1}},{0,0,0}},
 {{{-1,-1,1},{-1,1,1},{-1,1,-1},{-1,-1,-1}},{0,0,0}},
 {{{-1,1,1},{1,1,1},{1,1,-1},{-1,1,-1}},{0,0,0}},
 {{{-1,-1,-1},{1,-1,-1},{1,-1,1},{-1,-1,1}},{0,0,0}}
};

#endif

// -----------------------------------------------------------------------------

const char *pszSphereFS =
	"uniform sampler2D sphereTex;\r\n" \
	"uniform vec4 vHit [3];\r\n" \
	"uniform vec3 fRad;\r\n" \
	"varying vec3 vertPos;\r\n" \
	"void main() {\r\n" \
	"vec3 scale;\r\n" \
	"scale.x = 1.0 - clamp (length (vertPos - vec3 (vHit [0])) * fRad.x, 0.0, 1.0);\r\n" \
	"scale.y = 1.0 - clamp (length (vertPos - vec3 (vHit [1])) * fRad.y, 0.0, 1.0);\r\n" \
	"scale.z = 1.0 - clamp (length (vertPos - vec3 (vHit [2])) * fRad.z, 0.0, 1.0);\r\n" \
	"gl_FragColor = texture2D (sphereTex, gl_TexCoord [0].xy) * gl_Color * max (scale.x, max (scale.y, scale.z));\r\n" \
	"}"
	;

// -----------------------------------------------------------------------------

const char *pszSphereVS =
	"varying vec3 vertPos;\r\n" \
	"void main() {\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_Position = ftransform();\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	vertPos = vec3 (gl_Vertex);\r\n" \
	"	}"
	;

GLhandleARB sphereShaderProg = 0;
GLhandleARB sphereFS = 0;
GLhandleARB sphereVS = 0;

// -----------------------------------------------------------------------------

int CreateSphereShader (void)
{
	int	bOk;

if (!(ogl.m_states.bShadersOk && ogl.m_states.bPerPixelLightingOk)) {
	gameStates.render.bPerPixelLighting = 0;
	return 0;
	}
if (sphereShaderProg)
	return 1;
PrintLog ("building sphere shader program\n");
if (!sphereShaderProg) {
	bOk = CreateShaderProg (&sphereShaderProg) &&
			CreateShaderFunc (&sphereShaderProg, &sphereFS, &sphereVS, pszSphereFS, pszSphereVS, 1) &&
			LinkShaderProg (&sphereShaderProg);
	if (!bOk) {
		ogl.m_states.bPerPixelLightingOk = 0;
		gameStates.render.bPerPixelLighting = 0;
		return -1;
		}
	}
return 1;
}

// -----------------------------------------------------------------------------

void ResetSphereShaders (void)
{
sphereShaderProg = 0;
}

// -----------------------------------------------------------------------------

void UnloadSphereShader (void)
{
if (gameStates.render.history.nShader != 1) {
	gameStates.render.history.nShader = -1;
	glUseProgramObject (0);
	}
}

// -----------------------------------------------------------------------------

int SetupSphereShader (CObject* objP, float alpha)
{
	int	nHits = 0;

PROF_START
if (CreateSphereShader () < 1) {
	PROF_END(ptShaderStates)
	return 0;
	}

	CObjHitInfo	hitInfo = objP->HitInfo ();
	float fSize = 1.0f + 2.0f / X2F (objP->Size ());
	float fScale [3];
	CFloatVector vHitf [3];

	tObjTransformation *posP = OBJPOS (objP);
	CFixMatrix m;
	CFixVector vPos;

if (!ogl.m_states.bUseTransform) {
	fSize *= X2F (objP->Size ());
	ogl.SetupTransform (0);
	m = CFixMatrix::IDENTITY;
	transformation.Begin (*PolyObjPos (objP, &vPos), m); 
	}
else {
	m = posP->mOrient;
	m.Transpose (m);
	m = m.Inverse ();
	}
for (int i = 0; i < 3; i++) {
	int dt = gameStates.app.nSDLTicks - int (hitInfo.t [i]);
	if (dt < SHIELD_EFFECT_TIME) {
		float h = (fSize * float (cos (sqrt (float (dt) / float (SHIELD_EFFECT_TIME)) * Pi / 2)));
		if (h > 1.0f / 1e6f) {
			fScale [i] = 1.0f / h;
			vHitf [i][W] = 0.0f;
			if (ogl.m_states.bUseTransform) {
				vHitf [i].Assign (m * hitInfo.v [i]);
				CFloatVector::Normalize (vHitf [i]);
				}
			else {
				vHitf [i].Assign (hitInfo.v [i]);
				transformation.Transform (vHitf [i], vHitf [i]);
				}
			nHits++;
			}
		else {
			fScale [i] = 1e6f;
			vHitf [i].SetZero ();
			}
		}
	else {
		fScale [i] = 1e6f;
		vHitf [i].SetZero ();
		}
	}
if (!ogl.m_states.bUseTransform) {
	transformation.End ();
	ogl.ResetTransform (1);
	}

if (!nHits)
	return 0;

if (100 + ogl.m_states.bUseTransform != gameStates.render.history.nShader) {
	gameData.render.nShaderChanges++;
	glUseProgramObject (sphereShaderProg);
	glUniform1i (glGetUniformLocation (sphereShaderProg, "shaderTex"), 0);
	}
glUniform4fv (glGetUniformLocation (sphereShaderProg, "vHit"), 3, reinterpret_cast<GLfloat*> (vHitf));
glUniform3fv (glGetUniformLocation (sphereShaderProg, "fRad"), 1, reinterpret_cast<GLfloat*> (fScale));
ogl.ClearError (0);
PROF_END(ptShaderStates)
return gameStates.render.history.nShader = 100 + ogl.m_states.bUseTransform;
}

// -----------------------------------------------------------------------------

void CSphereData::Init (void)
{
#if !RINGED_SPHERE
m_nTessDepth = 0;
m_nFaces = 0;
m_nFaceNodes = 4; //tesselate using quads
#endif
m_pulseP = NULL;
}

// -----------------------------------------------------------------------------

#if !RINGED_SPHERE

CFloatVector *OOF_TriangleCenter (OOF::CTriangle *pt)
{
pt->c = (pt->p [0] + pt->p [1] + pt->p [2]) / 3.0f;
return &pt->c;
}

// -----------------------------------------------------------------------------

static int SplitTriangle (OOF::CTriangle *pDest, OOF::CTriangle *pSrc)
{
	int	i, j;
	CFloatVector	c = pSrc->c;
	CFloatVector	h [6];

for (i = 0; i < 3; i++)
	h [2 * i] = pSrc->p [i];
for (i = 1; i < 6; i += 2)
	h [i] = h [i - 1] + h [(i + 1) % 6];
for (i = 0; i < 6; i++, pDest++) {
	pDest->p [0] = h [i];
	pDest->p [1] = h [(i + 1) % 6];
	pDest->p [2] = c;
	for (j = 0; j < 3; j++)
		CFloatVector::Normalize (pDest->p [j]);
	OOF_TriangleCenter (pDest);
	}
return 1;
}

// -----------------------------------------------------------------------------

static int TesselateSphereTri (OOF::CTriangle *pDest, OOF::CTriangle *pSrc, int nFaces)
{
	int	i;

for (i = 0; i < nFaces; i++, pDest += 6, pSrc++)
	SplitTriangle (pDest, pSrc);
return 1;
}

// -----------------------------------------------------------------------------

static int BuildSphereTri (OOF::CTriangle **buf, int *pnFaces, int nTessDepth)
{
    int		i, j, nFaces = 0;
	 float	l;

l = (float) sqrt (2.0f) / 2.0f;
for (i = 0; i < 8; i++) {
	for (j = 0; j < 3; j++) {
		buf [0][i].p [j] = baseSphereOcta [i].p [j];
		buf [0][i].p [j] *= l;
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

// -----------------------------------------------------------------------------

static CFloatVector *OOF_QuadCenter (OOF::CQuad *pt)
{
pt->c = (pt->p [0] + pt->p [1] + pt->p [2] + pt->p [3]) / 4.0f;
return &pt->c;
}

// -----------------------------------------------------------------------------

static int SplitQuad (OOF::CQuad *pDest, OOF::CQuad *pSrc)
{
	int	i, j;
	CFloatVector	c = pSrc->c;
	CFloatVector	h [8];

for (i = 0; i < 4; i++)
	h [2 * i] = pSrc->p [i];
for (i = 1; i < 8; i += 2)
	h [i] = h [i - 1] + h [(i + 1) % 8];
for (i = 0; i < 8; i += 2, pDest++) {
	pDest->p [0] = h [i ? i - 1 : 7];
	pDest->p [1] = h [i];
	pDest->p [2] = h [(i + 1) % 8];
	pDest->p [3] = c;
	for (j = 0; j < 4; j++)
		CFloatVector::Normalize (pDest->p [j]);
	OOF_QuadCenter (pDest);
	}
return 1;
}

// -----------------------------------------------------------------------------

static int TesselateSphereQuad (OOF::CQuad *pDest, OOF::CQuad *pSrc, int nFaces)
{
	int	i;

for (i = 0; i < nFaces; i++, pDest += 4, pSrc++)
	SplitQuad (pDest, pSrc);
return 1;
}

// -----------------------------------------------------------------------------

static int BuildSphereQuad (OOF::CQuad **buf, int *pnFaces, int nTessDepth)
{
    int		i, j, nFaces;
	 float	l;

l = (float) sqrt (2.0f) / 2.0f;
for (i = 0; i < 6; i++) {
	for (j = 0; j < 4; j++) {
		buf [0][i].p [j] = baseSphereCube [i].p [j];
		buf [0][i].p [j] *= l;
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

// -----------------------------------------------------------------------------

int TesselateSphere (void)
{
	int			nFaces, i, j;
	CFloatVector	*buf [2];

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
	if (!(buf [i] = new CFloatVector [nFaces * (m_nFaceNodes + 1)])) {
		if (i)
			delete[] buf [i - 1];
		return -1;
		}
	}
j = (m_nFaceNodes == 3) ?
	 BuildSphereTri (reinterpret_cast<OOF::CTriangle **> (buf), &nFaces, m_nTessDepth) :
	 BuildSphereQuad (reinterpret_cast<OOF::CQuad **> (buf), &nFaces, m_nTessDepth);
delete[] buf [!j];
if (!m_texCoord.Create (nFaces * m_nFaceNodes)) {
	delete[] buf [j];
	return -1;
	}
m_vertices.SetBuffer (buf [j]);
return nFaces;
}

// -----------------------------------------------------------------------------

OOF::CTriangle *RotateSphere (CFloatVector *rotSphereP, CFloatVector *vPosP, float xScale, float yScale, float zScale)
{
	CFloatMatrix	m;
	CFloatVector	h, v, p,
					*vertP = m_vertices.Buffer (),
					*s = rotSphereP;
	int			nFaces;

OOF_MatVms2Oof (&m, transformation.m_info.view[0]);
OOF_VecVms2Oof (&p, transformation.m_info.pos);
for (nFaces = m_nFaces * (m_nFaceNodes + 1); nFaces; nFaces--, vertP++, rotSphereP++) {
	v = *vertP;
	v.x *= xScale;
	v.y *= yScale;
	v.z *= zScale;
	rotSphereP = m * (h = v - p);
	}
return (OOF::CTriangle *) s;
}

// -----------------------------------------------------------------------------

OOF::CTriangle *SortSphere (OOF::CTriangle *sphereP, int left, int right)
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
			OOF::CTriangle h = sphereP [l];
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

// -----------------------------------------------------------------------------

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
	ogl.SelectTMU (GL_TEXTURE0, true);
	glEnable (GL_TEXTURE_2D);
	if (bmP->Bind (1))
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

// -----------------------------------------------------------------------------

#if RINGED_SPHERE

#define UV_SCALE	3.0f

int CSphere::Create (int nRings, int nTiles)
{
	int				h, i, j;
	float				t1, t2, t3, a, sint1, cost1, sint2, cost2, sint3, cost3;
	tSphereVertex	*svP;

if (nRings > MAX_SPHERE_RINGS)
	nRings = MAX_SPHERE_RINGS;
if ((m_nRings == nRings) && (m_nTiles == nTiles) && (m_vertices.Buffer () != NULL))
	return 1;

m_nRings =
m_nTiles = 
m_nVertices = 0;
m_vertices.Destroy ();
h = nRings * (nRings + 1);
if (!m_vertices.Create (2))
	return 0;
m_nRings =
m_nTiles = 0;
m_nVertices = h;
h = nRings / 2;
a = float (2 * Pi / nRings);
svP = m_vertices.Buffer ();
for (j = 0; j < h; j++) {
	t1 = float (j * a - Pi / 2);
	t2 = t1 + a;
	sint1 = float (sin (t1));
	cost1 = float (cos (t1));
	sint2 = float (sin (t2));
	cost2 = float (cos (t2));
	for (i = 0; i <= nRings; i++) {
		t3 = i * a;
		sint3 = float (sin (t3));
		cost3 = float (cos (t3));
		svP->vPos [X] = cost2 * cost3;
		svP->vPos [Y] = sint2;
		svP->vPos [Z] = cost2 * sint3;
		svP->uv.v.u = (1 - float (i) / nRings) * nTiles * UV_SCALE;
		svP->uv.v.v = (float (2 * j + 2) / nRings) * nTiles * UV_SCALE;
		svP++;
		svP->vPos [X] = cost1 * cost3;
		svP->vPos [Y] = sint1;
		svP->vPos [Z] = cost1 * sint3;
		svP->uv.v.u = (1 - float (i) / nRings) * nTiles * UV_SCALE;
		svP->uv.v.v = (float (2 * j) / nRings) * nTiles * UV_SCALE;
		svP++;
		}
	}
return 1;
}

// -----------------------------------------------------------------------------

void CSphere::RenderRing (int nOffset, int nItems, int bTextured, int nPrimitive)
{
if (ogl.EnableClientStates (bTextured, 0, 0, GL_TEXTURE0)) {
	if (bTextured)
		OglTexCoordPointer (2, GL_FLOAT, sizeof (tSphereVertex), reinterpret_cast<GLfloat*> (&m_vertices [nOffset * nItems].uv));
	OglVertexPointer (3, GL_FLOAT, sizeof (tSphereVertex), reinterpret_cast<GLfloat*> (&m_vertices [nOffset * nItems].vPos));
	OglDrawArrays (nPrimitive, 0, nItems);
	ogl.DisableClientStates (bTextured, 0, 0, GL_TEXTURE0);
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

// -----------------------------------------------------------------------------

void CSphere::RenderRing (CFloatVector *vertexP, tTexCoord2f *texCoordP, int nItems, int bTextured, int nPrimitive)
{
if (ogl.EnableClientStates (bTextured, 0, 0, GL_TEXTURE0)) {
	if (bTextured)
		OglTexCoordPointer (2, GL_FLOAT, 0, texCoordP);
	OglVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), vertexP);
	OglDrawArrays (nPrimitive, 0, nItems);
	ogl.DisableClientStates (bTextured, 0, 0, GL_TEXTURE0);
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

// -----------------------------------------------------------------------------

void CSphere::RenderRings (float fRadius, int nRings, float red, float green, float blue, float alpha, int bTextured, int nTiles)
{
	int				nCull, h, i, j, nQuads;
	CFloatVector	p [2 * MAX_SPHERE_RINGS + 2];
	tTexCoord2f		tc [2 * MAX_SPHERE_RINGS + 2];
	tSphereVertex	*svP [2];

if (nRings > MAX_SPHERE_RINGS)
	nRings = MAX_SPHERE_RINGS;
if (!Create (nRings, nTiles))
	return;
h = nRings / 2;
nQuads = 2 * nRings + 2;

if (ogl.m_states.bUseTransform) {
	glScalef (fRadius, fRadius, fRadius);
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
				transformation.Transform (p [i], p [i], 0);
				if (bTextured)
					tc [i] = svP [0]->uv;
				}
			RenderRing (p, tc, nQuads, bTextured, GL_QUAD_STRIP);
#if 0
			if (!bTextured) {
				for (i = 0; i < nQuads; i++, svP [1]++) {
					p [i] = svP [1]->vPos;
					VmVecScale (p + i, p + i, fRadius);
					transformation.Transform (p + i, p + i, 0);
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

// -----------------------------------------------------------------------------

#else //!RINGED_SPHERE

int CSphere::RenderTesselated (CFloatVector *vPosP, float xScale, float yScale, float zScale,
										 float red, float green, float blue, float alpha, CBitmap *bmP)
{
	int			i, j, nFaces = m_nFaces;
	CFloatVector *ps,
					*sphereP = m_sphere,
					*rotSphereP = new CFloatVector [nFaces * (m_nFaceNodes + 1)];

if (!rotSphereP)
	return -1;
#	if 1
sphereP = reinterpret_cast<CFloatVector*> (Rotate (rotSphereP, vPosP, xScale, yScale, zScale));
#	else
sphereP = reinterpret_cast<CFloatVector*> (Sort (Rotate (rotSphereP, vPosP, nFaces, xScale, yScale, zScale), 0, nFaces - 1));
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

// -----------------------------------------------------------------------------

int CSphere::Render (CObject* objP, CFloatVector *vPosP, float xScale, float yScale, float zScale,
							float red, float green, float blue, float alpha, CBitmap *bmP, int nTiles, int bAdditive)
{
	float	fScale = 1.0f;
	int	bTextured = 0;
	int	bEffect = (objP->info.nType == OBJ_PLAYER) || (objP->info.nType == OBJ_ROBOT);

glEnable (GL_BLEND);
#if !RINGED_SPHERE
if (m_nFaceNodes == 3)
	bmP = NULL;
else
#endif
	bTextured = InitSurface (red, green, blue, bEffect ? 1.0f : alpha, bmP, &fScale);
glDepthFunc (GL_LEQUAL);
#if ADDITIVE_SPHERE_BLENDING
if (bAdditive == 2)
	glBlendFunc (GL_ONE, GL_ONE); //_MINUS_SRC_COLOR);
else if (bAdditive == 1)
	glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_COLOR);
else
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#else
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
#if RINGED_SPHERE
#if 1
ogl.m_states.bUseTransform = 1;
if (!bEffect)
#else
if (ogl.m_states.bUseTransform = !bEffect)
#endif
	UnloadSphereShader ();
else {
	if (!SetupSphereShader (objP, alpha))
		return 0;
	}
ogl.SetupTransform (0);
tObjTransformation *posP = OBJPOS (objP);
CFixVector vPos;
transformation.Begin (*PolyObjPos (objP, &vPos), posP->mOrient);
RenderRings (xScale, 32, red, green, blue, alpha, bTextured, nTiles);
transformation.End ();
ogl.ResetTransform (0);
ogl.m_states.bUseTransform = 0;
#else
RenderTesselated (vPosP, xScale, yScale, zScale, red, green, blue, alpha, bmP);
#endif //RINGED_SPHERE
glDepthMask (1);
glDepthFunc (GL_LESS);
return 0;
}

// -----------------------------------------------------------------------------

void CSphere::Destroy (void)
{
m_vertices.Destroy ();
Init ();
}

// -----------------------------------------------------------------------------

void CSphere::SetPulse (CPulseData* pulseP)
{
m_pulseP = pulseP;
}

// -----------------------------------------------------------------------------

void CSphere::SetupPulse (float fSpeed, float fMin)
{
m_pulse.fScale =
m_pulse.fMin = fMin;
m_pulse.fSpeed =
m_pulse.fDir = fSpeed;
}

// -----------------------------------------------------------------------------

void SetupSpherePulse (CPulseData *pulseP, float fSpeed, float fMin)
{
pulseP->fScale =
pulseP->fMin = fMin;
pulseP->fSpeed =
pulseP->fDir = fSpeed;
}

// -----------------------------------------------------------------------------

int CreateShieldSphere (void)
{
if (!LoadShield ())
	return 0;
#if RINGED_SPHERE
//gameData.render.shield.Destroy ();
gameData.render.shield.Create (32, 1);
#else
if (gameData.render.shield.nTessDepth != gameOpts->render.textures.nQuality + 2) {
	gameData.render.shield.Destroy ();
	gameData.render.shield.nTessDepth = gameOpts->render.textures.nQuality + 2;
	}
if (!gameData.render.shield.sphereP)
	gameData.render.shield.nFaces = gameData.render.shield.Create ();
#endif
gameData.render.shield.SetupPulse (0.02f, 0.5f);
gameData.render.shield.SetPulse (gameData.render.shield.Pulse ());
return 1;
}

// -----------------------------------------------------------------------------

void DrawShieldSphere (CObject *objP, float red, float green, float blue, float alpha, fix nSize)
{
if (!CreateShieldSphere ())
	return;
#if !RINGED_SPHERE
if (gameData.render.shield.nFaces > 0)
#endif
 {
	if ((gameOpts->render.bDepthSort > 0) || (!gameOpts->render.bDepthSort))
		transparencyRenderer.AddSphere (riSphereShield, red, green, blue, alpha, objP, nSize);
	else {
		float	fScale;
		int	bAdditive;
		if (nSize) {
			fScale = 0.5f;
			bAdditive = 0;
			}
		else {
			if (objP->rType.polyObjInfo.nModel < 0) 
				nSize = objP->info.xSize;
			else {
				CPolyModel* modelP = GetPolyModel (objP, NULL, objP->rType.polyObjInfo.nModel, 0);
				nSize = modelP ? modelP->Rad () : objP->info.xSize;
				}
			fScale = gameData.render.shield.Pulse ()->fScale;
			bAdditive = 2;
			}
		tObjTransformation *posP = OBJPOS (objP);
		float r = X2F (nSize);
		gameData.render.shield.Render (objP, NULL, r, r, r, red, green, blue, alpha, bmpShield, 1, bAdditive);
		if (gameStates.render.history.nShader != 100) {	// full and not just partial sphere rendered
			CFixVector vPos;
			transformation.Begin (*PolyObjPos (objP, &vPos), posP->mOrient);
			vPos.SetZero ();
			RenderObjectHalo (&vPos, 3 * nSize / 2, red * fScale, green * fScale, blue * fScale, alpha * fScale, 0);
			transformation.End ();
			}
		}
	}
}

// -----------------------------------------------------------------------------

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
	if ((gameOpts->render.bDepthSort > 0) || (!gameOpts->render.bDepthSort))
		transparencyRenderer.AddSphere (riMonsterball, red, green, blue, alpha, objP);
	else {
		float r = X2F (objP->info.xSize);
		ogl.m_states.bUseTransform = 1;
		ogl.SetupTransform (0);
		transformation.Begin (objP->info.position.vPos, objP->info.position.mOrient);
		CFloatVector p;
		p.SetZero ();
		gameData.render.monsterball.Render (objP, &p, r, r, r, red, green, blue, gameData.hoard.monsterball.bm.Buffer () ? 1.0f : alpha,
														&gameData.hoard.monsterball.bm, 4, 0);
		transformation.End ();
		ogl.ResetTransform (1);
		ogl.m_states.bUseTransform = 0;
		}
	}
}

// -----------------------------------------------------------------------------

void DestroyShieldSphere (void)
{
gameData.render.shield.Destroy ();
}

// -----------------------------------------------------------------------------

void DestroyMonsterball (void)
{
gameData.render.monsterball.Destroy ();
}

// -----------------------------------------------------------------------------

void InitSpheres (void)
{
PrintLog ("   creating spheres\n");
CreateShieldSphere ();
}

// -----------------------------------------------------------------------------
