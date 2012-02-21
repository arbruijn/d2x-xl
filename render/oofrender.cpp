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

int OOF_FacingPoint (CFloatVector *pv, CFloatVector *pn, CFloatVector *pp)
{
	CFloatVector	v = *pp - *pv;

return CFloatVector::Dot (v, *pn) > 0;
}

//------------------------------------------------------------------------------

int OOF_FacingViewer (CFloatVector *pv, CFloatVector *pn)
{
return OOF_FacingPoint (pv, pn, &vPos);
}

//------------------------------------------------------------------------------

int OOF_FacingLight (CFloatVector *pv, CFloatVector *pn)
{
return OOF_FacingPoint (pv, pn, &vrLightPos);
}

//------------------------------------------------------------------------------

CFloatVector *OOF_CalcFacePerp (CSubModel *pso, CFace *pf)
{
	CFloatVector*	pv = pso->m_verts.Buffer ();
	CFaceVert*		pfv = pf->m_verts;

CFloatVector::Normalize (CFloatVector::Perp (pf->m_vRotNormal, pv [pfv [0].m_nIndex], pv [pfv [1].m_nIndex], pv [pfv [2].m_nIndex]));
return &pf->m_vRotNormal;
}

//------------------------------------------------------------------------------

int OOF_LitFace (CSubModel *pso, CFace *pf)
{
//OOF_CalcFacePerp (pso, pf);
return pf->m_bFacingLight = 
#if 1
	OOF_FacingLight (&pf->m_vRotCenter, &pf->m_vRotNormal); 
#else
	OOF_FacingLight (pso->m_rotVerts + pf->m_verts->m_nIndex, &pf->m_vRotNormal); //OOF_CalcFacePerp (pso, pf)); 
#endif
}

//------------------------------------------------------------------------------

int OOF_FrontFace (CSubModel *pso, CFace *pf)
{
#if 0
return OOF_FacingViewer (&pf->m_vRotCenter, &pf->m_vRotNormal);
#else
return OOF_FacingViewer (pso->m_rotVerts + pf->m_verts->m_nIndex, &pf->m_vRotNormal);	//OOF_CalcFacePerp (pso, pf));
#endif
}

//------------------------------------------------------------------------------

int OOF_GetLitFaces (CSubModel *pso)
{
	CFace		*pf;
	int				i;

for (i = pso->m_faces.m_nFaces, pf = pso->m_faces.m_list.Buffer (); i; i--, pf++) {
	pf->m_bFacingLight = OOF_LitFace (pso, pf);
#if 0
	if (bSWCulling)
		pf->m_bFacingViewer = OOF_FrontFace (pso, pf);
#endif
	}
return pso->m_faces.m_nFaces;
}

//------------------------------------------------------------------------------

