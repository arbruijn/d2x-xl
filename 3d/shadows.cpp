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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <math.h>
#include <stdlib.h>
#include "error.h"

#include "descent.h"
#include "interp.h"
#include "shadows.h"
#include "globvars.h"
#include "gr.h"
#include "byteswap.h"
#include "u_mem.h"
#include "console.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "network.h"
#include "rendermine.h"
#include "gameseg.h"
#include "light.h"
#include "lightning.h"

extern tFaceColor tMapColor;
extern int bPrintLine;

using namespace POF;

//------------------------------------------------------------------------------

//shadow clipping
//1: Compute hit point of vector from current light through each model vertex (fast)
//2: Compute hit point of vector from current light through each lit submodel vertex (slow)
//3: Compute hit point of vector from current light through each lit model face (fastest, flawed)

#define SHADOW_TEST				0
#define NORM_INF					1

float fInfinity [4] = {100.0f, 100.0f, 200.0f, 400.0f};	//5, 10, 20 standard cubes
float fInf;

extern tRgbaColorf shadowColor [2], modelColor [2];

static CFloatVector	vLightPosf,
							//vViewerPos,
							vCenterf;
static CFixVector vLightPos;

//#define N_OPCODES (sizeof (opcode_table) / sizeof (*opcode_table))
//this is a table of mappings from RGB15 to palette colors

#if DBG_SHADOWS
extern int bShadowTest;
extern int bFrontCap;
extern int bRearCap;
extern int bShadowVolume;
extern int bFrontFaces;
extern int bBackFaces;
extern int bSWCulling;
#endif
#if SHADOWS
extern int bZPass;
#endif
static int bTriangularize = 0;
//static int bIntrinsicFacing = 0;
static int bFlatPolys = 1;
static int bTexPolys = 1;

//------------------------------------------------------------------------------

inline int G3CheckPointFacing (CFloatVector *pv, CFloatVector *pNorm, CFloatVector *pDir)
{
	CFloatVector	h = *pDir - *pv;

return (h * *pNorm) > 0;
}

//------------------------------------------------------------------------------

inline int CFace::IsFacingLight (void)
{
return G3CheckPointFacing (&m_vCenterf, &m_vNormf, &vLightPosf);
}

//------------------------------------------------------------------------------

inline int CFace::IsFacingViewer (void)
{
return CFloatVector::Dot (m_vCenterf, m_vNormf) >= 0;
}

//------------------------------------------------------------------------------

inline CFloatVector* CFace::RotateNormal (void)
{
m_vNormf.Assign (transformation.Rotate (m_vRotNorm, m_vNorm));
return &m_vNormf;
}

//------------------------------------------------------------------------------

inline CFloatVector *CFace::TransformCenter (void)
{
	CFixVector	v;

m_vCenterf.Assign (transformation.Transform (v, m_vCenter, 0));
return &m_vCenterf;
}

//------------------------------------------------------------------------------

inline int CFace::IsLit (void)
{
return m_bFacingLight = IsFacingLight ();
}

//------------------------------------------------------------------------------

inline int CFace::IsFront (void)
{
return m_bFrontFace = IsFacingViewer ();
}

//------------------------------------------------------------------------------

int G3GetFaceWinding (CFloatVector *v0, CFloatVector *v1, CFloatVector *v2)
{
return (((*v1) [X] - (*v0) [X]) * ((*v2) [Y] - (*v1) [Y]) < 0) ? GL_CW : GL_CCW;
}

//------------------------------------------------------------------------------

int CModel::CountItems (void *modelDataP)
{
	ubyte *p = reinterpret_cast<ubyte*> (modelDataP);

G3CheckAndSwap (modelDataP);
for (;;)
	switch (WORDVAL (p)) {
		case OP_EOF:
			return 1;

		case OP_DEFPOINTS: {
			int n = WORDVAL (p+2);
			m_nVerts += n;
			p += n * sizeof (CFixVector) + 4;
			break;
			}

		case OP_DEFP_START: {
			int n = WORDVAL (p+2);
			p += n * sizeof (CFixVector) + 8;
			m_nVerts += n;
			break;
			}

		case OP_FLATPOLY: {
			int nVerts = WORDVAL (p+2);
			m_nAdjFaces += nVerts;
			m_nFaces++;
			m_nFaceVerts += nVerts;
			p += 30 + ((nVerts & ~ 1) + 1) * 2;
			break;
			}

		case OP_TMAPPOLY: {
			int nVerts = WORDVAL (p + 2);
			m_nAdjFaces += nVerts;
			m_nFaces += nVerts - 2;
			m_nFaceVerts += (nVerts - 2) * 3;
			p += 30 + ((nVerts&~1)+1)*2 + nVerts*12;
			break;
			}

		case OP_SORTNORM:
			CountItems (p + WORDVAL (p+28));
			CountItems (p + WORDVAL (p+30));
			p += 32;
			break;


		case OP_RODBM: {
			p+=36;
			break;
			}

		case OP_SUBCALL: {
			m_nSubModels++;
			CountItems (p + WORDVAL (p+16));
			p += 20;
			break;
			}

		case OP_GLOW:
			p += 4;
			break;

		default:
			Error ("invalid polygon model\n");
	}
return 1;
}

//------------------------------------------------------------------------------

void CFace::CalcNormal (CModel* po)
{
if (bTriangularize) {
	CFixVector	*pv = po->m_verts.Buffer ();
	short			*pfv = m_verts;

	m_vNorm = CFixVector::Normal (pv [pfv [0]], pv [pfv [1]], pv [pfv [2]]);
	}
else
	m_vNormf.Assign (m_vNorm);
}

//------------------------------------------------------------------------------

CFloatVector CFace::CalcCenterf (CModel* po)
{
	CFloatVector	*pv = po->m_vertsf.Buffer ();
	short				*pfv = m_verts;
	int				i;

	CFloatVector	c;

c.SetZero ();
for (i = m_nVerts; i; i--, pfv++)
	c += pv [*pfv];
c /= static_cast<float> (m_nVerts);
return c;
}

//------------------------------------------------------------------------------

CFixVector CFace::CalcCenter (CModel* po)
{
	CFloatVector	cf;
	CFixVector		c;

cf = CalcCenterf (po);
c.Assign (cf);
return c;
}

