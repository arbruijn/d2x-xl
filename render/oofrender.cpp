#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif
//#include  "oof.h
#include "inferno.h"
#include "args.h"
#include "u_mem.h"
#include "error.h"
#include "light.h"
#include "dynlight.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "network.h"
#include "render.h"
#include "strutil.h"

using namespace OOFModel;

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

void OOF_SetCullAndStencil (int bCullFront)
{
#if DBG_SHADOWS
if (bSingleStencil || bShadowTest) 
#endif
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
}

//------------------------------------------------------------------------------

int OOF_DrawShadowVolume (CModel *po, CSubModel *pso, int bCullFront)
{
	CEdge		*pe;
	CFloatVector	*pv, v [4];
	int				i, j;

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
pv = pso->m_rotVerts.Buffer ();
#if DBG_SHADOWS
if (bShadowTest < 2)
	glBegin (GL_QUADS);
else
	glBegin (GL_LINES);
#endif
for (i = pso->m_edges.m_nContourEdges, pe = pso->m_edges.m_list.Buffer (); i; pe++)
	if (pe->m_bContour) {
		i--;
#if DBG_SHADOWS
		if (bShadowTest < 2) {
			if (bShadowTest)
				glColor4fv (reinterpret_cast<GLfloat*> (shadowColor + bCullFront));
#endif
			j = (pe->m_faces [1] && pe->m_faces [1]->m_bFacingLight);
			if (pe->m_faces [j]->m_bReverse)
				j = !j;
			v [0] = pv [pe->m_v1 [j]];
			v [3] = v [0] - vrLightPos;
			v [1] = pv [pe->m_v0 [j]];
			v [2] = v [1] - vrLightPos;
#if NORM_INF
			v [2] *= G3_INFINITY / v [2].Mag ();
			v [3] *= G3_INFINITY / v [3].Mag ();
#else
			v [2] *= G3_INFINITY;
			v [3] *= G3_INFINITY;
#endif
			v [2] += v [1];
			v [3] += v [0];
#if !DBG
			glEnableClientState (GL_VERTEX_ARRAY);
			glVertexPointer (3, GL_FLOAT, 0, v);
			glDrawArrays (GL_QUADS, 0, 4);
			glDisableClientState (GL_VERTEX_ARRAY);
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
			glVertex3fv (reinterpret_cast<GLfloat*> (pv + pe->m_v0 [0]));
			glVertex3fv (reinterpret_cast<GLfloat*> (pv + pe->m_v1 [0]));
			}
#endif
		}
#if DBG_SHADOWS
glEnd ();
glEnable (GL_CULL_FACE);
#endif
return 1;
}

//------------------------------------------------------------------------------