int OOF_GetSilhouette (CSubModel *pso)
{
	CEdge		*pe;
	CFloatVector		*pv;
	int				h, i, j;

OOF_GetLitFaces (pso);
pv = pso->m_rotVerts.Buffer ();
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
	CFloatVector*			pv;
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
pv = pso->m_rotVerts.Buffer ();
for (nVerts = 0, pe = pso->m_edges.m_list.Buffer (); nEdges; pe++) {
	if (pe->m_bContour) {
		nEdges--;
#if DBG_SHADOWS
		if (bShadowTest < 2) {
#endif
			int h = (pe->m_faces [1] && pe->m_faces [1]->m_bFacingLight);
			if (pe->m_faces [h]->m_bReverse)
				h = !h;
			ogl.VertexBuffer () [nVerts] = pv [pe->m_v1 [h]];
			ogl.VertexBuffer () [nVerts + 1] = pv [pe->m_v0 [h]];
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
			ogl.VertexBuffer () [nVerts++] = pv [pe->m_v0 [0]];
			ogl.VertexBuffer () [nVerts++] = pv [pe->m_v1 [0]];
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

	CFace*			pf;
	CFaceVert*		pfv;
	CFloatVector*	pv, v0, v1;
	int				nVerts, i, j, bReverse;

#if DBG_SHADOWS
if (bZPass)
	return 1;
if (bShadowTest > 4)
	return 1;
glColor4fv (reinterpret_cast<GLfloat*> (modelColor));
#endif
pv = pso->m_rotVerts.Buffer ();
OOF_SetCullAndStencil (bCullFront);

nVerts = 0;
for (i = pso->m_faces.m_nFaces, pf = pso->m_faces.m_list.Buffer (); i; i--, pf++)
	nVerts += pf->m_nVerts;

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
		for (i = pso->m_faces.m_nFaces, pf = pso->m_faces.m_list.Buffer (); i; i--, pf++) {
			if (pf->m_bReverse == bReverse) {
				j = pf->m_nVerts;
				if (!ogl.Buffers ().SizeVertices (j))
					return 1;
				nVerts = 0;
				for (pfv = pf->m_verts; j; j--, pfv++) {
					v0 = pv [pfv->m_nIndex];
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
		for (i = pso->m_faces.m_nFaces, pf = pso->m_faces.m_list.Buffer (); i; i--, pf++) {
			if (pf->m_bReverse == bReverse) {
				j = pf->m_nVerts;
				if (!ogl.Buffers ().SizeVertices (j))
					return 1;
				nVerts = 0;
				for (pfv = pf->m_verts; j; j--, pfv++)
					ogl.VertexBuffer () [nVerts++] = pv [pfv->m_nIndex];
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
	CFace*			pf;
	CFaceVert*		pfv;
	CFloatVector*	pv, * pvn, * phv;
	CFaceColor*		vertColorP, vertColor, segColor;
	CBitmap*			bmP = NULL;
	int				h, i, j, nVerts [3];
	int				bBright = EGI_FLAG (bBrightObjects, 0, 1, 0), bTextured = -1, bReverse;
	int				bDynLighting = gameStates.render.bApplyDynLight;
	float				fl, r, g, b, fAlpha = po->m_fAlpha;
	// helper pointers into render buffers
	CFloatVector*	pvb;
	CFloatVector*	pcb;
	tTexCoord2f*	ptb;

segColor.Set (1.0f, 1.0f, 1.0f, 1.0f);
segColor.index = 1;
#if DBG_SHADOWS
if (bShadowTest && (bShadowTest < 4))
	return 1;
#endif
pv = m_rotVerts.Buffer ();
pvn = m_normals.Buffer ();
vertColorP = m_vertColors.Buffer ();
//memset (vertColorP, 0, m_nVerts * sizeof (CFaceColor));
ogl.SetFaceCulling (true);
OglCullFace (0);
ogl.SetBlendMode (OGL_BLEND_ALPHA);
if (!bDynLighting) {
	segColor = *lightManager.AvgSgmColor (objP->info.nSegment, &objP->info.position.vPos, 0);
	if (segColor.index != gameStates.render.nFrameFlipFlop + 1)
		segColor.Red () = segColor.Green () = segColor.Blue () = 1;
	}

for (bReverse = 0; bReverse <= 1; bReverse++) {
	nVerts [1] = nVerts [2] = 0;
	for (i = m_faces.m_nFaces, pf = m_faces.m_list.Buffer (); i; i--, pf++) {
		if (pf->m_bReverse == bReverse)
			nVerts [pf->m_bTextured] += pf->m_nVerts;
		}
	nVerts [2] = nVerts [nVerts [1] > nVerts [0]];
	if (!ogl.Buffers ().SizeBuffers (nVerts [2]))
		return 0;

	if (bReverse)
		glFrontFace (GL_CCW);

	nVerts [0] = 0;
	pvb = ogl.VertexBuffer ().Buffer ();
	pcb = ogl.ColorBuffer ().Buffer ();
	ptb = ogl.TexCoordBuffer ().Buffer ();

	for (i = m_faces.m_nFaces, pf = m_faces.m_list.Buffer (); i; i--, pf++) {
		if (pf->m_bReverse != bReverse)
			continue;
		pfv = pf->m_verts;
		if (pf->m_bTextured) {
			if (bTextured == 0) {
				ogl.FlushBuffers (GL_TRIANGLE_FAN, nVerts [0], 3, 0, 0);
				bTextured = 1;
				}

			if (bmP != po->m_textures.m_bitmaps + pf->m_texProps.nTexId) {
				bmP = po->m_textures.m_bitmaps + pf->m_texProps.nTexId;
				if (bmP->Texture () && ((int) bmP->Texture ()->Handle () < 0))
					bmP->Texture ()->SetHandle (0);
				bmP->SetTranspType (0);
				if (bmP->Bind (1))
					return 0;
				bmP->Texture ()->Wrap (GL_REPEAT);
				ogl.SetTexturing (true);
				bTextured = 1;
				}

			fl = *fLight * (0.75f - 0.25f * (pf->m_vNormal * mView.m.dir.f));
			if (fl > 1)
				fl = 1;
			if (m_nFlags & (bDynLighting ? OOF_SOF_THRUSTER : (OOF_SOF_GLOW | OOF_SOF_THRUSTER))) {
				pcb->Red () = fl * m_glowInfo.m_color.Red ();
				pcb->Green () = fl * m_glowInfo.m_color.Green ();
				pcb->Blue () = fl * m_glowInfo.m_color.Blue ();
				pcb->Alpha () = m_pfAlpha [pfv->m_nIndex] * fAlpha;
				pcb++;
				}
			else if (!bDynLighting) {
				if (bBright)
					fl += (1 - fl) / 2;
				pcb->Red () = segColor.Red () * fl;
				pcb->Green () = segColor.Green () * fl;
				pcb->Blue () = segColor.Blue () * fl;
				pcb->Alpha () = m_pfAlpha [pfv->m_nIndex] * fAlpha;
				pcb++;
				}
			for (j = pf->m_nVerts; j; j--, pfv++) {
				phv = pv + (h = pfv->m_nIndex);
				if (bDynLighting) {
					if (vertColorP [h].index != gameStates.render.nFrameFlipFlop + 1)
						G3VertexColor (-1, -1, -1, reinterpret_cast<CFloatVector3*> (pvn + h), reinterpret_cast<CFloatVector3*> (phv), vertColorP + h, NULL, 1, 0, 0);
					
					vertColor.Red () = (float) sqrt (vertColorP [h].Red ());
					vertColor.Green () = (float) sqrt (vertColorP [h].Green ());
					vertColor.Blue () = (float) sqrt (vertColorP [h].Blue ());
					if (bBright) {
						vertColor.Red () += (1.0f - vertColor.Red ()) * 0.5f;
						vertColor.Green () += (1.0f - vertColor.Green ()) * 0.5f;
						vertColor.Blue () += (1.0f - vertColor.Blue ()) * 0.5f;
						}
					vertColor.Alpha () = m_pfAlpha [pfv->m_nIndex] * fAlpha;
					*pcb++ = vertColor;
					}
				ptb->v.u = pfv->m_fu;
				ptb->v.v = pfv->m_fv;
				ptb++;
				*pvb++ = *phv;
			}
#if DBG_SHADOWS
			if (pf->m_bFacingLight && (bShadowTest > 3)) {
					CFloatVector	fv0;

				glLineWidth (3);
				glColor4f (1.0f, 0.0f, 0.5f, 1.0f);
				glBegin (GL_LINES);
				fv0 = pf->m_vRotCenter;
				glVertex3fv (reinterpret_cast<GLfloat*> (&fv0));
				fv0 += pf->m_vRotNormal;
				glVertex3fv (reinterpret_cast<GLfloat*> (&fv0));
				glEnd ();
				glColor4f (0.0f, 1.0f, 0.5f, 1.0f);
				glBegin (GL_LINES);
				fv0 = pf->m_vRotCenter;
				glVertex3fv (reinterpret_cast<GLfloat*> (&fv0));
				fv0 = vrLightPos;
				glVertex3fv (reinterpret_cast<GLfloat*> (&fv0));
				glEnd ();
				glLineWidth (1);
				}
#endif
			}
		else {
			if (bTextured == 1) {
				ogl.FlushBuffers (GL_TRIANGLE_FAN, nVerts [1], 3, 1, 1);
				bTextured = 0;
				ogl.SetTexturing (false);
				bmP = NULL;
				}
			fl = fLight [1];
			r = fl * (float) pf->m_texProps.color.Red () / 255.0f;
			g = fl * (float) pf->m_texProps.color.Green () / 255.0f;
			b = fl * (float) pf->m_texProps.color.Blue () / 255.0f;
			glColor4f (r, g, b, m_pfAlpha [pfv->m_nIndex] * fAlpha);
			*pvb++ = pv [pfv->m_nIndex];
			}
		}
	if (bTextured != -1)
		ogl.FlushBuffers (GL_TRIANGLE_FAN, nVerts [bTextured], 3, bTextured, bTextured);
	}
glFrontFace (GL_CW);
return 1;
}

//------------------------------------------------------------------------------

inline void CSubModel::TransformVertex (CFloatVector *prv, CFloatVector *pv, CFloatVector *vo)
{
	CFloatVector	v;

if (ogl.m_states.bUseTransform)
	*prv = *pv + *vo;
else {
	v = *pv - vPos;
	v += *vo;
	*prv = mView * v;
	}
}

//------------------------------------------------------------------------------

void CSubModel::Transform (CFloatVector vo)
{
	CFloatVector	*pv, *prv;
	CFace				*pf;
	int				i;

for (i = m_nVerts, pv = m_verts.Buffer (), prv = m_rotVerts.Buffer (); i; i--, pv++, prv++)
	TransformVertex (prv, pv, &vo);
for (i = m_faces.m_nFaces, pf = m_faces.m_list.Buffer (); i; i--, pf++) {
#if OOF_TEST_CUBE
	if (i == 6)
		pf->m_bReverse = 1;
	else
#endif
	pf->m_bReverse = 0; //(pf->m_vRotNormal * pf->m_vNormal) < 0;
#if OOF_TEST_CUBE
	if (i == 6)
		pf->m_vNormal.z = (float) fabs (pf->m_vNormal.z);	//fix flaw in model data
#endif
	CFloatVector::Normalize (pf->m_vRotNormal = mView * pf->m_vNormal);
	TransformVertex (&pf->m_vRotCenter, &pf->m_vCenter, &vo);
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