//------------------------------------------------------------------------------

inline void CModel::AddTriangle (CFace* pf, short v0, short v1, short v2)
{
	short	*pfv = m_faceVerts + m_iFaceVert;

pf->m_verts = pfv;
pf->m_nVerts = 3;
pfv [0] = v0;
pfv [1] = v1;
pfv [2] = v2;
pf->CalcNormal (this);
}

//------------------------------------------------------------------------------

#if 0

short G3FindPolyModelFace (CModel* po, CFace* pf)
{
	int			h, i, j, k, l;
	CFace	*pfh;

for (h = pf->m_nVerts, i = po->m_iFace, pfh = po->m_faces; i; i--, pfh++)
	if (pfh->nVerts == h) {
		for (k = h, j = 0; j < h; j++)
			for (l = 0; l < h; l++)
				if (pf->m_verts [j] == pfh->verts [l]) {
					k--;
					break;
					}
		if (!k)
			return i;
		}
return -1;
}

#else
//------------------------------------------------------------------------------

int CModel::FindFace (short *p, int nVerts)
{
	CFace*	pf;
	short*	pfv;
	int		h, i, j, k;

for (i = m_iFace, pf = m_faces.Buffer (); i; i--, pf++) {
	if (pf->m_nVerts != nVerts)
		continue;
	pfv = pf->m_verts;
	for (j = 0, h = *p; j < nVerts; j++) {
		if (h == pfv [j]) {
			h = j;
			for (k = 0; k < nVerts; k++, j = (j + 1) % nVerts) {
				if (p [k] != pfv [j]) {
					j = h;
					for (k = 0; k < nVerts; k++) {
						if (p [k] != pfv [j])
							break;
						if (!j)
							j = nVerts;
						j--;
						}
					break;
					}
				}
			if (k == nVerts)
				return 1;
			break;
			}
		}
	}
return 0;
}
#endif
//------------------------------------------------------------------------------

CFace* CModel::AddFace (CSubModel* pso, CFace* pf, CFixVector *pn, ubyte *p, int bShadowData)
{
	short			nVerts = WORDVAL (p+2);
	CFixVector	*pv = m_verts.Buffer (), v;
	short			*pfv;
	short			i, v0;

//if (G3FindPolyModelFace (po, WORDPTR (p+30), nVerts))
//	return pf;
if (bShadowData) {
	pf->m_vNorm = *pn;
	if (bTriangularize) {
		pfv = WORDPTR (p+30);
		v0 = *pfv;
		if (nVerts == 3) {
			AddTriangle (pf, pfv [0], pfv [1], pfv [2]);
			m_iFaceVert += 3;
			pf->m_bGlow = (nGlow >= 0);
			pf++;
			m_iFace++;
			pso->m_nFaces++;
			}
		else if (nVerts == 4) {
			v = pv [pfv [3]] - pv [pfv [1]];
			if (CFixVector::Dot (*pn, v) < 0) {
				AddTriangle (pf, pfv [0], pfv [1], pfv [3]);
				m_iFaceVert += 3;
				pf->m_bGlow = (nGlow >= 0);
				pf++;
				m_iFace++;
				pso->m_nFaces++;
				AddTriangle (pf, pfv [1], pfv [2], pfv [3]);
				m_iFaceVert += 3;
				pf->m_bGlow = (nGlow >= 0);
				pf++;
				m_iFace++;
				pso->m_nFaces++;
				}
			else {
				AddTriangle (pf, pfv [0], pfv [1], pfv [2]);
				m_iFaceVert += 3;
				pf->m_bGlow = (nGlow >= 0);
				pf++;
				m_iFace++;
				pso->m_nFaces++;
				AddTriangle (pf, pfv [0], pfv [2], pfv [3]);
				m_iFaceVert += 3;
				pf->m_bGlow = (nGlow >= 0);
				pf++;
				m_iFace++;
				pso->m_nFaces++;
				}
			}
		else {
			for (i = 1; i < nVerts - 1; i++) {
				AddTriangle (pf, v0, pfv [i], pfv [i + 1]);
				m_iFaceVert += 3;
				pf->m_bGlow = (nGlow >= 0);
				pf++;
				m_iFace++;
				pso->m_nFaces++;
				}
			}
		}
	else { //if (nVerts < 5) {
		pfv = pf->m_verts = m_faceVerts + m_iFaceVert;
		pf->m_nVerts = nVerts;
		memcpy (pfv, WORDPTR (p+30), nVerts * sizeof (short));
		pf->m_bGlow = (nGlow >= 0);
		pf->CalcNormal (this);
#if 0
		if (memcmp (&pf->m_vCenter, m_verts + *pv, sizeof (CFixVector))) //make sure we have a vertex from the face
			pf->m_vCenter = m_verts [*pv];
		VmVecNormal (&n, m_verts + pv [0], m_verts + pv [1], m_verts + pv [2]); //check the precomputed Normal
		if (memcmp (&pf->m_vNorm, &n, sizeof (CFixVector)))
			pf->m_vNorm = n;
#endif
#if 0
		if (G3FindPolyModelFace (po, pf) < 0) //check for duplicate faces
#endif
		 {
			m_iFaceVert += nVerts;
			pf++;
			m_iFace++;
			pso->m_nFaces++;
			}
		}
#if 0
	else {
		short h = (nVerts + 1) / 2;
		pfv = pf->m_verts = m_faceVerts + m_iFaceVert;
		pf->m_nVerts = h;
		memcpy (pfv, WORDPTR (p+30), h * sizeof (short));
		pf->m_bGlow = (nGlow >= 0);
		G3CalcFaceNormal (po, pf);
		m_iFaceVert += h;
		pf->m_bTest = 1;
		pf++;
		m_iFace++;
		pso->m_nFaces++;
		pfv = pf->m_verts = m_faceVerts + m_iFaceVert;
		pf->m_nVerts = h;
		memcpy (pfv, WORDPTR (p + 30) + nVerts / 2, h * sizeof (short));
		pf->m_bGlow = (nGlow >= 0);
		G3CalcFaceNormal (po, pf);
		pf->m_bTest = 1;
		m_iFaceVert += h;
		pf++;
		m_iFace++;
		pso->m_nFaces++;
		}
#endif
	}
else {
	CFloatVector	nf;
	g3sNormal		*pvn;

	pfv = WORDPTR (p+30);
	nf.Assign (*pn);
	for (i = 0; i < nVerts; i++) {
		pvn = m_vertNorms + pfv [i];
		pvn->vNormal += nf;
		pvn->nFaces++;
		}
	}
return pf;
}

