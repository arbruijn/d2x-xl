#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif
//#include  "oof.h
#include "descent.h"
#include "args.h"
#include "u_mem.h"
#include "error.h"
#include "light.h"
#include "dynlight.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "network.h"
#include "rendermine.h"
#include "strutil.h"

using namespace OOF;

//------------------------------------------------------------------------------

#define NORM_INF		1

#ifdef __unix
#	ifndef stricmp
#		define	stricmp	strcasecmp
#	endif
#	ifndef strnicmp
#		define	strnicmp	strncasecmp
#	endif
#endif

#if DBG_SHADOWS
extern int32_t bShadowTest;
extern int32_t bSingleStencil;

int32_t bFrontCap = 1;
int32_t bRearCap = 1;
int32_t bFrontFaces = 1;
int32_t bBackFaces = 1;
int32_t bShadowVolume = 1;
int32_t bWallShadows = 1;
int32_t bSWCulling = 0;
#endif

int32_t bZPass = 0;

static CFloatVector vPos;
static CFloatVector vrLightPos;
static CFloatMatrix mView;

//------------------------------------------------------------------------------

int32_t OOF_FacingPoint (CFloatVector *modelVertP, CFloatVector *pn, CFloatVector *pp)
{
	CFloatVector	v = *pp - *modelVertP;

return CFloatVector::Dot (v, *pn) > 0;
}

//------------------------------------------------------------------------------

int32_t OOF_FacingViewer (CFloatVector *modelVertP, CFloatVector *pn)
{
return OOF_FacingPoint (modelVertP, pn, &vPos);
}

//------------------------------------------------------------------------------

int32_t OOF_FacingLight (CFloatVector *modelVertP, CFloatVector *pn)
{
return OOF_FacingPoint (modelVertP, pn, &vrLightPos);
}

//------------------------------------------------------------------------------

CFloatVector *OOF_CalcFacePerp (CSubModel *pso, CFace *pFace)
{
	CFloatVector*	modelVertP = pso->m_vertices.Buffer ();
	CFaceVert*		pFaceVertex = pFace->m_vertices;

CFloatVector::Normalize (CFloatVector::Perp (pFace->m_vRotNormal, modelVertP [pFaceVertex [0].m_nIndex], modelVertP [pFaceVertex [1].m_nIndex], modelVertP [pFaceVertex [2].m_nIndex]));
return &pFace->m_vRotNormal;
}

//------------------------------------------------------------------------------

int32_t OOF_LitFace (CSubModel *pso, CFace *pFace)
{
//OOF_CalcFacePerp (pso, pFace);
return pFace->m_bFacingLight = 
#if 1
	OOF_FacingLight (&pFace->m_vRotCenter, &pFace->m_vRotNormal); 
#else
	OOF_FacingLight (pso->m_rotVerts + pFace->m_vertices->m_nIndex, &pFace->m_vRotNormal); //OOF_CalcFacePerp (pso, pFace)); 
#endif
}

//------------------------------------------------------------------------------

int32_t OOF_FrontFace (CSubModel *pso, CFace *pFace)
{
#if 0
return OOF_FacingViewer (&pFace->m_vRotCenter, &pFace->m_vRotNormal);
#else
return OOF_FacingViewer (pso->m_rotVerts + pFace->m_vertices->m_nIndex, &pFace->m_vRotNormal);	//OOF_CalcFacePerp (pso, pFace));
#endif
}

//------------------------------------------------------------------------------

int32_t OOF_GetLitFaces (CSubModel *pso)
{
	CFace		*pFace;
	int32_t				i;

for (i = pso->m_faces.m_nFaces, pFace = pso->m_faces.m_list.Buffer (); i; i--, pFace++) {
	pFace->m_bFacingLight = OOF_LitFace (pso, pFace);
#if 0
	if (bSWCulling)
		pFace->m_bFacingViewer = OOF_FrontFace (pso, pFace);
#endif
	}
return pso->m_faces.m_nFaces;
}

//------------------------------------------------------------------------------

