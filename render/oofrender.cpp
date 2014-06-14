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
extern int bShadowTest;
extern int bSingleStencil;

int bFrontCap = 1;
int bRearCap = 1;
int bFrontFaces = 1;
int bBackFaces = 1;
int bShadowVolume = 1;
int bWallShadows = 1;
int bSWCulling = 0;
#endif

int bZPass = 0;

static CFloatVector vPos;
static CFloatVector vrLightPos;
static CFloatMatrix mView;

//------------------------------------------------------------------------------

int OOF_FacingPoint (CFloatVector *modelVertP, CFloatVector *pn, CFloatVector *pp)
{
	CFloatVector	v = *pp - *modelVertP;

return CFloatVector::Dot (v, *pn) > 0;
}

//------------------------------------------------------------------------------

int OOF_FacingViewer (CFloatVector *modelVertP, CFloatVector *pn)
{
return OOF_FacingPoint (modelVertP, pn, &vPos);
}

//------------------------------------------------------------------------------

int OOF_FacingLight (CFloatVector *modelVertP, CFloatVector *pn)
{
return OOF_FacingPoint (modelVertP, pn, &vrLightPos);
}

//------------------------------------------------------------------------------

CFloatVector *OOF_CalcFacePerp (CSubModel *pso, CFace *faceP)
{
	CFloatVector*	modelVertP = pso->m_vertices.Buffer ();
	CFaceVert*		faceVertP = faceP->m_vertices;

CFloatVector::Normalize (CFloatVector::Perp (faceP->m_vRotNormal, modelVertP [faceVertP [0].m_nIndex], modelVertP [faceVertP [1].m_nIndex], modelVertP [faceVertP [2].m_nIndex]));
return &faceP->m_vRotNormal;
}

//------------------------------------------------------------------------------

int OOF_LitFace (CSubModel *pso, CFace *faceP)
{
//OOF_CalcFacePerp (pso, faceP);
return faceP->m_bFacingLight = 
#if 1
	OOF_FacingLight (&faceP->m_vRotCenter, &faceP->m_vRotNormal); 
#else
	OOF_FacingLight (pso->m_rotVerts + faceP->m_vertices->m_nIndex, &faceP->m_vRotNormal); //OOF_CalcFacePerp (pso, faceP)); 
#endif
}

//------------------------------------------------------------------------------

int OOF_FrontFace (CSubModel *pso, CFace *faceP)
{
#if 0
return OOF_FacingViewer (&faceP->m_vRotCenter, &faceP->m_vRotNormal);
#else
return OOF_FacingViewer (pso->m_rotVerts + faceP->m_vertices->m_nIndex, &faceP->m_vRotNormal);	//OOF_CalcFacePerp (pso, faceP));
#endif
}

//------------------------------------------------------------------------------

int OOF_GetLitFaces (CSubModel *pso)
{
	CFace		*faceP;
	int				i;

for (i = pso->m_faces.m_nFaces, faceP = pso->m_faces.m_list.Buffer (); i; i--, faceP++) {
	faceP->m_bFacingLight = OOF_LitFace (pso, faceP);
#if 0
	if (bSWCulling)
		faceP->m_bFacingViewer = OOF_FrontFace (pso, faceP);
#endif
	}
return pso->m_faces.m_nFaces;
}

//------------------------------------------------------------------------------