//------------------------------------------------------------------------------

void CSubModel::RotateNormals (void)
{
	CFace*	pf;
	int		i;

for (i = m_nFaces, pf = m_faces; i; i--, pf++) {
	pf->RotateNormal ();
	pf->TransformCenter ();
	}
}

//------------------------------------------------------------------------------

CFloatVector* CModel::VertsToFloat (void)
{
for (int i = 0; i < m_nVerts; i++)
	m_vertsf [i].Assign (m_verts [i]);
return m_vertsf.Buffer ();
}

//------------------------------------------------------------------------------

#define MAXGAP	0.01f;

int CSubModel::FindEdge (CFace* pf0, int v0, int v1)
{
	int		h, i, j, n;
	CFace*	pf1;
	short*	pfv;

for (i = m_nFaces, pf1 = m_faces; i; i--, pf1++) {
	if (pf1 != pf0) {
		for (j = 0, n = pf1->m_nVerts, pfv = pf1->m_verts; j < n; j++) {
			h = (j + 1) % n;
			if (((pfv [j] == v0) && (pfv [h] == v1)) || ((pfv [j] == v1) && (pfv [h] == v0)))
				return (int) (pf1 - m_faces);
			}
		}
	}
return -1;
}

//------------------------------------------------------------------------------

int CModel::GatherAdjFaces (void)
{
	short			h, i, j, n;
	CSubModel*	pso = m_subModels.Buffer ();
	CFace*		pf;
	short*		pfv;

m_nAdjFaces = 0;
VertsToFloat ();
for (i = m_nSubModels; i; i--, pso++) {
	pso->m_adjFaces = m_adjFaces + m_nAdjFaces;
	for (j = pso->m_nFaces, pf = pso->m_faces; j; j--, pf++) {
		pf->m_nAdjFaces = m_nAdjFaces;
		for (h = 0, n = pf->m_nVerts, pfv = pf->m_verts; h < n; h++)
			m_adjFaces [m_nAdjFaces++] = pso->FindEdge (pf, pfv [h], pfv [(h + 1) % n]);
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int CSubModel::CalcFacing (CModel* po)
{
	short		i;

RotateNormals ();
for (i = 0; i < m_nFaces; i++) {
	m_faces [i].IsLit ();
#if 0
	pf->IsFront ();
#endif
	}
return m_nFaces;
}

//------------------------------------------------------------------------------

int CSubModel::GatherLitFaces (CModel* po)
{
	short		i;
	CFace		*pf;

CalcFacing (po);
m_nLitFaces = 0;
m_litFaces = po->m_litFaces + po->m_litFaces.ToS ();
for (i = m_nFaces, pf = m_faces; i; i--, pf++) {
	if (pf->m_bFacingLight) {
		po->m_litFaces.Push (pf);
		m_nLitFaces++;
		}
	}
return 1;
}

//------------------------------------------------------------------------------

void CModel::CalcCenters (void)
{
	short			i, j;
	CSubModel*	pso = m_subModels.Buffer ();
	CFace*		pf;

for (i = m_nSubModels; i; i--, pso++)
	for (j = pso->m_nFaces, pf = pso->m_faces; j; j--, pf++)
		pf->m_vCenter = pf->CalcCenter (this);
}

//------------------------------------------------------------------------------

int CModel::GatherItems (void *modelDataP, CAngleVector *animAngleP, int bInitModel, int bShadowData, int nThis, int nParent)
{
	ubyte*		p = reinterpret_cast<ubyte*> (modelDataP);
	CSubModel*	pso = m_subModels + nThis;
	CFace*		pf = m_faces + m_iFace;
	int			nChild;

G3CheckAndSwap (modelDataP);
nGlow = -1;
if (bInitModel && !pso->m_faces) {
	pso->m_faces = pf;
	pso->m_nParent = nParent;
	}
for (;;)
	switch (WORDVAL (p)) {
		case OP_EOF:
			return 1;
		case OP_DEFPOINTS: {
			int n = WORDVAL (p+2);
			if (bInitModel)
				memcpy (m_verts.Buffer (), VECPTR (p+4), n * sizeof (CFixVector));
			else
				RotatePointListToVec (m_verts.Buffer (), VECPTR (p+4), n);
			//m_nVerts += n;
			p += n * sizeof (CFixVector) + 4;
			break;
			}

		case OP_DEFP_START: {
			int n = WORDVAL (p+2);
			int s = WORDVAL (p+4);
			if (bInitModel)
				memcpy (m_verts + s, VECPTR (p+8), n * sizeof (CFixVector));
			else
				RotatePointListToVec (m_verts + s, VECPTR (p+8), n);
			p += n * sizeof (CFixVector) + 8;
			break;
			}

		case OP_FLATPOLY: {
			int nVerts = WORDVAL (p+2);
			if (bInitModel && bFlatPolys) {
				pf = AddFace (pso, pf, VECPTR (p+16), p, bShadowData);
				}
			p += 30 + (nVerts | 1) * 2;
			break;
			}

		case OP_TMAPPOLY: {
			int nVerts = WORDVAL (p + 2);
			if (bInitModel && bTexPolys) {
				pf = AddFace (pso, pf, VECPTR (p+16), p, bShadowData);
				}
			p += 30 + (nVerts | 1) * 2 + nVerts * 12;
			break;
			}

		case OP_SORTNORM:
			GatherItems (p + WORDVAL (p+28), animAngleP, bInitModel, bShadowData, nThis, nParent);
			if (bInitModel)
				pf = m_faces + m_iFace;
			GatherItems (p + WORDVAL (p+30), animAngleP, bInitModel, bShadowData, nThis, nParent);
			if (bInitModel)
				pf = m_faces + m_iFace;
			p += 32;
			break;


		case OP_RODBM:
			p+=36;
			break;

		case OP_SUBCALL: {
			CAngleVector *a;

			if (animAngleP)
				a = &animAngleP [WORDVAL (p+2)];
			else
				a = const_cast<CAngleVector*> (&CAngleVector::ZERO);
			nChild = ++m_iSubObj;
			m_subModels [nChild].m_vPos = *VECPTR (p+4);
			m_subModels [nChild].m_vAngles = *a;
			transformation.Begin (*VECPTR (p+4), *a);
			GatherItems (p + WORDVAL (p+16), animAngleP, bInitModel, bShadowData, nChild, nThis);
			transformation.End ();
			if (bInitModel)
				pf = m_faces + m_iFace;
			p += 20;
			break;
			}

		case OP_GLOW:
			nGlow = WORDVAL (p+2);
			p += 4;
			break;

		default:
			Error ("invalid polygon model\n");
	}
return 1;
}

//------------------------------------------------------------------------------

int CModel::Create (void *modelDataP, int bShadowData)
{
	int	h;

Init ();
m_nSubModels = 1;
CountItems (modelDataP);
h = m_nSubModels * sizeof (CSubModel);
if (!(m_subModels.Create (h))) {
	Destroy ();
	return 0;
	}
m_subModels.Clear ();
h = m_nVerts;
if (!(m_verts.Create (h))) {
	Destroy ();
	return 0;
	}
if (!(m_fClipDist.Create (h)))
	gameOpts->render.shadows.nClip = 1;
if (!(m_vertFlags.Create (h)))
	gameOpts->render.shadows.nClip = 1;
if (bShadowData) {
	if (!(m_faces.Create (m_nFaces))) {
		Destroy ();
		return 0;
		}
	if (!(m_litFaces.Create (m_nFaces))) {
		Destroy ();
		return 0;
		}
	if (!(m_adjFaces.Create (m_nAdjFaces))) {
		Destroy ();
		return 0;
		}
	if (!(m_vertsf.Create (m_nVerts))) {
		Destroy ();
		return 0;
		}
	if (!(m_faceVerts.Create (m_nFaceVerts))) {
		Destroy ();
		return 0;
		}	
	}
if (!(m_vertNorms.Create (h))) {
	Destroy ();
	return 0;
	}
m_vertNorms.Clear ();
return m_nState = 1;
}

//------------------------------------------------------------------------------

void G3SetCullAndStencil (int bCullFront, int bZPass)
{
glEnable (GL_CULL_FACE);
if (bCullFront) {
	OglCullFace (1);
	if (bZPass)
		glStencilOp (GL_KEEP, GL_KEEP, GL_INCR_WRAP);
	else
		glStencilOp (GL_KEEP, GL_INCR_WRAP, GL_KEEP);
	}
else {
	OglCullFace (0);
	if (bZPass)
		glStencilOp (GL_KEEP, GL_KEEP, GL_DECR_WRAP);
	else
		glStencilOp (GL_KEEP, GL_DECR_WRAP, GL_KEEP);
	}
}

//------------------------------------------------------------------------------

void G3RenderShadowVolumeFace (CFloatVector *pv)
{
	CFloatVector	v [4];

memcpy (v, pv, 2 * sizeof (CFloatVector));
v [3] = v [0] - gameData.render.shadows.vLightPos;
v [2] = v [1] - gameData.render.shadows.vLightPos;
#if NORM_INF
v [3] *= fInf / v [3].Mag ();
v [2] *= fInf / v [2].Mag ();
#else
v [3] /= fInf;
v [2] /= fInf;
#endif
v [2] += v [1];
v [3] += v [0];
glVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), v);
#if DBG_SHADOWS
if (bShadowTest)
	glDrawArrays (GL_LINES, 0, 4);
else
#endif
glDrawArrays (GL_QUADS, 0, 4);
}

//------------------------------------------------------------------------------

void G3RenderFarShadowCapFace (CFloatVector *pv, int nVerts)
{
	CFloatVector	v0, v1;

#if DBG_SHADOWS
if (bShadowTest == 1)
	glColor4fv (reinterpret_cast<GLfloat*> (shadowColor));
else if (bShadowTest > 1)
	glColor4f (1.0f, 1.0f, 1.0f, 1.0f);
#	if 0
if (bShadowTest)
	glBegin (GL_LINE_LOOP);
else
#	endif
#endif
glBegin (GL_TRIANGLE_FAN);
for (pv += nVerts; nVerts; nVerts--) {
	v0 = *(--pv);
	v1 = v0 - vLightPosf;
#if NORM_INF
	v1 *= fInf / v1.Mag ();
#else
	v1 *= fInf;
#endif
	v0 += v1;
	glVertex3fv (reinterpret_cast<GLfloat*> (&v0));
	}
glEnd ();
}

//------------------------------------------------------------------------------

int CSubModel::RenderShadowVolume (CModel* po, int bCullFront)
{
	CFloatVector*	pvf, v [4];
	CFace*			pf, ** ppf;
	short*			pfv, * paf;
	short				h, i, j, n;
	float				fClipDist;
	int				nClip;

#if DBG_SHADOWS
if (!bShadowVolume)
	return 1;
if (bCullFront && !bBackFaces)
	return 1;
if (!bCullFront && !bFrontFaces)
	return 1;
if (bShadowTest == 1)
	glColor4fv (reinterpret_cast<GLfloat*> (shadowColor));
else if (bShadowTest > 1)
	glColor4f (1.0f, 1.0f, 1.0f, 1.0f);
#endif
G3SetCullAndStencil (bCullFront, bZPass);
pvf = po->m_vertsf.Buffer ();
#if DBG_SHADOWS
if (bShadowTest < 2)
	;
else if (bShadowTest == 2)
	glLineWidth (3);
else {
	glLineWidth (3);
	glBegin (GL_LINES);
	}
#endif
nClip = gameOpts->render.shadows.nClip ? po->m_fClipDist.Buffer () ? gameOpts->render.shadows.nClip : 1 : 0;
fClipDist = (nClip >= 2) ? m_fClipDist : fInf;
glVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), v);
for (i = m_nLitFaces, ppf = m_litFaces; i; i--, ppf++) {
	pf = *ppf;
	paf = po->m_adjFaces + pf->m_nAdjFaces;
	// walk through all edges of the current lit face and check whether the adjacent face does not face the light source
	// if so, that edge is a contour edge: Use it to render a shadow volume face ("side wall" of the shadow volume)
	for (j = 0, n = pf->m_nVerts, pfv = pf->m_verts; j < n; j++) {
		h = *paf++;
		if ((h < 0) || !m_faces [h].m_bFacingLight) {
			v [1] = pvf [pfv [j]];
			v [0] = pvf [pfv [(j + 1) % n]];
#if DBG_SHADOWS
			if (bShadowTest < 3) {
				glColor4fv (reinterpret_cast<GLfloat*> (shadowColor + bCullFront));
#endif
				v [3] = v [0] - vLightPosf;
				v [2] = v [1] - vLightPosf;
#if NORM_INF
				v [3] *= fClipDist / v [3].Mag ();
				v [2] *= fClipDist / v [2].Mag ();
#else
				v [3] *= fClipDist;
				v [2] *= fClipDist;
#endif
				v [2] += v [1];
				v [3] += v [0];
#if 1//!DBG
				glDrawArrays (GL_QUADS, 0, 4);
#else
				glVertex3fv (reinterpret_cast<GLfloat*> (v));
				glVertex3fv (reinterpret_cast<GLfloat*> (v+1));
				glVertex3fv (reinterpret_cast<GLfloat*> (v+2));
				glVertex3fv (reinterpret_cast<GLfloat*> (v+3));
#endif
#if DBG_SHADOWS
				}
			else {
				glColor4f (1.0f, 1.0f, 1.0f, 1.0f);
				glVertex3fv (reinterpret_cast<GLfloat*> (v));
				glVertex3fv (reinterpret_cast<GLfloat*> (v + 1));
				}
#endif
			}
		}
	}