int OOF_DrawShadowCaps (CModel *po, CSubModel *pso, int bCullFront)
{
	CFace		*pf;
	CFaceVert	*pfv;
	CFloatVector		*pv, v0, v1;
	int				i, j;

#if DBG_SHADOWS
if (bZPass)
	return 1;
if (bShadowTest > 4)
	return 1;
glColor4fv (reinterpret_cast<GLfloat*> (modelColor));
#endif
pv = pso->m_rotVerts.Buffer ();
OOF_SetCullAndStencil (bCullFront);
if (bCullFront) {
#if DBG_SHADOWS
	if (!bRearCap)
		return 1;
#endif
	for (i = pso->m_faces.m_nFaces, pf = pso->m_faces.m_list.Buffer (); i; i--, pf++) {
		if (pf->m_bReverse)
			glFrontFace (GL_CCW);
		glBegin (GL_TRIANGLE_FAN);
		for (j = pf->m_nVerts, pfv = pf->m_verts; j; j--, pfv++) {
			v0 = pv [pfv->m_nIndex];
			v1 = v0 - vrLightPos;
#	if NORM_INF
			v1 *= G3_INFINITY / v1.Mag ();
#	else
			v1 *= G3_INFINITY;
#	endif
			v1 += v0;
			glVertex3fv (reinterpret_cast<GLfloat*> (&v1));
			}
		glEnd ();
		if (pf->m_bReverse)
			glFrontFace (GL_CW);
		}
#if 1
	}
else {
#endif
#if DBG_SHADOWS
	if (!bFrontCap)
		return 1;
#endif
	for (i = pso->m_faces.m_nFaces, pf = pso->m_faces.m_list.Buffer (); i; i--, pf++) {
		if (pf->m_bReverse)
			glFrontFace (GL_CCW);
		glBegin (GL_TRIANGLE_FAN);
		for (j = pf->m_nVerts, pfv = pf->m_verts; j; j--, pfv++) {
			glVertex3fv (reinterpret_cast<GLfloat*> (pv + pfv->m_nIndex));
			}
		glEnd ();
		if (pf->m_bReverse)
			glFrontFace (GL_CW);
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int OOF_DrawShadow (CModel *po, CSubModel *pso)
{
return OOF_DrawShadowVolume (po, pso, 0) && 
		 OOF_DrawShadowVolume (po, pso, 1) && 
		 OOF_DrawShadowCaps (po, pso, 0) && 
		 OOF_DrawShadowCaps (po, pso, 1); 
}

//------------------------------------------------------------------------------

int CSubModel::Draw (CObject *objP, CModel *po, float *fLight)
{
	CFace*			pf;
	CFaceVert*		pfv;
	CFloatVector*	pv, * pvn, * phv;
	tFaceColor*		pvc, vc, sc = {{1,1,1,1}};
	CBitmap			*bmP;
	int				h, i, j;
	int				bBright = EGI_FLAG (bBrightObjects, 0, 1, 0);
	int				bDynLighting = gameStates.render.bApplyDynLight;
	float				fl, r, g, b, fAlpha = po->m_fAlpha;

#if DBG_SHADOWS
if (bShadowTest && (bShadowTest < 4))
	return 1;
#endif
pv = m_rotVerts.Buffer ();
pvn = m_normals.Buffer ();
pvc = m_vertColors.Buffer ();
//memset (pvc, 0, m_nVerts * sizeof (tFaceColor));
glEnable (GL_CULL_FACE);
OglCullFace (0);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
if (!bDynLighting) {
	sc = *AvgSgmColor (objP->info.nSegment, &objP->info.position.vPos);
	if (sc.index != gameStates.render.nFrameFlipFlop + 1)
		sc.color.red = sc.color.green = sc.color.blue = 1;
	}
for (i = m_faces.m_nFaces, pf = m_faces.m_list.Buffer (); i; i--, pf++) {
	if (pf->m_bReverse)
		glFrontFace (GL_CCW);
	pfv = pf->m_verts;
#if 0
	if (!(gameStates.ogl.bUseTransform || OOF_FrontFace (pso, pf)))
		continue;
#endif
	if (pf->m_bTextured) {
#if DBG
		fl = pf->m_vNormal * mView [FVEC];
		fl = 0.75f + 0.25f * fl;
		fl = fl * *fLight;
#else
		fl = *fLight * (0.75f - 0.25f * (pf->m_vNormal * mView.f);
#endif
		if (fl > 1)
			fl = 1;
//		fl = 1.0f;
		bmP = po->m_textures.m_bitmaps + pf->m_texProps.nTexId;
		if (bmP->Texture () && ((int) bmP->Texture ()->Handle () < 0))
			bmP->Texture ()->SetHandle (0);
		if (bmP->Bind (1, 0))
			return 0;
		bmP->Texture ()->Wrap (GL_REPEAT);
		if (m_nFlags & (bDynLighting ? OOF_SOF_THRUSTER : (OOF_SOF_GLOW | OOF_SOF_THRUSTER))) {
			glColor4f (fl * m_glowInfo.color.red, 
						  fl * m_glowInfo.color.green, 
						  fl * m_glowInfo.color.blue, 
						  m_pfAlpha [pfv->m_nIndex] * fAlpha);
			}
		else if (!bDynLighting) {
#if 0
			fl = (float) sqrt (fl);
#endif
			if (bBright)
#if 1
				fl += (1 - fl) / 2;
#else
				fl = (float) sqrt (fl);
#endif
			glColor4f (sc.color.red * fl, sc.color.green * fl, sc.color.blue * fl,
						  m_pfAlpha [pfv->m_nIndex] * fAlpha);
			}
		glBegin (GL_TRIANGLE_FAN);
		for (j = pf->m_nVerts; j; j--, pfv++) {
			phv = pv + (h = pfv->m_nIndex);
			if (bDynLighting) {
				if (pvc [h].index != gameStates.render.nFrameFlipFlop + 1)
					G3VertexColor (reinterpret_cast<CFloatVector3*> (pvn + h), reinterpret_cast<CFloatVector3*> (phv), -1, pvc + h, NULL, 1, 0, 0);
				vc.color.red = (float) sqrt (pvc [h].color.red);
				vc.color.green = (float) sqrt (pvc [h].color.green);
				vc.color.blue = (float) sqrt (pvc [h].color.blue);
				if (bBright) {
#if 1
					vc.color.red += (1 - vc.color.red) / 2;
					vc.color.green += (1 - vc.color.green) / 2;
					vc.color.blue += (1 - vc.color.blue) / 2;
#else
					vc.color.red = (float) sqrt (vc.color.red);
					vc.color.green = (float) sqrt (vc.color.green);
					vc.color.blue = (float) sqrt (vc.color.blue);
#endif
					}
				OglColor4sf (vc.color.red, vc.color.green, vc.color.blue, m_pfAlpha [pfv->m_nIndex] * fAlpha);
				}
			glTexCoord2f (pfv->m_fu, pfv->m_fv);
			glVertex3fv (reinterpret_cast<GLfloat*> (phv));
			//glVertex4f (phv->m_x, phv->m_y, phv->m_z, 0.5);
			}
		glEnd ();
#if DBG_SHADOWS
		if (pf->m_bFacingLight && (bShadowTest > 3)) {
				CFloatVector	fv0;

			glLineWidth (3);
			glColor4f (1.0f, 0.0f, 0.5f, 1.0f);
			glBegin (GL_LINES);
			fv0 = pf->m_vRotCenter;
			glVertex3fv (reinterpret_cast<GLfloat*> (&fv0));
			fv0.x += pf->m_vRotNormal.x * 1;
			fv0.y += pf->m_vRotNormal.y * 1;
			fv0.z += pf->m_vRotNormal.z * 1;
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
		fl = fLight [1];
		r = fl * (float) pf->m_texProps.color.r / 255.0f;
		g = fl * (float) pf->m_texProps.color.g / 255.0f;
		b = fl * (float) pf->m_texProps.color.b / 255.0f;
		glColor4f (r, g, b, m_pfAlpha [pfv->m_nIndex] * fAlpha);
		glBegin (GL_TRIANGLE_FAN);
		for (j = pf->m_nVerts, pfv = pf->m_verts; j; j--, pfv++) {
			glVertex3fv (reinterpret_cast<GLfloat*> (pv + pfv->m_nIndex));
			}
		glEnd ();
		}
	if (pf->m_bReverse)
		glFrontFace (GL_CW);
	}
return 1;
}

//------------------------------------------------------------------------------

inline void CSubModel::TransformVertex (CFloatVector *prv, CFloatVector *pv, CFloatVector *vo)
{
	CFloatVector	v;

if (gameStates.ogl.bUseTransform)
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
		if (!Render (objP, po, vo, j, fLight))
			return 0;
	}
#endif
#if SHADOWS
if (gameStates.render.nShadowPass == 2)
	OOF_DrawShadow (po, pso);
else 
#endif
	Draw (objP, po, fLight);
return 1;
}

//------------------------------------------------------------------------------

int CModel::Draw (CObject *objP, float *fLight)
{
	CSubModel		*pso;
	int				r = 1, i;
	CFloatVector	vo = {0.0f,0.0f,0.0f};

G3StartInstanceMatrix (objP->info.position.vPos, objP->info.position.mOrient);
if (!gameStates.ogl.bUseTransform)
	OOF_MatVms2Oof (&mView, viewInfo.view[0]);
vPos = viewInfo.pos;
if (IsMultiGame && netGame.BrightPlayers)
	*fLight = 1.0f;
OglActiveTexture (GL_TEXTURE0, 0);
glEnable (GL_TEXTURE_2D);
for (i = 0, pso = m_pSubModels; i < m_nSubModels; i++, pso++)
	if (pso->m_nParent == -1) {
		if (!pso->m_Render (objP, this, vo, i, fLight)) {
			r = 0;
			break;
			}
		}
G3DoneInstance ();
glDisable (GL_TEXTURE_2D);
return r;
}

//------------------------------------------------------------------------------

int CModel::RenderShadow (CObject *objP, float *fLight)
{
	short		i, *pnl = gameData.render.lights.dynamic.nearestSegLights + gameData.objs.consoleP->m_info.nSegment * MAX_NEAREST_LIGHTS;

gameData.render.shadows.nLight = 0; 
for (i = 0; (gameData.render.shadows.nLight < gameOpts->m_render.shadows.nLights) && (*pnl >= 0); i++, pnl++) {
	gameData.render.shadows.lights = gameData.render.lights.dynamic.shader.lights + *pnl;
	if (!gameData.render.shadows.lights [0].info.bState)
		continue;
	gameData.render.shadows.nLight++;
	memcpy (&vrLightPos, gameData.render.shadows.lights [0].vPosf + 1, sizeof (CFloatVector));
	if (!Draw (objP, this, fLight))
		return 0;
	if (FAST_SHADOWS)
		RenderShadowQuad (0);
	}
return 1;
}

//------------------------------------------------------------------------------

int CModel::Render (CObject *objP, float *fLight, int bCloaked)
{
	float	dt;

#if SHADOWS
if (FAST_SHADOWS && (gameStates.render.nShadowPass == 3))
	return 1;
#endif
if (m_bCloaked != bCloaked) {
	m_bCloaked = bCloaked;
	m_nCloakPulse = 0;
	m_nCloakChangedTime = gameStates.app.nSDLTicks;
	}
dt = (float) (gameStates.app.nSDLTicks - m_nCloakChangedTime) / 1000.0f;
if (bCloaked) {
	if (m_nCloakPulse) {
		//dt = 0.001f;
		m_nCloakChangedTime = gameStates.app.nSDLTicks;
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
			m_nCloakChangedTime = gameStates.app.nSDLTicks;
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
#if SHADOWS
return (!gameStates.render.bShadowMaps && (gameStates.render.nShadowPass == 2)) ? 
	RenderShadow (objP, fLight) :
	Draw (objP, fLight);
#else
return Draw (objP, fLight);
#endif
}

//------------------------------------------------------------------------------
//eof