int OOF_GetSilhouette (CSubModel *pso)
{
	CEdge		*pe;
	int				h, i, j;

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

void SetCullAndStencil (int bCullFront, int bZPass = 0);

void OOF_SetCullAndStencil (int bCullFront)
{
SetCullAndStencil (bCullFront);
}

//------------------------------------------------------------------------------

int OOF_DrawShadowVolume (CModel *po, CSubModel *pso, int bCullFront)
{
if (bCullFront && ogl.m_features.bSeparateStencilOps.Available ())
	return 1;

	CEdge*					pe;
	CFloatVector*			modelVertP;
	int						nVerts, nEdges;

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
			int h = (pe->m_faces [1] && pe->m_faces [1]->m_bFacingLight);
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

int OOF_DrawShadowCaps (CModel *po, CSubModel *pso, int bCullFront)
{
if (bCullFront && ogl.m_features.bSeparateStencilOps.Available ())
	return 1;

	CFace*			faceP;
	CFaceVert*		faceVertP;
	CFloatVector*	modelVertP, v0, v1;
	int				nVerts, i, j, bReverse;

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
for (i = pso->m_faces.m_nFaces, faceP = pso->m_faces.m_list.Buffer (); i; i--, faceP++)
	nVerts += faceP->m_nVerts;

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
		for (i = pso->m_faces.m_nFaces, faceP = pso->m_faces.m_list.Buffer (); i; i--, faceP++) {
			if (faceP->m_bReverse == bReverse) {
				j = faceP->m_nVerts;
				if (!ogl.Buffers ().SizeVertices (j))
					return 1;
				nVerts = 0;
				for (faceVertP = faceP->m_vertices; j; j--, faceVertP++) {
					v0 = modelVertP [faceVertP->m_nIndex];
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
		for (i = pso->m_faces.m_nFaces, faceP = pso->m_faces.m_list.Buffer (); i; i--, faceP++) {
			if (faceP->m_bReverse == bReverse) {
				j = faceP->m_nVerts;
				if (!ogl.Buffers ().SizeVertices (j))
					return 1;
				nVerts = 0;
				for (faceVertP = faceP->m_vertices; j; j--, faceVertP++)
					ogl.VertexBuffer () [nVerts++] = modelVertP [faceVertP->m_nIndex];
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

int OOF_DrawShadow (CModel *po, CSubModel *pso)
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

int CSubModel::Draw (CObject *objP, CModel *po, float *fLight)
{
	CFace*			faceP;
	CFaceVert*		faceVertP;
	CFloatVector*	modelVertP, * modelNormalP, * phv;
	CFaceColor*		vertColorP, vertColor, segColor;
	CBitmap*			bmP = NULL;
	int				h, i, j, nModelVerts [2], nVerts, nFaceVerts;
	int				bBright = EGI_FLAG (bBrightObjects, 0, 1, 0), bTextured = -1, bReverse;
	int				bDynLighting = gameStates.render.bApplyDynLight;
	float				fl, r, g, b, fAlpha = po->m_fAlpha;
	// helper pointers into render buffers
	CFloatVector*	vertP;
	CFloatVector*	colorP;
	tTexCoord2f*	texCoordP;

segColor.Set (1.0f, 1.0f, 1.0f, 1.0f);
segColor.index = 1;
#if DBG_SHADOWS
if (bShadowTest && (bShadowTest < 4))
	return 1;
#endif
modelVertP = m_rotVerts.Buffer ();
modelNormalP = m_normals.Buffer ();
vertColorP = m_vertColors.Buffer ();
//memset (vertColorP, 0, m_nVerts * sizeof (CFaceColor));
ogl.SetFaceCulling (true);
OglCullFace (0);
ogl.SetBlendMode (OGL_BLEND_ALPHA);
ogl.SelectTMU (GL_TEXTURE0);
if (!bDynLighting) {
	segColor = *lightManager.AvgSgmColor (objP->info.nSegment, &objP->info.position.vPos, 0);
	if (segColor.index != gameStates.render.nFrameFlipFlop + 1)
		segColor.Red () = segColor.Green () = segColor.Blue () = 1;
	}

for (bReverse = 0; bReverse <= 1; bReverse++) {
	nModelVerts [0] = nModelVerts [1] = 0;
	for (i = m_faces.m_nFaces, faceP = m_faces.m_list.Buffer (); i; i--, faceP++) {
		if (faceP->m_bReverse == bReverse)
			nModelVerts [faceP->m_bTextured] += faceP->m_nVerts;
		}
	if (!(nVerts = nModelVerts [nModelVerts [1] > nModelVerts [0]]))
		continue;
	if (!ogl.Buffers ().SizeBuffers (nVerts))
		return 0;

	if (bReverse)
		glFrontFace (GL_CCW);

	vertP = ogl.VertexBuffer ().Buffer ();
	colorP = ogl.ColorBuffer ().Buffer ();
	texCoordP = ogl.TexCoordBuffer ().Buffer ();

	nVerts = 0;
	nFaceVerts = -1;
	for (i = m_faces.m_nFaces, faceP = m_faces.m_list.Buffer (); i; i--, faceP++) {
		if (faceP->m_bReverse != bReverse)
			continue;
		if (nFaceVerts != faceP->m_nVerts) {
			if (nVerts && (nFaceVerts != -1)) {
				ogl.FlushBuffers ((nFaceVerts == 3) ? GL_TRIANGLES : /*(nFaceVerts == 4) ? GL_QUADS :*/ GL_TRIANGLE_FAN, nVerts, 3, bTextured, bTextured);
				nVerts = 0;
				bTextured = -1;
				}
			nFaceVerts = faceP->m_nVerts;
			}
		faceVertP = faceP->m_vertices;
		if (faceP->m_bTextured) {
			//if (bTextured == 0) {
			//	ogl.FlushBuffers ((nFaceVerts == 3) ? GL_TRIANGLES : /*(nFaceVerts == 4) ? GL_QUADS :*/ GL_TRIANGLE_FAN, nVerts, 3, 0, 0);
			//	nFaceVerts = faceP->m_nVerts;
			//	nVerts = 0;
			//	}

			if (bmP != po->m_textures.m_bitmaps + faceP->m_texProps.nTexId) {
				if (nVerts) {
					ogl.FlushBuffers ((nFaceVerts == 3) ? GL_TRIANGLES : /*(nFaceVerts == 4) ? GL_QUADS :*/ GL_TRIANGLE_FAN, nVerts, 3, 1, 1);
					nVerts = 0;
					}
				bmP = po->m_textures.m_bitmaps + faceP->m_texProps.nTexId;
				if (bmP->Texture () && ((int) bmP->Texture ()->Handle () < 0))
					bmP->Texture ()->SetHandle (0);
				bmP->SetTranspType (0);
				ogl.SetTexturing (true);
				if (bmP->Bind (1))
					return 0;
				bmP->Texture ()->Wrap (GL_REPEAT);
				}
			bTextured = 1;

			fl = *fLight * (0.75f - 0.25f * (faceP->m_vNormal * mView.m.dir.f));
			if (fl > 1)
				fl = 1;
#if DBG
			if (m_nFlags & OOF_SOF_LAYER)
				BRP;
#endif
			if (m_nFlags & (bDynLighting ? OOF_SOF_THRUSTER : (OOF_SOF_GLOW | OOF_SOF_THRUSTER))) {
				colorP [nVerts].Red () = fl * m_glowInfo.m_color.Red ();
				colorP [nVerts].Green () = fl * m_glowInfo.m_color.Green ();
				colorP [nVerts].Blue () = fl * m_glowInfo.m_color.Blue ();
				colorP [nVerts].Alpha () = m_pfAlpha [faceVertP->m_nIndex] * fAlpha;
				}
			else if (!bDynLighting) {
				if (bBright)
					fl += (1 - fl) / 2;
				colorP [nVerts].Red () = segColor.Red () * fl;
				colorP [nVerts].Green () = segColor.Green () * fl;
				colorP [nVerts].Blue () = segColor.Blue () * fl;
				colorP [nVerts].Alpha () = m_pfAlpha [faceVertP->m_nIndex] * fAlpha;
				}
			for (j = faceP->m_nVerts; j; j--, faceVertP++) {
				phv = modelVertP + (h = faceVertP->m_nIndex);
				if (bDynLighting) {
					if (vertColorP [h].index != gameStates.render.nFrameFlipFlop + 1)
						GetVertexColor (-1, -1, -1, reinterpret_cast<CFloatVector3*> (modelNormalP + h), reinterpret_cast<CFloatVector3*> (phv), vertColorP + h, NULL, 1, 0, 0);
					
					vertColor.Red () = (float) sqrt (vertColorP [h].Red ());
					vertColor.Green () = (float) sqrt (vertColorP [h].Green ());
					vertColor.Blue () = (float) sqrt (vertColorP [h].Blue ());
					if (bBright) {
						vertColor.Red () += (1.0f - vertColor.Red ()) * 0.5f;
						vertColor.Green () += (1.0f - vertColor.Green ()) * 0.5f;
						vertColor.Blue () += (1.0f - vertColor.Blue ()) * 0.5f;
						}
					vertColor.Alpha () = m_pfAlpha [faceVertP->m_nIndex] * fAlpha;
					colorP [nVerts] = vertColor;
					}
				texCoordP [nVerts].v.u = faceVertP->m_fu;
				texCoordP [nVerts].v.v = faceVertP->m_fv;
				vertP [nVerts++] = *phv;
				}
#if DBG_SHADOWS
			if (faceP->m_bFacingLight && (bShadowTest > 3)) {
					CFloatVector	fv0;

				glLineWidth (3);
				glColor4f (1.0f, 0.0f, 0.5f, 1.0f);
				glBegin (GL_LINES);
				fv0 = faceP->m_vRotCenter;
				glVertex3fv (reinterpret_cast<GLfloat*> (&fv0));
				fv0 += faceP->m_vRotNormal;
				glVertex3fv (reinterpret_cast<GLfloat*> (&fv0));
				glEnd ();
				glColor4f (0.0f, 1.0f, 0.5f, 1.0f);
				glBegin (GL_LINES);
				fv0 = faceP->m_vRotCenter;
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
				bmP = NULL;
				bTextured = 0;
				}
			fl = fLight [1];
			r = fl * (float) faceP->m_texProps.color.Red () / 255.0f;
			g = fl * (float) faceP->m_texProps.color.Green () / 255.0f;
			b = fl * (float) faceP->m_texProps.color.Blue () / 255.0f;
			glColor4f (r, g, b, m_pfAlpha [faceVertP->m_nIndex] * fAlpha);
			for (j = faceP->m_nVerts; j; j--, faceVertP++) 
				vertP [nVerts++] = modelVertP [faceVertP->m_nIndex];
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
	CFace				*faceP;
	int				i;

for (i = m_nVerts, modelVertP = m_vertices.Buffer (), prv = m_rotVerts.Buffer (); i; i--, modelVertP++, prv++)
	TransformVertex (prv, modelVertP, &vo);
for (i = m_faces.m_nFaces, faceP = m_faces.m_list.Buffer (); i; i--, faceP++) {
#if OOF_TEST_CUBE
	if (i == 6)
		faceP->m_bReverse = 1;
	else
#endif
	faceP->m_bReverse = 0; //(faceP->m_vRotNormal * faceP->m_vNormal) < 0;
#if OOF_TEST_CUBE
	if (i == 6)
		faceP->m_vNormal.z = (float) fabs (faceP->m_vNormal.z);	//fix flaw in model data
#endif
	CFloatVector::Normalize (faceP->m_vRotNormal = mView * faceP->m_vNormal);
	TransformVertex (&faceP->m_vRotCenter, &faceP->m_vCenter, &vo);
	}
}

//------------------------------------------------------------------------------

int CSubModel::Render (CObject *objP, CModel *po, CFloatVector vo, int nIndex, float *fLight)
{
	CSubModel	*pso;
	int			i, j;

vo += m_vOffset;
Transform (vo);
//if ((gameStates.render.nShadowPass != 2) && (bFacing != ((m_nFlags & OOF_SOF_FACING) != 0)))
//	return 1;
#if 1
for (i = 0; i < m_nChildren; i++) {
	pso = po->m_subModels + (j = m_children [i]);
	Assert (j >= 0 && j < po->m_nSubModels);
	if (pso->m_nParent == m_nIndex)
		if (!pso->Render (objP, po, vo, j, fLight))
			return 0;
	}
#endif
if (gameStates.render.nShadowPass == 2)
	OOF_DrawShadow (po, po->m_subModels + nIndex);
else
	Draw (objP, po, fLight);
ogl.SetFaceCulling (true);
return 1;
}

//------------------------------------------------------------------------------

int CModel::Draw (CObject *objP, float *fLight)
{
	CSubModel		*pso;
	int				r = 1, i;
	CFloatVector	vo;

vo.SetZero ();
transformation.Begin (objP->info.position.vPos, objP->info.position.mOrient);
if (!ogl.m_states.bUseTransform)
	mView.Assign (transformation.m_info.view [0]);
vPos.Assign (transformation.m_info.pos);
if (IsMultiGame && netGame.m_info.BrightPlayers)
	*fLight = 1.0f;
ogl.SelectTMU (GL_TEXTURE0);
ogl.SetTexturing (true);
for (i = 0, pso = m_subModels.Buffer (); i < m_nSubModels; i++, pso++)
	if (pso->m_nParent == -1) {
		if (!pso->Render (objP, this, vo, i, fLight)) {
			r = 0;
			break;
			}
		}
transformation.End ();
ogl.SetTexturing (false);
return r;
}

//------------------------------------------------------------------------------

int CModel::RenderShadow (CObject *objP, float *fLight)
{
	short		i, *nearestLightP = lightManager.NearestSegLights () + gameData.objs.consoleP->info.nSegment * MAX_NEAREST_LIGHTS;

gameData.render.shadows.nLight = 0; 
for (i = 0; (gameData.render.shadows.nLight < gameOpts->render.shadows.nLights) && (*nearestLightP >= 0); i++, nearestLightP++) {
	gameData.render.shadows.lightP = lightManager.RenderLights (*nearestLightP);
	if (!gameData.render.shadows.lightP [0].render.bState)
		continue;
	gameData.render.shadows.nLight++;
	memcpy (&vrLightPos, gameData.render.shadows.lightP [0].render.vPosf + 1, sizeof (CFloatVector));
	if (!Draw (objP, fLight))
		return 0;
	if (FAST_SHADOWS)
		RenderShadowQuad ();
	}
return 1;
}

//------------------------------------------------------------------------------

int CModel::Render (CObject *objP, float *fLight, int bCloaked)
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
		  ? RenderShadow (objP, fLight) 
		  : Draw (objP, fLight);
}

//------------------------------------------------------------------------------
//eof