#if DBG_SHADOWS
glLineWidth (1);
#endif
return 1;
}

//	-----------------------------------------------------------------------------

int LineHitsFace (CFixVector *pHit, CFixVector *p0, CFixVector *p1, short nSegment, short nSide)
{
	short			i, nFaces, nVerts;
	CSegment*	segP = SEGMENTS + nSegment;

nFaces = segP->Side (nSide)->FaceCount ();
nVerts = 5 - nFaces;
for (i = 0; i < nFaces; i++)
	if (segP->CheckLineToFace (*pHit, p0, p1, 0, nSide, i))
		return nSide;
return -1;
}

//	-----------------------------------------------------------------------------

float NearestShadowedWallDist (short nObject, short nSegment, CFixVector *vPos, float fScale)
{
	static float fClip [4] = {1.1f, 1.5f, 2.0f, 3.0f};

#if 1
	CFixVector	vHit, v, vh;
	CSegment		*segP;
	int			nSide, nHitSide, nParent, nChild, nWID, bHit = 0;
	float			fDist;
#if USE_SEGRADS
	fix			xDist;
#endif
	static		uint nVisited = 0;
	static		uint bVisited [MAX_SEGMENTS_D2X];

if (0 > (nSegment = FindSegByPos (*vPos, nSegment, 1, 0)))
	return G3_INFINITY;
v = *vPos - vLightPos;
CFixVector::Normalize(v);
v *= I2X (G3_INFINITY);
if (!nVisited++)
	memset (bVisited, 0, gameData.segs.nSegments * sizeof (uint));
#if DBG
if (bPrintLine) {
	CFloatVector	vf;
	glLineWidth (3);
	if (!bShadowTest) {
		glColorMask (1,1,1,1);
		glDisable (GL_STENCIL_TEST);
		}
	glColor4d (1,0.8,0,1);
	glBegin (GL_LINES);
	vf.Assign (*vPos);
	glVertex3fv (reinterpret_cast<GLfloat*> (&vf));
	vf.Assign (*vPos);
	glVertex3fv (reinterpret_cast<GLfloat*> (&vf));
	glEnd ();
	if (!bShadowTest) {
		glColorMask (0,0,0,0);
		glEnable (GL_STENCIL_TEST);
		}
	}
#endif
nParent = 0x7fffffff;
vHit = *vPos;
for (;;) {
	segP = SEGMENTS + nSegment;
	bVisited [nSegment] = nVisited;
	nHitSide = -1;
#if USE_SEGRADS
	for (nSide = 0; nSide < 6; nSide++) {
		nChild = segP->m_children [nSide];
		if ((nChild < 0) || (bVisited [nChild] == nVisited))
			continue;
		xDist = VmLinePointDist (vPos, &v, gameData.segs.segCenters [1] + nChild);
		if (xDist <= gameData.segs.segRads [0][nChild]) {
			nHitSide = LineHitsFace (&vHit, vPos, &v, nSegment, nSide);
			break;
			}
		}
	if (nHitSide < 0)
#endif
	 {
		for (nSide = 0; nSide < 6; nSide++) {
			nChild = segP->m_children [nSide];
			if ((nChild >= 0) && (bVisited [nChild] == nVisited))
				continue;
			if (0 <= LineHitsFace (&vHit, vPos, &v, nSegment, nSide)) {
				vh = vHit - *vPos;
				if (CFixVector::Dot (vh, v) > 0) {
					nHitSide = nSide;
					break;
					}
				}
			}
		}
	if (nHitSide < 0)
		break;
	fDist = X2F (CFixVector::Dist(*vPos, vHit));
	if (nChild < 0) {
		bHit = 1;
		break;
		}
	nWID = segP->IsDoorWay (nHitSide, OBJECTS + nObject);
	if (!(nWID & WID_FLY_FLAG) &&
		 (((nWID & (WID_RENDER_FLAG | WID_RENDPAST_FLAG)) != (WID_RENDER_FLAG | WID_RENDPAST_FLAG)))) {
		bHit = 1;
		break;
		}
	if (fDist >= G3_INFINITY)
		break;
	nParent = nSegment;
	nSegment = nChild;
	}
if (bHit)
	return fDist * (/*fScale ? fScale :*/ fClip [gameOpts->render.shadows.nReach]);
return G3_INFINITY;

#else //slower method

	tFVIQuery	fq;
	tFVIData		fi;
	CFixVector	v;

if (!gameOpts->render.shadows.nClip)
	return G3_INFINITY;
if (0 > (nSegment = FindSegByPos (vPos, nSegment, 1, 0)))
	return G3_INFINITY;
fq.p0				  = vPos;
VmVecSub (&v, fq.p0, &vLightPos);
CFixVector::Normalize(v);
VmVecScale (&v, I2X (G3_INFINITY));
fq.startSeg		  = nSegment;
fq.p1				  = &v;
fq.rad			  = 0;
fq.thisObjNum	  = nObject;
fq.ignoreObjList = NULL;
fq.flags			  = FQ_TRANSWALL;
fq.bCheckVisibility = false;
if (FindVectorIntersection (&fq, &fi) != HIT_WALL)
	return G3_INFINITY;
return //fScale ? X2F (VmVecDist (fq.p0, &fi.hit.vPoint)) * fScale :
		 X2F (VmVecDist (fq.p0, &fi.hit.vPoint)) * fClip [gameOpts->render.shadows.nReach];
#endif
}

