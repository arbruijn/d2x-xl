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
#include "../effects/glow.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "objeffects.h"
#include "objrender.h"
#include "ogl_render.h"
#include "transprender.h"
#include "oof.h"
#include "addon_bitmaps.h"

#define ADDITIVE_SPHERE_BLENDING 1
#define MAX_SPHERE_RINGS 256

// TODO: Create a c-tor for the two tables

float CSphereVertex::m_fNormRadScale = 1.0f; //(float) sqrt (2.0f) / 2.0f;

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

int32_t sphereShaderProg = -1;

// -----------------------------------------------------------------------------

int32_t CreateSphereShader (void)
{
if (!(ogl.m_features.bShaders && ogl.m_features.bPerPixelLighting.Available ())) {
	gameStates.render.bPerPixelLighting = 0;
	return 0;
	}
if (sphereShaderProg < 0) {
	PrintLog (1, "building sphere shader program\n");
	if (!shaderManager.Build (sphereShaderProg, pszSphereFS, pszSphereVS)) {
		ogl.m_features.bPerPixelLighting.Available (0);
		gameStates.render.bPerPixelLighting = 0;
		PrintLog (-1);
		return -1;
		}
	PrintLog (-1);
	}
return 1;
}

// -----------------------------------------------------------------------------

void ResetSphereShaders (void)
{
//sphereShaderProg = -1;
}

// -----------------------------------------------------------------------------

void UnloadSphereShader (void)
{
shaderManager.Deploy (-1);
}

// -----------------------------------------------------------------------------