int32_t OOF_GetSilhouette (CSubModel *pso)
{
	OOF::CEdge	*pe;
	int32_t		h, i, j;

OOF_GetLitFaces (pso);
for (h = j = 0, i = pso->m_edges.m_nEdges, pe = pso->m_edges.m_list.Buffer (); i; i--, pe++) {
	if (pe->m_faces [0] && pe->m_faces [1]) {
		if ((pe->m_bContour = (pe->m_faces [0]->m_bFacingLight != pe->m_faces [1]->m_bFacingLight)))
			h++;
		}
	else {
#if 0
		pe->m_bContour = 0;
#else
		pe->m_bContour = 1;
		h++;
#endif
		j++;
		}
	}
return pso->m_edges.m_nContourEdges = h;
}

//------------------------------------------------------------------------------

void SetCullAndStencil (int32_t bCullFront, int32_t bZPass = 0);

void OOF_SetCullAndStencil (int32_t bCullFront)
{
SetCullAndStencil (bCullFront);
}

//------------------------------------------------------------------------------

int32_t OOF_DrawShadowVolume (CModel *po, CSubModel *pso, int32_t bCullFront)
{
if (bCullFront && ogl.m_features.bSeparateStencilOps.Available ())
	return 1;

	OOF::CEdge		*pe;
	CFloatVector	*modelVertP;
	int32_t			nVerts, nEdges;

if (!bCullFront)
	OOF_GetSilhouette (pso);
#if DBG_SHADOWS
if (bCullFront && !bBackFaces)
	return 1;
if (!bCullFront && !bFrontFaces)
	return 1;
if (!bShadowVolume)
	return 1;
if (bShadowTest > 3)
	return 1;
if (bShadowTest < 2)
	glColor4fv (reinterpret_cast<GLfloat*> (shadowColor + bCullFront));
#endif
OOF_SetCullAndStencil (bCullFront);

#if DBG_SHADOWS
if (bShadowTest == 1)
	glColor4fv (reinterpret_cast<GLfloat*> (shadowColor + bCullFront));
else if (bShadowTest > 1)
	glColor4f (1.0f, 1.0f, 1.0f, 1.0f);
#endif

nEdges = pso->m_edges.m_nContourEdges;
if (!ogl.Buffers ().SizeVertices (nEdges * 4))
	return 1;
modelVertP = pso->m_rotVerts.Buffer ();
for (nVerts = 0, pe = pso->m_edges.m_list.Buffer (); nEdges; pe++) {
	if (pe->m_bContour) {
		nEdges--;
#if DBG_SHADOWS
		if (bShadowTest < 2) {
#endif
			int32_t h = (pe->m_faces [1] && pe->m_faces [1]->m_bFacingLight);
			if (pe->m_faces [h]->m_bReverse)
				h = !h;
			ogl.VertexBuffer () [nVerts] = modelVertP [pe->m_v1 [h]];
			ogl.VertexBuffer () [nVerts + 1] = modelVertP [pe->m_v0 [h]];
			ogl.VertexBuffer () [nVerts + 2] = ogl.VertexBuffer () [1] - vrLightPos;
			ogl.VertexBuffer () [nVerts + 3] = ogl.VertexBuffer () [0] - vrLightPos;
#if NORM_INF
			ogl.VertexBuffer () [nVerts + 2] *= G3_INFINITY / ogl.VertexBuffer () [nVerts + 2].Mag ();
			ogl.VertexBuffer () [nVerts + 3] *= G3_INFINITY / ogl.VertexBuffer () [nVerts + 3].Mag ();
#else
			ogl.VertexBuffer () [nVerts + 2] *= G3_INFINITY;
			ogl.VertexBuffer () [nVerts + 3] *= G3_INFINITY;
#endif
			ogl.VertexBuffer () [nVerts + 2] += ogl.VertexBuffer () [nVerts + 1];
			ogl.VertexBuffer () [nVerts + 3] += ogl.VertexBuffer () [nVerts];
			nVerts += 4;
#if DBG_SHADOWS
			}
		else {
			ogl.VertexBuffer () [nVerts++] = modelVertP [pe->m_v0 [0]];
			ogl.VertexBuffer () [nVerts++] = modelVertP [pe->m_v1 [0]];
			}
#endif
		}
	}
OglVertexPointer (3, GL_FLOAT, 0, ogl.VertexBuffer ().Buffer ());
#if DBG_SHADOWS
ogl.FlushBuffers ((bShadowTest < 2) ? GL_QUADS : GL_LINES, nVerts);
#else
ogl.FlushBuffers (GL_QUADS, nVerts);
#endif
#if DBG_SHADOWS
ogl.SetFaceCulling (true);
#endif
return 1;
}