//------------------------------------------------------------------------------

inline float CFace::ClipDist (CObject *objP)
{
	CFixVector	vCenter;
	
vCenter.Assign (m_vCenterf);
return NearestShadowedWallDist (objP->Index (), objP->info.nSegment, &vCenter, 0.0f);
}

//------------------------------------------------------------------------------
// use face centers to determine clipping distance

float CSubModel::ClipDistByFaceCenters (CObject *objP, CModel* po, int i, int incr)
{
	CFace*	pf, ** ppf;
	short		h;
	float		fClipDist, fMaxDist = 0;

for (h = m_nLitFaces, ppf = m_litFaces + i; i < h; i += incr, ppf += incr) {
	pf = *ppf;
	fClipDist = pf->ClipDist (objP);
#if DBG
	if (fClipDist == G3_INFINITY)
		fClipDist = pf->ClipDist (objP);
#endif
	if (fMaxDist < fClipDist)
		fMaxDist = fClipDist;
	}
return fMaxDist;
}

//------------------------------------------------------------------------------

float CSubModel::ClipDistByFaceVerts (CObject *objP, CModel* po, float fMaxDist, int i, int incr)
{
	CFloatVector*	pv;
	CFixVector		v;
	float*			pfc;
	CFace*			pf, ** ppf;
	short*			pfv, h, j, m, n;
	short				nObject = objP->Index ();
	short				nPointSeg, nSegment = objP->info.nSegment;
	float				fClipDist;

pv = po->m_vertsf.Buffer ();
pfc = po->m_fClipDist.Buffer ();
for (m = m_nLitFaces, ppf = m_litFaces + i; i < m; i += incr, ppf += incr) {
	pf = *ppf;
	for (j = 0, n = pf->m_nVerts, pfv = pf->m_verts; j < n; j++, pfv++) {
		h = *pfv;
		if (!(fClipDist = pfc [h])) {
			v.Assign (pv [h]);
			nPointSeg = FindSegByPos (v, nSegment, 1, 0);
			if (nPointSeg < 0)
				continue;
			pfc [h] = fClipDist = NearestShadowedWallDist (nObject, nPointSeg, &v, 3.0f);
#if DBG
			if (fClipDist == G3_INFINITY)
				fClipDist = NearestShadowedWallDist (nObject, nPointSeg, &v, 3.0f);
#endif
			}
		if (fMaxDist < fClipDist)
			fMaxDist = fClipDist;
		}
	}
return fMaxDist;
}