int32_t SetupSphereShader (CObject* pObj, float alpha)
{
	int32_t	nHits = 0;

PROF_START
if (CreateSphereShader () < 1) {
	PROF_END(ptShaderStates)
	return 0;
	}

	CObjHitInfo	hitInfo = pObj->HitInfo ();
	float fSize = 1.0f + 2.0f / X2F (pObj->Size ());
	float fScale [3];
	CFloatVector vHitf [3];

	tObjTransformation *pPos = OBJPOS (pObj);
	CFixMatrix m;
	CFixVector m_v;

if (!ogl.UseTransform ()) {
	fSize *= X2F (pObj->Size ());
	ogl.SetupTransform (0);
	m = CFixMatrix::IDENTITY;
	transformation.Begin (*PolyObjPos (pObj, &m_v), m); 
	}
else {
	m = pPos->mOrient;
	m.Transpose (m);
	m = m.Inverse ();
	}
for (int32_t i = 0; i < 3; i++) {
	int32_t dt = gameStates.app.nSDLTicks [0] - int32_t (hitInfo.t [i]);
	if (dt < SHIELD_EFFECT_TIME) {
		float h = (fSize * float (cos (sqrt (float (dt) / float (SHIELD_EFFECT_TIME)) * PI / 2)));
		if (h > 1.0f / 1e6f) {
			fScale [i] = 1.0f / h;
			vHitf [i].v.coord.w = 0.0f;
			if (ogl.UseTransform ()) {
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
if (!ogl.UseTransform ()) {
	transformation.End ();
	ogl.ResetTransform (1);
	}

if (!nHits)
	return 0;

GLhandleARB shaderProg = GLhandleARB (shaderManager.Deploy (sphereShaderProg));
if (shaderProg) {
	if (shaderManager.Rebuild (shaderProg))
		/*nothing*/;
	glUniform1i (glGetUniformLocation (shaderProg, "sphereTex"), 0);
	//if (shaderProg) 
		{
		glUniform4fv (glGetUniformLocation (shaderProg, "vHit"), 3, reinterpret_cast<GLfloat*> (vHitf));
		glUniform3fv (glGetUniformLocation (shaderProg, "fRad"), 1, reinterpret_cast<GLfloat*> (fScale));
		}
	}
ogl.ClearError (0);
PROF_END(ptShaderStates)
return shaderManager.Current ();
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void CSphereData::Init (void)
{
m_pPulse = NULL;
m_nFrame = 0;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void CSphereVertex::Normalize (void)
{
m_v /= m_v.Mag () * m_fNormRadScale;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void CSphereFace::Normalize (CFloatVector& v)
{
v /= v.Mag () * CSphereVertex::m_fNormRadScale;
}

// -----------------------------------------------------------------------------

void CSphereFace::ComputeNormal (void)
{
m_vNormal.m_v = CFloatVector::Normal (Vertex (0), Vertex (1), Vertex (2));
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

CSphereVertex *CSphereTriangle::ComputeCenter (void)
{
m_vCenter = (m_v [0] + m_v [1] + m_v [2]) * (1.0f / 3.0f);
m_vCenter.Normalize ();
return &m_vCenter;
}

// -----------------------------------------------------------------------------

CSphereTriangle *CSphereTriangle::Split (CSphereTriangle *pDest)
{
	static int32_t o [4][3] = {{0, 3, 5}, {3, 4, 5}, {3, 1, 4}, {4, 2, 5}};

	int32_t			i, j;
	CSphereVertex	h [6];

for (i = 0; i < 3; i++)
	h [i] = m_v [i];
for (i = 0; i < 3; i++) {
	CSphereVertex v = (h [i] + h [(i + 1) % 3]) * 0.5f;
	v.Normalize ();
	h [i + 3] = v;
	}

for (i = 0; i < 4; i++, pDest++) {
	for (j = 0; j < 3; j++)
		pDest->m_v [i] = h [o [i][j]];
	}
return pDest;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

CSphereVertex *CSphereQuad::ComputeCenter (void)
{
m_vCenter = (m_v [0] + m_v [1] + m_v [2] + m_v [3]) * 0.25f;
m_vCenter.Normalize ();
return &m_vCenter;
}

// -----------------------------------------------------------------------------

inline int32_t Wrap (int32_t i, int32_t l)
{
return (i < 0) ? i + l : i % l;
}

// -----------------------------------------------------------------------------

CSphereQuad *CSphereQuad::Split (CSphereQuad *pDest)
{
	static int32_t o [4][4] = {{0, 4, 8, 7}, {4, 1, 5, 8}, {8, 5, 2, 6}, {7, 8, 6, 3}};

	int32_t			i, j;
	CSphereVertex	h [9];

ComputeCenter ();
for (i = 0; i < 4; i++)
	h [i] = m_v [i];
for (i = 0; i < 4; i++) {
	CSphereVertex v = (m_v [i] + m_v [(i + 1) % 4]) * 0.5f;
	v.Normalize ();
	h [i + 4] = v;
	}
h [8] = m_vCenter;

for (i = 0; i < 4; i++, pDest++) {
	for (j = 0; j < 4; j++)
		pDest->m_v [j] = h [o [i][j]];
	}
return pDest;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

int16_t CSphere::FindVertex (CSphereVertex& v)
{
for (int16_t i = 0; i < m_nVertices; i++)
	if (CFloatVector::Dist (m_worldVerts [i].m_v, v.m_v) < 1e-6f)
		return i;
return -1;
}

// -----------------------------------------------------------------------------

int16_t CSphere::AddVertex (CSphereVertex& v)
{
v.Normalize ();
int16_t i = FindVertex (v);
if (i < 0) {
	i = m_nVertices++;
	m_worldVerts [i] = v;
	}
return i;
}

// -----------------------------------------------------------------------------

void CSphere::SetPulse (CPulseData* pPulse)
{
m_pPulse = pPulse;
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

void CSphere::Pulsate (void)
{
if (m_pPulse) {
	static time_t	t0 = 0;
	if (gameStates.app.nSDLTicks [0] - t0 > 25) {
		t0 = gameStates.app.nSDLTicks [0];
		m_pPulse->fScale += m_pPulse->fDir;
		if (m_pPulse->fScale > 1.0f) {
			m_pPulse->fScale = 1.0f;
			m_pPulse->fDir = -m_pPulse->fSpeed;
			}
		else if (m_pPulse->fScale < m_pPulse->fMin) {
			m_pPulse->fScale = m_pPulse->fMin;
			m_pPulse->fDir = m_pPulse->fSpeed;
			}
		}
	}
}

// -----------------------------------------------------------------------------

void CSphere::Animate (CBitmap* pBm)
{
#if 1
if (pBm && (pBm == shield.Bitmap ())) {
	static CTimeout to (100);
	if (to.Expired () && pBm->CurFrame ()) 
		pBm->NextFrame ();
	}
#endif
}

// -----------------------------------------------------------------------------

int32_t CSphere::InitSurface (float red, float green, float blue, float alpha, CBitmap *pBm, float fScale)
{
	int32_t	bTextured = pBm != NULL;

gameStates.render.EnableCartoonStyle ();
fScale = m_pPulse ? m_pPulse->fScale : 1.0f;
ogl.ResetClientStates (0);
if (pBm) {
	Animate (pBm);
	ogl.EnableClientStates (bTextured, 0, 0, GL_TEXTURE0);
	if (pBm->CurFrame ())
		pBm = pBm->CurFrame ();
	if (pBm->Bind (1))
		pBm = NULL;
	else {
		if (!bTextured)
			bTextured = -1;
		}
	}
if (!pBm) {
	ogl.SetTexturing (false);
	bTextured = 0;
	alpha /= 2;
	ogl.EnableClientStates (0, 0, 0, GL_TEXTURE0);
	}
if (alpha < 0)
	alpha = (float) (1.0f - gameStates.render.grAlpha / (float) FADE_LEVELS);
if (alpha < 1.0f) {
#if ADDITIVE_SPHERE_BLENDING
	fScale *= coronaIntensities [gameOpts->render.coronas.nObjIntensity];
#endif
	if (m_pPulse && m_pPulse->fScale) {
		red *= fScale;
		green *= fScale;
		blue *= fScale;
		}
	}
ogl.SetDepthWrite (false);
m_pBm = pBm;
m_color.Set (red, green, blue, alpha);
gameStates.render.DisableCartoonStyle ();
return bTextured;
}

// -----------------------------------------------------------------------------

void CSphere::DrawFaces (int32_t nOffset, int32_t nFaces, int32_t bTextured, int32_t nPrimitive)
{
int32_t nVertices = nFaces * FaceNodes ();
ogl.EnableClientStates (bTextured, 0, 0, GL_TEXTURE0);
if (bTextured && !m_pBm->Bind (1))
	OglTexCoordPointer (2, GL_FLOAT, sizeof (CSphereVertex), reinterpret_cast<GLfloat*> (&m_worldVerts [nOffset * nVertices].m_tc));
OglVertexPointer (3, GL_FLOAT, sizeof (CSphereVertex), reinterpret_cast<GLfloat*> (&m_worldVerts [nOffset * nVertices].m_v));
#if 0 //DBG
glColor3f (1.0f, 0.5f, 0.0f);
#else
glColor4fv ((GLfloat*) m_color.v.vec);
#endif
OglDrawArrays (nPrimitive, 0, nVertices);
ogl.DisableClientStates (bTextured, 0, 0, GL_TEXTURE0);
}

// -----------------------------------------------------------------------------

void CSphere::DrawFaces (CFloatVector *pVertex, tTexCoord2f *pTexCoord, int32_t nFaces, int32_t bTextured, int32_t nPrimitive)
{
ogl.EnableClientStates (bTextured, 0, 0, GL_TEXTURE0);
if (bTextured && !m_pBm->Bind (1))
	OglTexCoordPointer (2, GL_FLOAT, 0, pTexCoord);
OglVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), pVertex);
glColor4fv ((GLfloat*) m_color.v.vec);
OglDrawArrays (nPrimitive, 0, nFaces * FaceNodes ());
ogl.DisableClientStates (bTextured, 0, 0, GL_TEXTURE0);
}

// -----------------------------------------------------------------------------

int32_t CSphere::Render (CObject *pObj, CFloatVector *pvPos, float xScale, float yScale, float zScale,
								 float red, float green, float blue, float alpha, CBitmap *bmP, int32_t nFaces, char bAdditive)
{
	float	fScale = 1.0f;
	int32_t	bTextured = 0;
#if 0 //DBG
	int32_t	bEffect = 0;
#else
	int32_t	bAppearing = pObj->Appearing ();
	int32_t	bEffect = (pObj->info.nType == OBJ_PLAYER) || (pObj->info.nType == OBJ_ROBOT);
	int32_t	bGlow = /*!bAppearing &&*/ (bAdditive != 0) && glowRenderer.Available (GLOW_SHIELDS);
#endif

CFixVector vPos;
PolyObjPos (pObj, &vPos);
if (bAppearing) {
	float scale = pObj->AppearanceScale ();
	red *= scale;
	green *= scale;
	blue *= scale;
	}
Pulsate ();
if (bGlow) {
	glowRenderer.Begin (GLOW_SHIELDS, 2, false, 0.85f);
	if (!glowRenderer.SetViewport (GLOW_SHIELDS, vPos, 4 * xScale / 3)) {
		glowRenderer.Done (GLOW_SHIELDS);
		ogl.SetDepthMode (GL_LEQUAL);
		return 0;
		}
	ogl.SetBlendMode (OGL_BLEND_REPLACE);
	}
else {
	ogl.SetBlendMode (bAdditive > 0);
	}
ogl.SetDepthMode (GL_LEQUAL);

ogl.SetTransform (1);
if (bAppearing) {
	UnloadSphereShader ();
	float scale = pObj->AppearanceScale ();
	scale = Min (1.0f, (float) pow (1.0f - scale, 0.25f));
#if 1
	xScale *= scale;
	yScale *= scale;
	zScale *= scale;
#endif
	}
else if (!bEffect)
	UnloadSphereShader ();
else if (gameOpts->render.bUseShaders && ogl.m_features.bShaders.Available ()) {
	if (!SetupSphereShader (pObj, alpha)) {
		if (bGlow)
			glowRenderer.Done (GLOW_SHIELDS);
		return 0;
		}
	}

#if DBG
bTextured = InitSurface (red, green, blue, bEffect ? 1.0f : alpha, bmP, fScale);
#else
bTextured = InitSurface (red, green, blue, bEffect ? 1.0f : alpha, bmP, fScale);
#endif
//ogl.SetupTransform (0);
tObjTransformation *posP = OBJPOS (pObj);
transformation.Begin (vPos, posP->mOrient);
RenderFaces (xScale, red, green, blue, alpha, bTextured, nFaces);
#if !DBG
if (gameStates.render.CartoonStyle ())
#endif
	RenderOutline (pObj);
transformation.End ();
//ogl.ResetTransform (0);
ogl.SetTransform (0);
if (bGlow) 
#if 0
	glowRenderer.Done (GLOW_SHIELDS);
#else
	glowRenderer.End ();
#endif
ogl.SetDepthWrite (true);
//ogl.SetDepthMode (GL_LEQUAL);
return 1;
}

// -----------------------------------------------------------------------------

void CSphere::Destroy (void)
{
m_worldVerts.Destroy ();
Init ();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CSphereEdge::Transform (CFloatVector vPos)
{
for (int32_t i = 0; i < 2; i++) {
	transformation.Rotate (m_faces [i].m_vNormal [1], m_faces [i].m_vNormal [0]);
	vPos += m_faces [i].m_vCenter [0];
	transformation.Transform (m_faces [i].m_vCenter [1], vPos);
	}
CMeshEdge::Transform ();
}

//------------------------------------------------------------------------------

int32_t CSphereEdge::Prepare (CFloatVector vPos, int32_t nFilter, float fDistance)
{
Transform (vPos);
int32_t nVisibility = Visibility ();
if ((nVisibility & 1) == (nVisibility >> 1))
	return 0;
gameData.segData.edgeVertexData [0].Add (m_vertices [0][0]);
gameData.segData.edgeVertexData [0].Add (m_vertices [1][0]);
return 1;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void CTesselatedSphere::Transform (float fScale)
{
	CSphereVertex	*v = m_worldVerts.Buffer (),
						*r = m_viewVerts.Buffer ();

for (int32_t i = 0; i < m_nVertices; i++) {
	transformation.Transform (r [i].m_v, v [i].m_v);
	r [i].m_v *= fScale;
	}
}

// -----------------------------------------------------------------------------

int32_t CTesselatedSphere::Quality (void)
{
#if 1 //DBG
return 0;
#else
return m_nQuality ? m_nQuality : gameOpts->render.textures.nQuality + 1;
#endif
}

// -----------------------------------------------------------------------------

void CTesselatedSphere::Destroy (void)
{
m_edges.Destroy ();
m_viewVerts.Destroy ();
CSphere::Destroy ();
}

// -----------------------------------------------------------------------------

int32_t CTesselatedSphere::FindEdge (CFloatVector& v1, CFloatVector& v2)
{
CSphereEdge e;
e.m_vertices [0][0] = v1;
e.m_vertices [1][0] = v2;
for (int32_t i = 0; i < m_nEdges; i++)
	if (m_edges [i] == e)
		return i;
return -1;
}

// -----------------------------------------------------------------------------

int32_t CTesselatedSphere::AddEdge (CFloatVector& v1, CFloatVector& v2, CFloatVector& vCenter)
{
#if DBG
if (m_nEdges >= gameData.segData.nEdges)
	return -1;
CSphereEdge *pEdge = m_edges + m_nEdges++;
pEdge->m_faces [0].m_vNormal [0] = CFloatVector::Normal (v1, v2, vCenter);
pEdge->m_faces [0].m_vCenter [0] = vCenter;
pEdge->m_vertices [0][0] = pEdge->m_faces [0].m_vCenter [0];
pEdge->m_vertices [1][0] = pEdge->m_faces [0].m_vCenter [0] + pEdge->m_faces [0].m_vNormal [0];
pEdge->m_nFaces = 1;
#else
int32_t nFace, i = FindEdge (v1, v2);
if (i < 0) {
#if DBG
	if (m_nEdges >= gameData.segData.nEdges)
		return -1;
#endif
	i = m_nEdges++;
	nFace = 0;
	}
else
	nFace = 1;
#if DBG
if (v1.IsZero ())
	BRP;
if (v2.IsZero ())
	BRP;
#endif
CSphereEdge *pEdge = m_edges + i;
pEdge->m_nFaces = nFace + 1;
if (!nFace) {
	pEdge->m_vertices [0][0] = v1;
	pEdge->m_vertices [1][0] = v2;
	}
pEdge->m_faces [nFace].m_vNormal [0] = CFloatVector::Normal (v1, v2, vCenter);
pEdge->m_faces [nFace].m_vCenter [nFace] = vCenter;
#endif
return 1;
}

// -----------------------------------------------------------------------------

int32_t CTesselatedSphere::CreateEdgeList (void)
{
m_nEdges = m_nFaces * 2;
if (m_nEdges > gameData.segData.nEdges) {
	if (!gameData.segData.edges.Resize (m_nEdges)) 
		return -1;
	gameData.segData.nEdges = m_nEdges;
	}
if (!m_edges.Create (m_nEdges))
	return -1;

m_nEdges = 0;

	int32_t nFaceNodes = FaceNodes ();

for (int32_t i = 0; i < m_nFaces; i++) {
	CSphereFace *pFace = Face (i);
	CSphereVertex *pVertex = pFace->Vertices ();
	pFace->ComputeNormal ();
	pFace->ComputeCenter ();
	for (int32_t j = 0; j < nFaceNodes; j++)
		AddEdge (pVertex [j].m_v, pVertex [(j + 1) % nFaceNodes].m_v, pFace->Center ());
	}
return m_nEdges;
}

// -----------------------------------------------------------------------------

void CTesselatedSphere::RenderOutline (CObject *pObj)
{
if (m_edges.Buffer ()) {
	CFloatVector vPos;
	vPos.Assign (pObj->Position ());

	for (int32_t i = 0; i < m_nEdges; i++)
		m_edges [i].Prepare (vPos);
	RenderMeshOutline (CMeshEdge::DistToScale (X2F (Max (0, CFixVector::Dist (pObj->Position (), gameData.objData.pViewer->Position ()) - pObj->Size ()))));
	}
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

#define UV_SCALE	3.0f

int32_t CRingedSphere::CreateVertices (int32_t nRings, int32_t nFaces)
{
	int32_t			h, i, j;
	float				t1, t2, t3, a, sint1, cost1, sint2, cost2, sint3, cost3;
	CSphereVertex	*pVertex;

if (nRings > MAX_SPHERE_RINGS)
	nRings = MAX_SPHERE_RINGS;
if ((m_nRings == nRings) && (m_nFaces == nFaces) && (m_worldVerts.Buffer () != NULL))
	return 1;

m_nRings =
m_nFaces = 
m_nVertices = 0;
m_worldVerts.Destroy ();
h = nRings * (nRings + 1);
if (!m_worldVerts.Create (h))
	return 0;
m_nRings = nRings;
m_nFaces = nFaces;
m_nVertices = h;
h = nRings / 2;
a = float (2 * PI / nRings);
pVertex = m_worldVerts.Buffer ();
for (j = 0; j < h; j++) {
	t1 = float (j * a - PI / 2);
	t2 = t1 + a;
	sint1 = float (sin (t1));
	cost1 = float (cos (t1));
	sint2 = float (sin (t2));
	cost2 = float (cos (t2));
	for (i = 0; i <= nRings; i++) {
		t3 = i * a;
		sint3 = float (sin (t3));
		cost3 = float (cos (t3));
		pVertex->m_v.v.coord.x = cost2 * cost3;
		pVertex->m_v.v.coord.y = sint2;
		pVertex->m_v.v.coord.z = cost2 * sint3;
		pVertex->m_tc.v.u =(1.0f - float (i) / nRings) * nFaces * UV_SCALE;
		pVertex->m_tc.v.v = (float (2 * j + 2) / nRings) * nFaces * UV_SCALE;
		pVertex++;
		pVertex->m_v.v.coord.x = cost1 * cost3;
		pVertex->m_v.v.coord.y = sint1;
		pVertex->m_v.v.coord.z = cost1 * sint3;
		pVertex->m_tc.v.u = (1.0f - float (i) / nRings) * nFaces * UV_SCALE;
		pVertex->m_tc.v.v = (float (2 * j) / nRings) * nFaces * UV_SCALE;
		pVertex++;
		}
	}
return 1;
}

// -----------------------------------------------------------------------------

int32_t CRingedSphere::Create (void)
{
return CreateVertices ();
}

// -----------------------------------------------------------------------------

void CRingedSphere::RenderFaces (float fRadius, float red, float green, float blue, float alpha, int32_t bTextured, int32_t nFaces)
{
	int32_t			nCull, h, i, j, nQuads, nRings = Quality ();
	CFloatVector	p [2 * MAX_SPHERE_RINGS + 2];
	tTexCoord2f		tc [2 * MAX_SPHERE_RINGS + 2];
	CSphereVertex*	svP [2];

if (nRings > MAX_SPHERE_RINGS)
	nRings = MAX_SPHERE_RINGS;
if (!CreateVertices (nRings, nFaces))
	return;
h = nRings / 2;
nQuads = 2 * nRings + 2;

ogl.EnableClientStates (bTextured, 0, 0, GL_TEXTURE0);
if (ogl.UseTransform ()) {
	glScalef (fRadius, fRadius, fRadius);
	for (nCull = 0; nCull < 2; nCull++) {
		svP [0] = svP [1] = m_worldVerts.Buffer ();
		ogl.SetCullMode (nCull ? GL_FRONT : GL_BACK);
		for (i = 0; i < h; i++) {
			DrawFaces (i, nQuads, bTextured, GL_QUAD_STRIP);
#if 0
			if (!bTextured) {
				for (i = 0; i < nQuads; i++, svP [1]++) {
					p [i] = svP [1]->m_v;
					if (bTextured) {
						tc [i].dir.u = svP [1]->m_tc.dir.u * nFaces;
						tc [i].dir.dir = svP [1]->m_tc.dir.dir * nFaces;
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
		ogl.SetCullMode (nCull ? GL_FRONT : GL_BACK);
		svP [0] = svP [1] = m_worldVerts.Buffer ();
		for (j = 0; j < h; j++) {
			for (i = 0; i < nQuads; i++, svP [0]++) {
				p [i] = svP [0]->m_v * fRadius;
				transformation.Transform (p [i], p [i], 0);
				if (bTextured)
					tc [i] = svP [0]->m_tc;
				}
			DrawFaces (p, tc, nQuads, bTextured, GL_QUAD_STRIP);
#if 0
			if (!bTextured) {
				for (i = 0; i < nQuads; i++, svP [1]++) {
					p [i] = svP [1]->m_v;
					VmVecScale (p + i, p + i, fRadius);
					transformation.Transform (p + i, p + i, 0);
					if (bTextured) {
						tc [i].dir.u = svP [1]->m_tc.dir.u * nFaces;
						tc [i].dir.dir = svP [1]->m_tc.dir.dir * nFaces;
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
ogl.DisableClientStates (bTextured, 0, 0, GL_TEXTURE0);
OglCullFace (0);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

int32_t CTriangleSphere::Tesselate (CSphereTriangle *pSrc, CSphereTriangle *pDest, int32_t nFaces)
{
for (int32_t i = 0; i < nFaces; i++)
	pDest = (pSrc++)->Split (pDest);
return 1;
}

// -----------------------------------------------------------------------------

void CTriangleSphere::SetupFaces (void)
{
static float baseOctagon [8][3][3] = {
	{{-1,0,1},{1,0,1},{0,1,0}},
	{{1,0,1},{1,0,-1},{0,1,0}},
	{{1,0,-1},{-1,0,-1},{0,1,0}},
	{{-1,0,-1},{-1,0,1},{0,1,0}},
	{{1,0,1},{-1,0,1},{0,-1,0}},
	{{1,0,-1},{1,0,1},{0,-1,0}},
	{{-1,0,-1},{1,0,-1},{0,-1,0}},
	{{-1,0,1},{-1,0,-1},{0,-1,0}}
	};

static tTexCoord2f baseTC [2][3] = {{{0,0}, {1,0}, {0.5f,0.5f}}, {{0.5f,0.5f}, {1,1}, {0,1}}};

for (int32_t i = 0; i < 8; i++) {
	for (int32_t j = 0; j < 3; j++) {
		CSphereVertex v;
		for (int32_t k = 0; k < 3; k++) 
			v.m_v.v.vec [k] = baseOctagon [i][j][k];
		v.m_tc = baseTC [i & 1][j];
		m_faces [0][i].m_v [j] = v;
		}
	}
}

// -----------------------------------------------------------------------------

int32_t CTriangleSphere::CreateFaces (void)
{
SetupFaces ();
int32_t i, j, nFaces = 8, q = Quality ();
for (i = 0, j = 1; i < q; i++, nFaces *= 4, j = !j) {
	Tesselate (m_faces [j].Buffer (), m_faces [!j].Buffer (), nFaces);
	}
m_nFaces = nFaces;
return !j;
}

// -----------------------------------------------------------------------------

int32_t CTriangleSphere::CreateBuffers (void)
{
m_nFaces = 8 * int32_t (pow (4.0f, float (Quality ())));
m_nVertices = m_nFaces * 3;

if (m_faces [0].Create (m_nFaces) && m_faces [1].Create (m_nFaces) && m_worldVerts.Create (m_nVertices) && m_viewVerts.Create (m_nVertices)) 
	return 1;
m_viewVerts.Destroy ();
m_worldVerts.Destroy ();
m_faces [1].Destroy ();
m_faces [0].Destroy ();
PrintLog (-1);
return 0;
}

// -----------------------------------------------------------------------------

int32_t CTriangleSphere::Create (void)
{
if (!CreateBuffers ())
	return 0;

m_nFaceBuffer = CreateFaces ();
CreateEdgeList ();

CSphereTriangle *pFace = m_faces [m_nFaceBuffer].Buffer ();
CSphereVertex *pVertex = m_worldVerts.Buffer ();

for (int32_t i = 0; i < m_nFaces; i++, pFace++) {
	for (int32_t j = 0; j < 3; j++)
		*(pVertex++) = pFace->m_v [j];
	}
m_faces [0].Destroy ();
m_faces [1].Destroy ();
return m_nFaces;
}

// -----------------------------------------------------------------------------

void CTriangleSphere::RenderFaces (float fRadius, float red, float green, float blue, float alpha, int32_t bTextured, int32_t nFaces)
{
if (!m_worldVerts.Buffer () || !m_viewVerts.Buffer ())
	return;
Transform (fRadius);
glScalef (fRadius, fRadius, fRadius);
#if 1
for (int32_t nCull = 0; nCull < 2; nCull++) {
	ogl.SetCullMode (nCull ? GL_FRONT : GL_BACK);
	DrawFaces (0, m_nFaces, bTextured, GL_TRIANGLES);
	}
#else
glBegin (GL_LINES);
CSphereVertex *ps = m_worldVerts.Buffer ();
for (int32_t j = m_nFaces; j; j--, ps++)
	for (int32_t i = 0; i < 3; i++, ps++) {
		if (bTextured)
			glTexCoord2f (ps->m_tc.v.u, ps->m_tc.v.v);
		glVertex3fv (reinterpret_cast<GLfloat*> (&ps->m_v));
		}
glEnd ();
glColor4f (red, green, blue, bTextured ? 1.0f : alpha);
glBegin (GL_TRIANGLES);
ps = m_worldVerts.Buffer ();
for (int32_t j = nFaces; j; j--, ps++)
	for (int32_t i = 0; i < 3; i++, ps++) {
		glVertex3fv (reinterpret_cast<GLfloat*> (ps));
		}
glEnd ();
#endif
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

#if DBG
int32_t nDestBuffer = 1;
#endif

int32_t CQuadSphere::Tesselate (CSphereQuad *pDest, CSphereQuad *pSrc, int32_t nFaces)
{
for (int32_t i = 0; i < nFaces; i++) {
	pDest = (pSrc++)->Split (pDest);
#if DBG
	if (pDest - m_faces [nDestBuffer].Buffer () > m_nFaces)
		BRP;
#endif
	}
return 1;
}

// -----------------------------------------------------------------------------

void CQuadSphere::SetupFaces (void)
{
static float baseCube [6][4][3] = {
	{{-1,-1,-1},{+1,-1,-1},{+1,+1,-1},{-1,+1,-1}},
	{{+1,-1,-1},{+1,-1,+1},{+1,+1,+1},{+1,+1,-1}},
	{{+1,-1,+1},{-1,-1,+1},{-1,+1,+1},{+1,+1,+1}},
	{{-1,-1,+1},{-1,-1,-1},{-1,+1,-1},{-1,+1,+1}},
	{{-1,+1,-1},{+1,+1,-1},{+1,+1,+1},{-1,+1,+1}},
	{{+1,-1,-1},{-1,-1,-1},{-1,-1,+1},{+1,-1,+1}}
	};

static tTexCoord2f baseTC [4] = {{0,0}, {1,0}, {1,1}, {0,1}};

for (int32_t i = 0; i < 6; i++) {
	for (int32_t j = 0; j < 4; j++) {
		CSphereVertex v;
		for (int32_t k = 0; k < 3; k++)
			v.m_v.v.vec [k] = baseCube [i][j][k];
		v.m_tc = baseTC [j];
		m_faces [0][i].m_v [j] = v;
		}
	}
}

// -----------------------------------------------------------------------------

int32_t CQuadSphere::CreateFaces (void)
{
SetupFaces ();
int32_t i, j, nFaces = 6, q = Quality ();
for (i = 0, j = 1; i < q; i++, nFaces *= 4, j = !j) {
#if DBG
	nDestBuffer = j;
#endif
	Tesselate (m_faces [j].Buffer (), m_faces [!j].Buffer (), nFaces);
	}
m_nFaces = nFaces;
return !j;
}

// -----------------------------------------------------------------------------

int32_t CQuadSphere::CreateBuffers (void)
{
m_nFaces = 6 * int32_t (pow (4.0f, float (Quality ())));
m_nVertices = m_nFaces * 4;

if (m_faces [0].Create (m_nFaces) && m_faces [1].Create (m_nFaces) && m_worldVerts.Create (m_nVertices) && m_viewVerts.Create (m_nVertices)) 
	return 1;
m_viewVerts.Destroy ();
m_worldVerts.Destroy ();
m_faces [1].Destroy ();
m_faces [0].Destroy ();
PrintLog (-1);
return 0;
}

// -----------------------------------------------------------------------------

int32_t CQuadSphere::Create (void)
{
if (!CreateBuffers ())
	return 0;

m_nFaceBuffer = CreateFaces ();
CreateEdgeList ();

CSphereQuad *pFace = m_faces [m_nFaceBuffer].Buffer ();
CSphereVertex *pVertex = m_worldVerts.Buffer ();

for (int32_t i = 0; i < m_nFaces; i++, pFace++) {
	for (int32_t j = 0; j < 4; j++)
		*(pVertex++) = pFace->m_v [j];
	}

m_faces [0].Destroy ();
m_faces [1].Destroy ();
return m_nFaces;
}

// -----------------------------------------------------------------------------

void CQuadSphere::RenderFaces (float fRadius, float red, float green, float blue, float alpha, int32_t bTextured, int32_t nFaces)
{
if (!m_worldVerts.Buffer () || !m_viewVerts.Buffer ())
	return;
//Transform (fRadius);
#if 1
glScalef (fRadius, fRadius, fRadius);
#	if 0
glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
glLineWidth (3);
for (int32_t nCull = 0; nCull < 2; nCull++) {
	ogl.SetCullMode (nCull ? GL_FRONT : GL_BACK);
	DrawFaces (0, m_nFaces, bTextured, GL_QUADS);
	}
glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
#	else
for (int32_t nCull = 0; nCull < 2; nCull++) {
	ogl.SetCullMode (nCull ? GL_FRONT : GL_BACK);
	DrawFaces (0, m_nFaces, bTextured, GL_QUADS);
	}
#	endif
#else
glBegin (GL_LINES);
CSphereVertex *ps = m_worldVerts.Buffer ();
for (int32_t j = m_nFaces; j; j--, ps++)
	for (int32_t i = 0; i < 3; i++, ps++) {
		if (bTextured)
			glTexCoord2f (ps->m_tc.v.u, ps->m_tc.v.v);
		glVertex3fv (reinterpret_cast<GLfloat*> (&ps->m_v));
		}
glEnd ();
glColor4f (red, green, blue, bTextured ? 1.0f : alpha);
glBegin (GL_QUADS);
ps = m_worldVerts.Buffer ();
for (int32_t j = nFaces; j; j--, ps++)
	for (int32_t i = 0; i < 3; i++, ps++) {
		glVertex3fv (reinterpret_cast<GLfloat*> (ps));
		}
glEnd ();
#endif
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void SetupSpherePulse (CPulseData *pPulse, float fSpeed, float fMin)
{
pPulse->fScale =
pPulse->fMin = fMin;
pPulse->fSpeed =
pPulse->fDir = fSpeed;
}

// -----------------------------------------------------------------------------

CSphere *CreateSphere (void)
{
#if SPHERE_TYPE == SPHERE_TYPE_RINGS
return new CRingedSphere ();
#elif SPHERE_TYPE == SPHERE_TYPE_TRIANGLES
return new CTriangleSphere ();
#elif SPHERE_TYPE == SPHERE_TYPE_QUADS
return new CQuadSphere ();
#endif
}

// -----------------------------------------------------------------------------

int32_t CreateShieldSphere (void)
{
if (!shield.Load ())
	return 0;
if (gameData.render.shield) {
	if (gameData.render.shield->HasQuality (gameOpts->render.textures.nQuality + 1))
		return 1;
	delete gameData.render.shield;
	}
if (!(gameData.render.shield = CreateSphere ()))
	return 0;
gameData.render.shield->Create ();
gameData.render.shield->SetupPulse (0.02f, 0.5f);
gameData.render.shield->SetPulse (gameData.render.shield->Pulse ());
return 1;
}

// -----------------------------------------------------------------------------

int32_t DrawShieldSphere (CObject *pObj, float red, float green, float blue, float alpha, char bAdditive, fix nSize)
{
if (!CreateShieldSphere ())
	return 0;
#if !RINGED_SPHERE
if (gameData.render.shield->m_nFaces > 0)
#endif
 {
	if (!nSize) {
#if 0 //DBG
		nSize = pObj->info.xSize;
#else
		if (pObj->rType.polyObjInfo.nModel < 0) 
			nSize = pObj->info.xSize;
		else {
			CPolyModel* pModel = GetPolyModel (pObj, NULL, pObj->ModelId (), 0);
			nSize = pModel ? pModel->Rad () : pObj->info.xSize;
			}
#endif
		}
	float r = X2F (nSize);
	if (gameStates.render.nType == RENDER_TYPE_TRANSPARENCY)
		gameData.render.shield->Render (pObj, NULL, r, r, r, red, green, blue, alpha, shield.Bitmap (), 1, bAdditive);
	else
		transparencyRenderer.AddSphere (riSphereShield, red, green, blue, alpha, pObj, bAdditive, nSize);
	}
return 1;
}

// -----------------------------------------------------------------------------

void DrawMonsterball (CObject *pObj, float red, float green, float blue, float alpha)
{
#if !RINGED_SPHERE
if (!gameData.render.monsterball) {
	gameData.render.monsterball = CreateSphere ();
	if (!gameData.render.monsterball)
		return;
	gameData.render.monsterball->SetQuality (3);
	gameData.render.monsterball->Create ();
	}
if (gameData.render.monsterball->m_nFaces > 0)
#endif
	{
	if (gameStates.render.nType != RENDER_TYPE_TRANSPARENCY)
		transparencyRenderer.AddSphere (riMonsterball, red, green, blue, alpha, pObj, 0);
	else {
		float r = X2F (pObj->info.xSize);
		CFloatVector p;
		p.SetZero ();
		gameData.render.monsterball->Render (pObj, &p, r, r, r, red, green, blue, gameData.hoard.monsterball.bm.Buffer () ? 1.0f : alpha,
														 &gameData.hoard.monsterball.bm, 4, 0);
		ogl.ResetTransform (1);
		ogl.SetTransform (0);
		}
	}
}

// -----------------------------------------------------------------------------

void DestroyShieldSphere (void)
{
if (gameData.render.shield)
	gameData.render.shield->Destroy ();
}

// -----------------------------------------------------------------------------

void DestroyMonsterball (void)
{
if (gameData.render.monsterball)
	gameData.render.monsterball->Destroy ();
}

// -----------------------------------------------------------------------------

void InitSpheres (void)
{
PrintLog (1, "creating spheres\n");
CreateShieldSphere ();
PrintLog (-1);
}

// -----------------------------------------------------------------------------