//------------------------------------------------------------------------------

int32_t OOF_DrawShadowCaps (CModel *po, CSubModel *pso, int32_t bCullFront)
{
if (bCullFront && ogl.m_features.bSeparateStencilOps.Available ())
	return 1;

	CFace*			pFace;
	CFaceVert*		pFaceVertex;
	CFloatVector*	modelVertP, v0, v1;
	int32_t				nVerts, i, j, bReverse;

#if DBG_SHADOWS
if (bZPass)
	return 1;
if (bShadowTest > 4)
	return 1;
glColor4fv (reinterpret_cast<GLfloat*> (modelColor));
#endif
modelVertP = pso->m_rotVerts.Buffer ();
OOF_SetCullAndStencil (bCullFront);

nVerts = 0;
for (i = pso->m_faces.m_nFaces, pFace = pso->m_faces.m_list.Buffer (); i; i--, pFace++)
	nVerts += pFace->m_nVerts;

//if (bCullFront) 
{
#if DBG_SHADOWS
	if (!bRearCap)
		return 1;
#endif
	for (bReverse = 0; bReverse <= 1; bReverse++) {
		nVerts = 0;
		if (bReverse)
			glFrontFace (GL_CCW);
		for (i = pso->m_faces.m_nFaces, pFace = pso->m_faces.m_list.Buffer (); i; i--, pFace++) {
			if (pFace->m_bReverse == bReverse) {
				j = pFace->m_nVerts;
				if (!ogl.Buffers ().SizeVertices (j))
					return 1;
				nVerts = 0;
				for (pFaceVertex = pFace->m_vertices; j; j--, pFaceVertex++) {
					v0 = modelVertP [pFaceVertex->m_nIndex];
					v1 = v0 - vrLightPos;
#if NORM_INF
					v1 *= G3_INFINITY / v1.Mag ();
#else
					v1 *= G3_INFINITY;
#endif
					ogl.VertexBuffer () [nVerts++] = v0 + v1;
					}
				ogl.FlushBuffers (GL_TRIANGLE_FAN, nVerts);
				}
			}
		if (bReverse)
			glFrontFace (GL_CW);
		}
	}
//else
	{
#if DBG_SHADOWS
	if (!bFrontCap)
		return 1;
#endif
	for (bReverse = 0; bReverse <= 1; bReverse++) {
		nVerts = 0;
		for (i = pso->m_faces.m_nFaces, pFace = pso->m_faces.m_list.Buffer (); i; i--, pFace++) {
			if (pFace->m_bReverse == bReverse) {
				j = pFace->m_nVerts;
				if (!ogl.Buffers ().SizeVertices (j))
					return 1;
				nVerts = 0;
				for (pFaceVertex = pFace->m_vertices; j; j--, pFaceVertex++)
					ogl.VertexBuffer () [nVerts++] = modelVertP [pFaceVertex->m_nIndex];
				}
			ogl.FlushBuffers (GL_TRIANGLE_FAN, nVerts);
			}
		if (bReverse)
			glFrontFace (GL_CW);
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t OOF_DrawShadow (CModel *po, CSubModel *pso)
{
#if 1
return 1;	// D2 oof models aren't 'shadow proof'
#else
return OOF_DrawShadowVolume (po, pso, 0) && 
		 OOF_DrawShadowVolume (po, pso, 1) && 
		 OOF_DrawShadowCaps (po, pso, 0) && 
		 OOF_DrawShadowCaps (po, pso, 1); 
#endif
}

//------------------------------------------------------------------------------

int32_t CSubModel::Draw (CObject *pObj, CModel *po, float *fLight)
{
	CFace*			pFace;
	CFaceVert*		pFaceVertex;
	CFloatVector*	modelVertP, * modelNormalP, * phv;
	CFaceColor*		pVertexColor, vertColor, segColor;
	CBitmap*			pBm = NULL;
	int32_t				h, i, j, nModelVerts [2], nVerts, nFaceVerts;
	int32_t				bBright = EGI_FLAG (bBrightObjects, 0, 1, 0), bTextured = -1, bReverse;
	int32_t				bDynLighting = gameStates.render.bApplyDynLight;
	float				fl, r, g, b, fAlpha = po->m_fAlpha;
	// helper pointers into render buffers
	CFloatVector*	pVertex;
	CFloatVector*	pColor;
	tTexCoord2f*	pTexCoord;

segColor.Set (1.0f, 1.0f, 1.0f, 1.0f);
segColor.index = 1;
#if DBG_SHADOWS
if (bShadowTest && (bShadowTest < 4))
	return 1;
#endif
modelVertP = m_rotVerts.Buffer ();
modelNormalP = m_normals.Buffer ();
pVertexColor = m_vertColors.Buffer ();
//memset (pVertexColor, 0, m_nVerts * sizeof (CFaceColor));
ogl.SetFaceCulling (true);
OglCullFace (0);
ogl.SetBlendMode (OGL_BLEND_ALPHA);
ogl.SelectTMU (GL_TEXTURE0);
if (!bDynLighting) {
	segColor = *lightManager.AvgSgmColor (pObj->info.nSegment, &pObj->info.position.vPos, 0);
	if (segColor.index != gameStates.render.nFrameFlipFlop + 1)
		segColor.Red () = segColor.Green () = segColor.Blue () = 1;
	}

for (bReverse = 0; bReverse <= 1; bReverse++) {
	nModelVerts [0] = nModelVerts [1] = 0;
	for (i = m_faces.m_nFaces, pFace = m_faces.m_list.Buffer (); i; i--, pFace++) {
		if (pFace->m_bReverse == bReverse)
			nModelVerts [pFace->m_bTextured] += pFace->m_nVerts;
		}
	if (!(nVerts = nModelVerts [nModelVerts [1] > nModelVerts [0]]))
		continue;
	if (!ogl.Buffers ().SizeBuffers (nVerts))
		return 0;

	if (bReverse)
		glFrontFace (GL_CCW);

	pVertex = ogl.VertexBuffer ().Buffer ();
	pColor = ogl.ColorBuffer ().Buffer ();
	pTexCoord = ogl.TexCoordBuffer ().Buffer ();

	nVerts = 0;
	nFaceVerts = -1;
	for (i = m_faces.m_nFaces, pFace = m_faces.m_list.Buffer (); i; i--, pFace++) {
		if (pFace->m_bReverse != bReverse)
			continue;
		if (nFaceVerts != pFace->m_nVerts) {
			if (nVerts && (nFaceVerts != -1)) {
				ogl.FlushBuffers ((nFaceVerts == 3) ? GL_TRIANGLES : /*(nFaceVerts == 4) ? GL_QUADS :*/ GL_TRIANGLE_FAN, nVerts, 3, bTextured, bTextured);
				nVerts = 0;
				bTextured = -1;
				}
			nFaceVerts = pFace->m_nVerts;
			}
		pFaceVertex = pFace->m_vertices;
		if (pFace->m_bTextured) {
			//if (bTextured == 0) {
			//	ogl.FlushBuffers ((nFaceVerts == 3) ? GL_TRIANGLES : /*(nFaceVerts == 4) ? GL_QUADS :*/ GL_TRIANGLE_FAN, nVerts, 3, 0, 0);
			//	nFaceVerts = pFace->m_nVerts;
			//	nVerts = 0;
			//	}

			if (pBm != po->m_textures.m_bitmaps + pFace->m_texProps.nTexId) {
				if (nVerts) {
					ogl.FlushBuffers ((nFaceVerts == 3) ? GL_TRIANGLES : /*(nFaceVerts == 4) ? GL_QUADS :*/ GL_TRIANGLE_FAN, nVerts, 3, 1, 1);
					nVerts = 0;
					}
				pBm = po->m_textures.m_bitmaps + pFace->m_texProps.nTexId;
				if (pBm->Texture () && ((int32_t) pBm->Texture ()->Handle () < 0))
					pBm->Texture ()->SetHandle (0);
				pBm->SetTranspType (0);
				ogl.SetTexturing (true);
				if (pBm->Bind (1))
					return 0;
				pBm->Texture ()->Wrap (GL_REPEAT);
				}
			bTextured = 1;

			fl = *fLight * (0.75f - 0.25f * (pFace->m_vNormal * mView.m.dir.f));
			if (fl > 1)
				fl = 1;
#if DBG
			if (m_nFlags & OOF_SOF_LAYER)
				BRP;
#endif
			if (m_nFlags & (bDynLighting ? OOF_SOF_THRUSTER : (OOF_SOF_GLOW | OOF_SOF_THRUSTER))) {
				pColor [nVerts].Red () = fl * m_glowInfo.m_color.Red ();
				pColor [nVerts].Green () = fl * m_glowInfo.m_color.Green ();
				pColor [nVerts].Blue () = fl * m_glowInfo.m_color.Blue ();
				pColor [nVerts].Alpha () = m_pfAlpha [pFaceVertex->m_nIndex] * fAlpha;
				}
			else if (!bDynLighting) {
				if (bBright)
					fl += (1 - fl) / 2;
				pColor [nVerts].Red () = segColor.Red () * fl;
				pColor [nVerts].Green () = segColor.Green () * fl;
				pColor [nVerts].Blue () = segColor.Blue () * fl;
				pColor [nVerts].Alpha () = m_pfAlpha [pFaceVertex->m_nIndex] * fAlpha;
				}
			for (j = pFace->m_nVerts; j; j--, pFaceVertex++) {
				phv = modelVertP + (h = pFaceVertex->m_nIndex);
				if (bDynLighting) {
					if (pVertexColor [h].index != gameStates.render.nFrameFlipFlop + 1)
						GetVertexColor (-1, -1, -1, reinterpret_cast<CFloatVector3*> (modelNormalP + h), reinterpret_cast<CFloatVector3*> (phv), pVertexColor + h, NULL, 1, 0, 0);
					
					vertColor.Red () = (float) sqrt (pVertexColor [h].Red ());
					vertColor.Green () = (float) sqrt (pVertexColor [h].Green ());
					vertColor.Blue () = (float) sqrt (pVertexColor [h].Blue ());
					if (bBright) {
						vertColor.Red () += (1.0f - vertColor.Red ()) * 0.5f;
						vertColor.Green () += (1.0f - vertColor.Green ()) * 0.5f;
						vertColor.Blue () += (1.0f - vertColor.Blue ()) * 0.5f;
						}
					vertColor.Alpha () = m_pfAlpha [pFaceVertex->m_nIndex] * fAlpha;
					pColor [nVerts] = vertColor;
					}
				pTexCoord [nVerts].v.u = pFaceVertex->m_fu;
				pTexCoord [nVerts].v.v = pFaceVertex->m_fv;
				pVertex [nVerts++] = *phv;
				}
#if DBG_SHADOWS
			if (pFace->m_bFacingLight && (bShadowTest > 3)) {
					CFloatVector	fv0;

				glLineWidth (3);
				glColor4f (1.0f, 0.0f, 0.5f, 1.0f);
				glBegin (GL_LINES);
				fv0 = pFace->m_vRotCenter;
				glVertex3fv (reinterpret_cast<GLfloat*> (&fv0));
				fv0 += pFace->m_vRotNormal;
				glVertex3fv (reinterpret_cast<GLfloat*> (&fv0));
				glEnd ();
				glColor4f (0.0f, 1.0f, 0.5f, 1.0f);
				glBegin (GL_LINES);
				fv0 = pFace->m_vRotCenter;
				glVertex3fv (reinterpret_cast<GLfloat*> (&fv0));
				fv0 = vrLightPos;
				glVertex3fv (reinterpret_cast<GLfloat*> (&fv0));
				glEnd ();
				glLineWidth (1);
				}
#endif
			}
		else {
			if (bTextured > 0) {
				ogl.FlushBuffers (GL_TRIANGLE_FAN, nVerts, 3, 1, 1);
				nVerts = 0;
				ogl.SetTexturing (false);
				pBm = NULL;
				bTextured = 0;
				}
			fl = fLight [1];
			r = fl * (float) pFace->m_texProps.color.Red () / 255.0f;
			g = fl * (float) pFace->m_texProps.color.Green () / 255.0f;
			b = fl * (float) pFace->m_texProps.color.Blue () / 255.0f;
			glColor4f (r, g, b, m_pfAlpha [pFaceVertex->m_nIndex] * fAlpha);
			for (j = pFace->m_nVerts; j; j--, pFaceVertex++) 
				pVertex [nVerts++] = modelVertP [pFaceVertex->m_nIndex];
			}
		}
	if (bTextured != -1)
		ogl.FlushBuffers ((nFaceVerts == 3) ? GL_TRIANGLES : /*(nFaceVerts == 4) ? GL_QUADS :*/ GL_TRIANGLE_FAN, nVerts, 3, bTextured, bTextured);

	}
glFrontFace (GL_CW);
return 1;
}

//------------------------------------------------------------------------------

inline void CSubModel::TransformVertex (CFloatVector *prv, CFloatVector *modelVertP, CFloatVector *vo)
{
	CFloatVector	v;

if (ogl.m_states.bUseTransform)
	*prv = *modelVertP + *vo;
else {
	v = *modelVertP - vPos;
	v += *vo;
	*prv = mView * v;
	}
}

//------------------------------------------------------------------------------

void CSubModel::Transform (CFloatVector vo)
{
	CFloatVector	*modelVertP, *prv;
	CFace				*pFace;
	int32_t				i;

for (i = m_nVerts, modelVertP = m_vertices.Buffer (), prv = m_rotVerts.Buffer (); i; i--, modelVertP++, prv++)
	TransformVertex (prv, modelVertP, &vo);
for (i = m_faces.m_nFaces, pFace = m_faces.m_list.Buffer (); i; i--, pFace++) {
#if OOF_TEST_CUBE
	if (i == 6)
		pFace->m_bReverse = 1;
	else
#endif
	pFace->m_bReverse = 0; //(pFace->m_vRotNormal * pFace->m_vNormal) < 0;
#if OOF_TEST_CUBE
	if (i == 6)
		pFace->m_vNormal.z = (float) fabs (pFace->m_vNormal.z);	//fix flaw in model data
#endif
	CFloatVector::Normalize (pFace->m_vRotNormal = mView * pFace->m_vNormal);
	TransformVertex (&pFace->m_vRotCenter, &pFace->m_vCenter, &vo);
	}
}

//------------------------------------------------------------------------------

int32_t CSubModel::Render (CObject *pObj, CModel *po, CFloatVector vo, int32_t nIndex, float *fLight)
{
	CSubModel	*pso;
	int32_t			i, j;

vo += m_vOffset;
Transform (vo);
//if ((gameStates.render.nShadowPass != 2) && (bFacing != ((m_nFlags & OOF_SOF_FACING) != 0)))
//	return 1;
#if 1
for (i = 0; i < m_nChildren; i++) {
	pso = po->m_subModels + (j = m_children [i]);
	Assert (j >= 0 && j < po->m_nSubModels);
	if (pso->m_nParent == m_nIndex)
		if (!pso->Render (pObj, po, vo, j, fLight))
			return 0;
	}
#endif
if (gameStates.render.nShadowPass == 2)
	OOF_DrawShadow (po, po->m_subModels + nIndex);
else
	Draw (pObj, po, fLight);
ogl.SetFaceCulling (true);
return 1;
}

//------------------------------------------------------------------------------

int32_t CModel::Draw (CObject *pObj, float *fLight)
{
	CSubModel		*pso;
	int32_t				r = 1, i;
	CFloatVector	vo;

vo.SetZero ();
transformation.Begin (pObj->info.position.vPos, pObj->info.position.mOrient);
if (!ogl.m_states.bUseTransform)
	mView.Assign (transformation.m_info.view [0]);
vPos.Assign (transformation.m_info.pos);
if (IsMultiGame && netGameInfo.m_info.BrightPlayers)
	*fLight = 1.0f;
ogl.SelectTMU (GL_TEXTURE0);
ogl.SetTexturing (true);
for (i = 0, pso = m_subModels.Buffer (); i < m_nSubModels; i++, pso++)
	if (pso->m_nParent == -1) {
		if (!pso->Render (pObj, this, vo, i, fLight)) {
			r = 0;
			break;
			}
		}
transformation.End ();
ogl.SetTexturing (false);
return r;
}

//------------------------------------------------------------------------------

int32_t CModel::RenderShadow (CObject *pObj, float *fLight)
{
	int16_t		i, *nearestLightP = lightManager.NearestSegLights () + gameData.objData.pConsole->info.nSegment * MAX_NEAREST_LIGHTS;

gameData.render.shadows.nLight = 0; 
for (i = 0; (gameData.render.shadows.nLight < gameOpts->render.shadows.nLights) && (*nearestLightP >= 0); i++, nearestLightP++) {
	gameData.render.shadows.pLight = lightManager.RenderLights (*nearestLightP);
	if (!gameData.render.shadows.pLight [0].render.bState)
		continue;
	gameData.render.shadows.nLight++;
	memcpy (&vrLightPos, gameData.render.shadows.pLight [0].render.vPosf + 1, sizeof (CFloatVector));
	if (!Draw (pObj, fLight))
		return 0;
	if (FAST_SHADOWS)
		RenderShadowQuad ();
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t CModel::Render (CObject *pObj, float *fLight, int32_t bCloaked)
{
	float	dt;

if (FAST_SHADOWS && (gameStates.render.nShadowPass == 3))
	return 1;
if (m_bCloaked != bCloaked) {
	m_bCloaked = bCloaked;
	m_nCloakPulse = 0;
	m_nCloakChangedTime = gameStates.app.nSDLTicks [0];
	}
dt = (float) (gameStates.app.nSDLTicks [0] - m_nCloakChangedTime) / 1000.0f;
if (bCloaked) {
	if (m_nCloakPulse) {
		//dt = 0.001f;
		m_nCloakChangedTime = gameStates.app.nSDLTicks [0];
		m_fAlpha += dt * m_nCloakPulse / 10.0f;
		if (m_nCloakPulse < 0) {
			if (m_fAlpha <= 0.01f)
				m_nCloakPulse = -m_nCloakPulse;
			}
		else if (m_fAlpha >= 0.1f)
			m_nCloakPulse = -m_nCloakPulse;
		}
	else {
		m_fAlpha = 1.0f - dt;
		if (m_fAlpha <= 0.01f) {
			m_nCloakPulse = 1;
			m_nCloakChangedTime = gameStates.app.nSDLTicks [0];
			}
		}
	}
else {
	m_fAlpha += dt;
	if (m_fAlpha > 1.0f)
		m_fAlpha = 1.0f;
	}
if (m_fAlpha < 0.01f)
	m_fAlpha = 0.01f;
return (!gameStates.render.nShadowMap && (gameStates.render.nShadowPass == 2)) 
		  ? RenderShadow (pObj, fLight) 
		  : Draw (pObj, fLight);
}

//------------------------------------------------------------------------------
//eof