//------------------------------------------------------------------------------

float G3ClipDistByLitVerts (CObject *objP, CModel* po, float fMaxDist, int i, int incr)
{
	CFloatVector	*pv;
	CFixVector	v;
	float			*pfc;
	short			j;
	ubyte			*pvf;
	short			nObject = objP->Index ();
	short			nPointSeg, nSegment = objP->info.nSegment;
	float			fClipDist;
	ubyte			nVertFlag = po->m_nVertFlag;

pv = po->m_vertsf.Buffer ();
pfc = po->m_fClipDist.Buffer ();
pvf = po->m_vertFlags + i;
for (j = po->m_nVerts; i < j; i += incr, pvf += incr) {
	if (*pvf != nVertFlag)
		continue;
	if (!(fClipDist = pfc [i])) {
		v.Assign (pv [i]);
		nPointSeg = FindSegByPos (v, nSegment, 1, 0);
		if (nPointSeg < 0)
			continue;
		pfc [i] = fClipDist = NearestShadowedWallDist (nObject, nPointSeg, &v, 3.0f);
#if DBG
		if (fClipDist == G3_INFINITY)
			fClipDist = NearestShadowedWallDist (nObject, nPointSeg, &v, 3.0f);
#endif
		}
	if (fMaxDist < fClipDist)
		fMaxDist = fClipDist;
	}
return fMaxDist;
}

//------------------------------------------------------------------------------

#if MULTI_THREADED_SHADOWS

int _CDECL_ ClipDistThread (void *pThreadId)
{
	int		nId = *reinterpret_cast<int*> (pThreadId);

while (!gameStates.app.bExit) {
	while (!gameData.threads.clipDist.info [nId].bExec)
		G3_SLEEP (0);
	gameData.threads.clipDist.data.fClipDist [nId] =
		G3ClipDistByFaceCenters (gameData.threads.clipDist.data.objP,
										 gameData.threads.clipDist.data.po,
										 gameData.threads.clipDist.data.pso, nId, 2);
	if (gameOpts->render.shadows.nClip == 3)
		gameData.threads.clipDist.data.fClipDist [nId] =
			G3ClipDistByLitVerts (gameData.threads.clipDist.data.objP,
										 gameData.threads.clipDist.data.po,
										 gameData.threads.clipDist.data.fClipDist [nId],
										 nId, 2);
	gameData.threads.clipDist.info [nId].bExec = 0;
	}
return 0;
}

#endif

//------------------------------------------------------------------------------

void G3GetLitVertices (CModel* po, CSubModel* pso)
{
	CFace	*pf, **ppf;
	short			*pfv, i, j;
	ubyte			*pvf;
	ubyte			nVertFlag = po->m_nVertFlag++;

if (!nVertFlag)
	po->m_vertFlags.Clear ();
nVertFlag++;
pvf = po->m_vertFlags.Buffer ();
for (i = pso->m_nLitFaces, ppf = pso->m_litFaces; i; i--, ppf++) {
	pf = *ppf;
	for (j = pf->m_nVerts, pfv = pf->m_verts; j; j--, pfv++)
		pvf [*pfv] = nVertFlag;
	}
}

//------------------------------------------------------------------------------
// use face centers and vertices to determine clipping distance

float CSubModel::ClipDist (CObject *objP, CModel* po)
{
	float	fMaxDist = 0;

if (gameOpts->render.shadows.nClip < 2)
	return fInf;
#if 0
if (!m_bCalcClipDist)
	return m_fClipDist;	//only recompute every 2nd frame
m_bCalcClipDist = 0;
#endif
#if MULTI_THREADED_SHADOWS
	if (gameStates.app.bMultiThreaded) {
	gameData.threads.clipDist.data.objP = objP;
	gameData.threads.clipDist.data.po = po;
	gameData.threads.clipDist.data.pso = pso;
	G3GetLitVertices (po, pso);
	SDL_SemPost (gameData.threads.clipDist.info [0].exec);
	SDL_SemPost (gameData.threads.clipDist.info [1].exec);
	SDL_SemWait (gameData.threads.clipDist.info [0].done);
	SDL_SemWait (gameData.threads.clipDist.info [1].done);
	fMaxDist = (gameData.threads.clipDist.data.fClipDist [0] > gameData.threads.clipDist.data.fClipDist [1]) ?
					gameData.threads.clipDist.data.fClipDist [0] : gameData.threads.clipDist.data.fClipDist [1];
	}
else
#endif
 {
	fMaxDist = ClipDistByFaceCenters (objP, po, 0, 1);
	if (gameOpts->render.shadows.nClip == 3)
		fMaxDist = ClipDistByFaceVerts (objP, po, fMaxDist, 0, 1);
	}
return m_fClipDist = (fMaxDist ? fMaxDist : (fInf < G3_INFINITY) ? fInf : G3_INFINITY);
}

//------------------------------------------------------------------------------

int CSubModel::RenderShadowCaps (CObject *objP, CModel* po, int bCullFront)
{
	CFloatVector*	pvf, v0, v1;
	CFace*			pf, ** ppf;
	short*			pfv, i, j;
	float				fClipDist;
	int				nClip;

#if DBG_SHADOWS
if (bShadowTest) {
	glColor4fv (reinterpret_cast<GLfloat*> (modelColor + bCullFront));
	glDepthFunc (GL_LEQUAL);
	}
#endif
G3SetCullAndStencil (bCullFront, bZPass);
pvf = po->m_vertsf.Buffer ();
#if DBG_SHADOWS
if (bRearCap)
#endif
nClip = gameOpts->render.shadows.nClip ? po->m_fClipDist.Buffer () ? gameOpts->render.shadows.nClip : 1 : 0;
fClipDist = (nClip >= 2) ? m_fClipDist : fInf;
for (i = m_nLitFaces, ppf = m_litFaces; i; i--, ppf++) {
	pf = *ppf;
#if DBG
	if (pf->m_bFacingLight && (bShadowTest > 3)) {
		glColor4f (0.20f, 0.8f, 1.0f, 1.0f);
		v1 = v0 = pf->m_vCenterf;
		glBegin (GL_LINES);
		glVertex3fv (reinterpret_cast<GLfloat*> (&v0));
		v0 += pf->m_vNormf;
		glVertex3fv (reinterpret_cast<GLfloat*> (&v0));
		glEnd ();
		glColor4d (0,0,1,1);
		glBegin (GL_LINES);
		glVertex3fv (reinterpret_cast<GLfloat*> (&v1));
		glVertex3fv (reinterpret_cast<GLfloat*> (&vLightPosf));
		glEnd ();
		glColor4fv (reinterpret_cast<GLfloat*> (modelColor + bCullFront));
		}
	if (bShadowTest && (bShadowTest != 2)) {
		glLineWidth (1);
		glBegin (GL_LINE_LOOP);
		}
	else
#endif
		glBegin (GL_TRIANGLE_FAN);
	for (j = pf->m_nVerts, pfv = pf->m_verts + j; j; j--) {
		v0 = pvf [*--pfv];
#if 1
		v1 = v0 - vLightPosf;
#if DBG_SHADOWS
		if (bShadowTest < 4)
#endif
		 {
#if NORM_INF
#	if DBG_SHADOWS
			if (bShadowTest == 2)
				v1 *= 5.0f / v1.Mag ();
			else
#	endif
				v1 *= fClipDist / v1.Mag ();
#else
			v1 *= fClipDist;
#endif
			v0 += v1;
#endif
			}
		glVertex3fv (reinterpret_cast<GLfloat*> (&v0));
		}
	glEnd ();
	}
#if DBG_SHADOWS
if (bFrontCap)
#endif
for (i = m_nLitFaces, ppf = m_litFaces; i; i--, ppf++) {
	pf = *ppf;
	if (!pf->m_bFacingLight)
		continue;
#if DBG_SHADOWS
	if (pf->m_bFacingLight && (bShadowTest > 3)) {
		glColor4f (1.0f, 0.8f, 0.2f, 1.0f);
		v1 = v0 = pf->m_vCenterf;
		glBegin (GL_LINES);
		glVertex3fv (reinterpret_cast<GLfloat*> (&v0));
		v0 += pf->m_vNormf;
		glVertex3fv (reinterpret_cast<GLfloat*> (&v0));
		glEnd ();
		glColor4d (1,0,0,1);
		glBegin (GL_LINES);
		glVertex3fv (reinterpret_cast<GLfloat*> (&v1));
		glVertex3fv (reinterpret_cast<GLfloat*> (&vLightPosf));
		glEnd ();
		glColor4fv (reinterpret_cast<GLfloat*> (modelColor + bCullFront));
		}
	if (bShadowTest && (bShadowTest != 2)) {
		glLineWidth (1);
		glBegin (GL_LINE_LOOP);
		}
	else
#endif
		glBegin (GL_TRIANGLE_FAN);
	for (j = pf->m_nVerts, pfv = pf->m_verts; j; j--) {
		v0 = pvf [*pfv++];
		glVertex3fv (reinterpret_cast<GLfloat*> (&v0));
		}
	glEnd ();
	}
return 1;
}

//------------------------------------------------------------------------------

int CSubModel::RenderShadow (CObject *objP, CModel* po)
{
	int			h = 1, i;

if (m_nParent >= 0)
	transformation.Begin (m_vPos, m_vAngles);
h = (int) (this - po->m_subModels);
for (i = 0; i < po->m_nSubModels; i++)
	if (po->m_subModels [i].m_nParent == h)
		po->m_subModels [i].RenderShadow (objP, po);
#if DBG
#	if 0
if (pso - po->m_subModels == 8)
#	endif
#endif
{
GatherLitFaces (po);
if ((m_nRenderFlipFlop = !m_nRenderFlipFlop))
	m_fClipDist = ClipDist (objP, po);
h = RenderShadowCaps (objP, po, 0) &&
	 RenderShadowCaps (objP, po, 1) &&
	 RenderShadowVolume (po, 0) &&
	 RenderShadowVolume (po, 1);
;
}
if (m_nParent >= 0)
	transformation.End ();
return h;
}

//------------------------------------------------------------------------------

int POFGatherPolyModelItems (CObject *objP, void *modelDataP, CAngleVector *animAngleP, CModel* po, int bShadowData)
{
	int				j;
	CFixVector*		pv;
	CFloatVector*	pvf;

if (!(po->m_nState || po->Create (modelDataP, bShadowData)))
	return 0;
if (po->m_nState == 1) {
	transformation.Begin (objP->info.position.vPos, objP->info.position.mOrient);
	po->GatherItems (modelDataP, animAngleP, 1, bShadowData, 0, -1);
	if (bShadowData) {
		CFixVector vCenter;
		vCenter.SetZero ();
		for (j = po->m_nVerts, pv = po->m_verts.Buffer (), pvf = po->m_vertsf.Buffer (); j; j--, pv++, pvf++) 
			vCenter += *pv;
		float fScale = (static_cast<float>(po->m_nVerts) * 65536.0f);
		vCenterf.Assign (vCenter);
		vCenterf /= fScale;
		po->m_vCenter += vCenter;

		po->GatherAdjFaces ();
		po->CalcCenters ();
		}
	po->m_nState = 2;
	transformation.End ();
	}
if (bShadowData) {
	po->m_iSubObj = 0;
	transformation.Begin (objP->info.position.vPos, objP->info.position.mOrient);
	po->GatherItems (modelDataP, animAngleP, 0, 1, 0, -1);
	transformation.End ();
	}
return 1;
}

//	-----------------------------------------------------------------------------

int G3DrawPolyModelShadow (CObject *objP, void *modelDataP, CAngleVector *animAngleP, int nModel)
{
#if SHADOWS
	CFixVector	v, vLightDir;
	short*		pnl;
	int			h, i, j;
	CModel*		po = gameData.models.pofData [gameStates.app.bD1Mission][1] + nModel;

Assert (objP->info.nId < MAX_ROBOT_TYPES);
if (!gameStates.render.bShadowMaps) {
	if (!POFGatherPolyModelItems (objP, modelDataP, animAngleP, po, 1))
		return 0;
	}
OglActiveTexture (GL_TEXTURE0, 0);
glDisable (GL_TEXTURE_2D);
glEnableClientState (GL_VERTEX_ARRAY);
pnl = lightManager.NearestSegLights  () + objP->info.nSegment * MAX_NEAREST_LIGHTS;
gameData.render.shadows.nLight = 0;
if (FAST_SHADOWS) {
	for (i = 0; (gameData.render.shadows.nLight < gameOpts->render.shadows.nLights) && (*pnl >= 0); i++, pnl++) {
		gameData.render.shadows.lightP = lightManager.RenderLights (*pnl);
		if (!gameData.render.shadows.lightP->info.bState)
			continue;
		if (!CanSeePoint (objP, &objP->info.position.vPos, &gameData.render.shadows.lightP->info.vPos, objP->info.nSegment))
			continue;
		vLightDir = objP->info.position.vPos - gameData.render.shadows.lightP->info.vPos;
		CFixVector::Normalize (vLightDir);
		if (gameData.render.shadows.nLight) {
			for (j = 0; j < gameData.render.shadows.nLight; j++)
				if (abs (CFixVector::Dot (vLightDir, gameData.render.shadows.vLightDir [j])) > I2X (2) / 3) // 60 deg
					break;
			if (j < gameData.render.shadows.nLight)
				continue;
			}
		gameData.render.shadows.vLightDir [gameData.render.shadows.nLight++] = vLightDir;
		if (gameStates.render.bShadowMaps)
			RenderShadowMap (gameData.render.shadows.lightP);
		else {
			gameStates.render.bRendering = 1;
			transformation.Transform (vLightPos, gameData.render.shadows.lightP->info.vPos, 0);
			vLightPosf.Assign (vLightPos);
			if (gameOpts->render.shadows.nClip) {
				// get a default clipping distance using the model position as fall back
				transformation.Transform (v, objP->info.position.vPos, 0);
				fInf = NearestShadowedWallDist (objP->Index (), objP->info.nSegment, &v, 0);
				}
			else
				fInf = G3_INFINITY;
			po->VertsToFloat ();
			transformation.Begin (objP->info.position.vPos, objP->info.position.mOrient);
			po->m_litFaces.Reset ();
			if (gameOpts->render.shadows.nClip >= 2)
				po->m_fClipDist.Clear ();
			po->m_subModels [0].RenderShadow (objP, po);
			transformation.End ();
			gameStates.render.bRendering = 0;
			}
		}
	}
else {
	h = objP->Index ();
	j = int (gameData.render.shadows.lightP - lightManager.Lights ());
	pnl = gameData.render.shadows.objLights + h * MAX_SHADOW_LIGHTS;
	for (i = 0; i < gameOpts->render.shadows.nLights; i++, pnl++) {
		if (*pnl < 0)
			break;
		if (*pnl == j) {
			vLightPosf = gameData.render.shadows.vLightPos;
			po->VertsToFloat ();
			transformation.Begin (objP->info.position.vPos, objP->info.position.mOrient);
			po->m_litFaces.Reset ();
			po->m_subModels [0].RenderShadow (objP, po);
			transformation.End ();
			}
		}
	}
#endif
glDisableClientState (GL_VERTEX_ARRAY);
return 1;
}

//------------------------------------------------------------------------------
//eof
