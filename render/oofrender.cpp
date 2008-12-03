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

static tOOF_vector vPos;
static tOOF_vector vrLightPos;
static tOOF_matrix mView;

//------------------------------------------------------------------------------

int OOF_FacingPoint (tOOF_vector *pv, tOOF_vector *pn, tOOF_vector *pp)
{
	tOOF_vector	v;

return OOF_VecDot (OOF_VecSub (&v, pp, pv), pn) > 0;
}

//------------------------------------------------------------------------------

int OOF_FacingViewer (tOOF_vector *pv, tOOF_vector *pn)
{
return OOF_FacingPoint (pv, pn, &vPos);
}

//------------------------------------------------------------------------------

int OOF_FacingLight (tOOF_vector *pv, tOOF_vector *pn)
{
return OOF_FacingPoint (pv, pn, &vrLightPos);
}

//------------------------------------------------------------------------------

tOOF_vector *OOF_CalcFacePerp (tOOF_subObject *pso, tOOF_face *pf)
{
	tOOF_vector		*pv = pso->pvVerts;
	tOOF_faceVert	*pfv = pf->pVerts;

#if 0
OOF_CalcFaceNormal (pso, pf);
#endif
return OOF_VecNormalize (OOF_VecPerp (&pf->vRotNormal, pv + pfv [0].nIndex, pv + pfv [1].nIndex, pv + pfv [2].nIndex));
}

//------------------------------------------------------------------------------

int OOF_LitFace (tOOF_subObject *pso, tOOF_face *pf)
{
//OOF_CalcFacePerp (pso, pf);
return pf->bFacingLight = 
#if 1
	OOF_FacingLight (&pf->vRotCenter, &pf->vRotNormal); 
#else
	OOF_FacingLight (pso->pvRotVerts + pf->pVerts->nIndex, &pf->vRotNormal); //OOF_CalcFacePerp (pso, pf)); 
#endif
}

//------------------------------------------------------------------------------

int OOF_FrontFace (tOOF_subObject *pso, tOOF_face *pf)
{
#if 0
return OOF_FacingViewer (&pf->vRotCenter, &pf->vRotNormal);
#else
return OOF_FacingViewer (pso->pvRotVerts + pf->pVerts->nIndex, &pf->vRotNormal);	//OOF_CalcFacePerp (pso, pf));
#endif
}

//------------------------------------------------------------------------------

int OOF_GetLitFaces (tOOF_subObject *pso)
{
	tOOF_face		*pf;
	int				i;

for (i = pso->faces.nFaces, pf = pso->faces.pFaces; i; i--, pf++) {
	pf->bFacingLight = OOF_LitFace (pso, pf);
#if 0
	if (bSWCulling)
		pf->bFacingViewer = OOF_FrontFace (pso, pf);
#endif
	}
return pso->faces.nFaces;
}

//------------------------------------------------------------------------------

int OOF_GetSilhouette (tOOF_subObject *pso)
{
	tOOF_edge		*pe;
	tOOF_vector		*pv;
	int				h, i, j;

OOF_GetLitFaces (pso);
pv = pso->pvRotVerts;
for (h = j = 0, i = pso->edges.nEdges, pe = pso->edges.pEdges; i; i--, pe++) {
	if (pe->pf [0] && pe->pf [1]) {
		if ((pe->bContour = (pe->pf [0]->bFacingLight != pe->pf [1]->bFacingLight)))
			h++;
		}
	else {
#if 0
		pe->bContour = 0;
#else
		pe->bContour = 1;
		h++;
#endif
		j++;
		}
	}
return pso->edges.nContourEdges = h;
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

int OOF_DrawShadowVolume (tOOFObject *po, tOOF_subObject *pso, int bCullFront)
{
	tOOF_edge		*pe;
	tOOF_vector		*pv, v [4];
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
	glColor4fv ((GLfloat *) (shadowColor + bCullFront));
#endif
OOF_SetCullAndStencil (bCullFront);
pv = pso->pvRotVerts;
#if DBG_SHADOWS
if (bShadowTest < 2)
	glBegin (GL_QUADS);
else
	glBegin (GL_LINES);
#endif
for (i = pso->edges.nContourEdges, pe = pso->edges.pEdges; i; pe++)
	if (pe->bContour) {
		i--;
#if DBG_SHADOWS
		if (bShadowTest < 2) {
			if (bShadowTest)
				glColor4fv ((GLfloat *) (shadowColor + bCullFront));
#endif
			j = (pe->pf [1] && pe->pf [1]->bFacingLight);
			if (pe->pf [j]->bReverse)
				j = !j;
			v [0] = pv [pe->v1 [j]];
			OOF_VecSub (v+3, v, &vrLightPos);
			v [1] = pv [pe->v0 [j]];
			OOF_VecSub (v+2, v+1, &vrLightPos);
#if NORM_INF
			OOF_VecScale (v+2, G3_INFINITY / OOF_VecMag (v+2));
			OOF_VecScale (v+3, G3_INFINITY / OOF_VecMag (v+3));
#else
			OOF_VecScale (v+2, G3_INFINITY);
			OOF_VecScale (v+3, G3_INFINITY);
#endif
			OOF_VecInc (v+2, v+1);
			OOF_VecInc (v+3, v);
#if !DBG
			glEnableClientState (GL_VERTEX_ARRAY);
			glVertexPointer (3, GL_FLOAT, 0, v);
			glDrawArrays (GL_QUADS, 0, 4);
			glDisableClientState (GL_VERTEX_ARRAY);
#else
			glVertex3fv ((GLfloat *) v);
			glVertex3fv ((GLfloat *) (v+1));
			glVertex3fv ((GLfloat *) (v+2));
			glVertex3fv ((GLfloat *) (v+3));
#endif
#if DBG_SHADOWS
			}
		else {
			glColor4f (1.0f, 1.0f, 1.0f, 1.0f);
			glVertex3fv ((GLfloat *) (pv + pe->v0 [0]));
			glVertex3fv ((GLfloat *) (pv + pe->v1 [0]));
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

int OOF_DrawShadowCaps (tOOFObject *po, tOOF_subObject *pso, int bCullFront)
{
	tOOF_face		*pf;
	tOOF_faceVert	*pfv;
	tOOF_vector		*pv, v0, v1;
	int				i, j;

#if DBG_SHADOWS
if (bZPass)
	return 1;
if (bShadowTest > 4)
	return 1;
glColor4fv ((GLfloat *) modelColor);
#endif
pv = pso->pvRotVerts;
OOF_SetCullAndStencil (bCullFront);
if (bCullFront) {
#if DBG_SHADOWS
	if (!bRearCap)
		return 1;
#endif
	for (i = pso->faces.nFaces, pf = pso->faces.pFaces; i; i--, pf++) {
		if (pf->bReverse)
			glFrontFace (GL_CCW);
		glBegin (GL_TRIANGLE_FAN);
		for (j = pf->nVerts, pfv = pf->pVerts; j; j--, pfv++) {
			v0 = pv [pfv->nIndex];
			OOF_VecSub (&v1, &v0, &vrLightPos);
#	if NORM_INF
			OOF_VecScale (&v1, G3_INFINITY / OOF_VecMag (&v1));
#	else
			OOF_VecScale (&v1, G3_INFINITY);
#	endif
			OOF_VecInc (&v1, &v0);
			glVertex3fv ((GLfloat *) &v1);
			}
		glEnd ();
		if (pf->bReverse)
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
	for (i = pso->faces.nFaces, pf = pso->faces.pFaces; i; i--, pf++) {
		if (pf->bReverse)
			glFrontFace (GL_CCW);
		glBegin (GL_TRIANGLE_FAN);
		for (j = pf->nVerts, pfv = pf->pVerts; j; j--, pfv++) {
			glVertex3fv ((GLfloat *) (pv + pfv->nIndex));
			}
		glEnd ();
		if (pf->bReverse)
			glFrontFace (GL_CW);
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int OOF_DrawShadow (tOOFObject *po, tOOF_subObject *pso)
{
return OOF_DrawShadowVolume (po, pso, 0) && 
		 OOF_DrawShadowVolume (po, pso, 1) && 
		 OOF_DrawShadowCaps (po, pso, 0) && 
		 OOF_DrawShadowCaps (po, pso, 1); 
}

//------------------------------------------------------------------------------

int OOF_DrawSubObject (tObject *objP, tOOFObject *po, tOOF_subObject *pso, float *fLight)
{
	tOOF_face		*pf;
	tOOF_faceVert	*pfv;
	tOOF_vector		*pv, *pvn, *phv;
	tFaceColor		*pvc, vc, sc = {{1,1,1,1}};
	CBitmap			*bmP;
	int				h, i, j;
	int				bBright = EGI_FLAG (bBrightObjects, 0, 1, 0);
	int				bDynLighting = gameStates.render.bApplyDynLight;
	float				fl, r, g, b, fAlpha = po->fAlpha;

#if DBG_SHADOWS
if (bShadowTest && (bShadowTest < 4))
	return 1;
#endif
pv = pso->pvRotVerts;
pvn = pso->pvNormals;
pvc = pso->pVertColors;
//memset (pvc, 0, pso->nVerts * sizeof (tFaceColor));
glEnable (GL_CULL_FACE);
OglCullFace (0);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
if (!bDynLighting) {
	sc = *AvgSgmColor (objP->info.nSegment, &objP->info.position.vPos);
	if (sc.index != gameStates.render.nFrameFlipFlop + 1)
		sc.color.red = sc.color.green = sc.color.blue = 1;
	}
for (i = pso->faces.nFaces, pf = pso->faces.pFaces; i; i--, pf++) {
	if (pf->bReverse)
		glFrontFace (GL_CCW);
	pfv = pf->pVerts;
#if 0
	if (!(gameStates.ogl.bUseTransform || OOF_FrontFace (pso, pf)))
		continue;
#endif
	if (pf->bTextured) {
#if DBG
		fl = OOF_VecDot (&pf->vNormal, &mView.f);
		fl = 0.75f + 0.25f * fl;
		fl = fl * *fLight;
#else
		fl = *fLight * (0.75f - 0.25f * OOF_VecDot (&pf->vNormal, &mView.f));
#endif
		if (fl > 1)
			fl = 1;
//		fl = 1.0f;
		bmP = po->textures.pBitmaps + pf->texProps.nTexId;
		if (bmP->Texture () && ((int) bmP->Texture ()->Handle () < 0))
			bmP->Texture ()->SetHandle (0);
		if (bmP->Bind (1, 0))
			return 0;
		bmP->Texture ()->Wrap (GL_REPEAT);
		if (pso->nFlags & (bDynLighting ? OOF_SOF_THRUSTER : (OOF_SOF_GLOW | OOF_SOF_THRUSTER))) {
			glColor4f (fl * pso->glowInfo.color.r, 
						  fl * pso->glowInfo.color.g, 
						  fl * pso->glowInfo.color.b, 
						  pso->pfAlpha [pfv->nIndex] * fAlpha);
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
						  pso->pfAlpha [pfv->nIndex] * fAlpha);
			}
		glBegin (GL_TRIANGLE_FAN);
		for (j = pf->nVerts; j; j--, pfv++) {
			phv = pv + (h = pfv->nIndex);
			if (bDynLighting) {
				if (pvc [h].index != gameStates.render.nFrameFlipFlop + 1)
					G3VertexColor ((fVector3 *) (pvn + h), (fVector3 *) phv, -1, pvc + h, NULL, 1, 0, 0);
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
				OglColor4sf (vc.color.red, vc.color.green, vc.color.blue, pso->pfAlpha [pfv->nIndex] * fAlpha);
				}
			glTexCoord2f (pfv->fu, pfv->fv);
			glVertex3fv ((GLfloat *) phv);
			//glVertex4f (phv->x, phv->y, phv->z, 0.5);
			}
		glEnd ();
#if DBG_SHADOWS
		if (pf->bFacingLight && (bShadowTest > 3)) {
				tOOF_vector	fv0;

			glLineWidth (3);
			glColor4f (1.0f, 0.0f, 0.5f, 1.0f);
			glBegin (GL_LINES);
			fv0 = pf->vRotCenter;
			glVertex3fv ((GLfloat *) &fv0);
			fv0.x += pf->vRotNormal.x * 1;
			fv0.y += pf->vRotNormal.y * 1;
			fv0.z += pf->vRotNormal.z * 1;
			glVertex3fv ((GLfloat *) &fv0);
			glEnd ();
			glColor4f (0.0f, 1.0f, 0.5f, 1.0f);
			glBegin (GL_LINES);
			fv0 = pf->vRotCenter;
			glVertex3fv ((GLfloat *) &fv0);
			fv0 = vrLightPos;
			glVertex3fv ((GLfloat *) &fv0);
			glEnd ();
			glLineWidth (1);
			}
#endif
		}
	else {
		fl = fLight [1];
		r = fl * (float) pf->texProps.color.r / 255.0f;
		g = fl * (float) pf->texProps.color.g / 255.0f;
		b = fl * (float) pf->texProps.color.b / 255.0f;
		glColor4f (r, g, b, pso->pfAlpha [pfv->nIndex] * fAlpha);
		glBegin (GL_TRIANGLE_FAN);
		for (j = pf->nVerts, pfv = pf->pVerts; j; j--, pfv++) {
			glVertex3fv ((GLfloat *) (pv + pfv->nIndex));
			}
		glEnd ();
		}
	if (pf->bReverse)
		glFrontFace (GL_CW);
	}
return 1;
}

//------------------------------------------------------------------------------

inline void OOF_RotVert (tOOF_vector *prv, tOOF_vector *pv, tOOF_vector *vo)
{
	tOOF_vector	v;

if (gameStates.ogl.bUseTransform)
	OOF_VecAdd (prv, pv, vo);
else {
	OOF_VecSub (&v, pv, &vPos);
	OOF_VecInc (&v, vo);
	OOF_VecRot (prv, &v, &mView);
	}
}

//------------------------------------------------------------------------------

void OOF_RotModelVerts (tOOF_subObject *pso, tOOF_vector vo)
{
	tOOF_vector	*pv, *prv;
	tOOF_face	*pf;
	int			i;

for (i = pso->nVerts, pv = pso->pvVerts, prv = pso->pvRotVerts; i; i--, pv++, prv++)
	OOF_RotVert (prv, pv, &vo);
for (i = pso->faces.nFaces, pf = pso->faces.pFaces; i; i--, pf++) {
#if 0
	OOF_CalcFacePerp (pso, pf);
	pf->vNormal = pf->vRotNormal;
#endif
#if OOF_TEST_CUBE
	if (i == 6)
		pf->bReverse = 1;
	else
#endif
	pf->bReverse = 0; //OOF_VecDot (&pf->vRotNormal, &pf->vNormal) < 0;
#if OOF_TEST_CUBE
	if (i == 6)
		pf->vNormal.z = (float) fabs (pf->vNormal.z);	//fix flaw in model data
#endif
	OOF_VecRot (&pf->vRotNormal, &pf->vNormal, &mView);
	OOF_VecNormalize (&pf->vRotNormal);
	OOF_RotVert (&pf->vRotCenter, &pf->vCenter, &vo);
	}
}

//------------------------------------------------------------------------------

int OOF_RenderSubObject (tObject *objP, tOOFObject *po, tOOF_subObject *pso, tOOF_vector vo, 
								 int nIndex, float *fLight)
{
	tOOF_subObject	*psc;
	int				i, j;

OOF_VecInc (&vo, &pso->vOffset);
OOF_RotModelVerts (pso, vo);
//if ((gameStates.render.nShadowPass != 2) && (bFacing != ((pso->nFlags & OOF_SOF_FACING) != 0)))
//	return 1;
#if 1
for (i = 0; i < pso->nChildren; i++) {
	psc = po->pSubObjects + (j = pso->children [i]);
	Assert (j >= 0 && j < po->nSubObjects);
	if (psc->nParent == nIndex)
		if (!OOF_RenderSubObject (objP, po, psc, vo, j, fLight))
			return 0;
	}
#endif
#if SHADOWS
if (gameStates.render.nShadowPass == 2)
	OOF_DrawShadow (po, pso);
else 
#endif
	OOF_DrawSubObject (objP, po, pso, fLight);
return 1;
}

//------------------------------------------------------------------------------

int OOF_RenderModel (tObject *objP, tOOFObject *po, float *fLight)
{
	tOOF_subObject	*pso;
	int				r = 1, i;
	tOOF_vector		vo = {0.0f,0.0f,0.0f};

G3StartInstanceMatrix(objP->info.position.vPos, objP->info.position.mOrient);
if (!gameStates.ogl.bUseTransform)
	OOF_MatVms2Oof (&mView, viewInfo.view[0]);
OOF_VecVms2Oof (&vPos, viewInfo.pos);
if (IsMultiGame && netGame.BrightPlayers)
	*fLight = 1.0f;
OglActiveTexture (GL_TEXTURE0, 0);
glEnable (GL_TEXTURE_2D);
for (i = 0, pso = po->pSubObjects; i < po->nSubObjects; i++, pso++)
	if (pso->nParent == -1) {
		if (!OOF_RenderSubObject (objP, po, pso, vo, i, fLight)) {
			r = 0;
			break;
			}
		}
G3DoneInstance ();
glDisable (GL_TEXTURE_2D);
return r;
}

//------------------------------------------------------------------------------

int OOF_RenderShadow (tObject *objP, tOOFObject *po, float *fLight)
{
	short			i, *pnl = gameData.render.lights.dynamic.nNearestSegLights + gameData.objs.consoleP->info.nSegment * MAX_NEAREST_LIGHTS;

gameData.render.shadows.nLight = 0; 
for (i = 0; (gameData.render.shadows.nLight < gameOpts->render.shadows.nLights) && (*pnl >= 0); i++, pnl++) {
	gameData.render.shadows.pLight = gameData.render.lights.dynamic.shader.lights + *pnl;
	if (!gameData.render.shadows.pLight->info.bState)
		continue;
	gameData.render.shadows.nLight++;
	memcpy (&vrLightPos, gameData.render.shadows.pLight->vPosf + 1, sizeof (tOOF_vector));
	if (!OOF_RenderModel (objP, po, fLight))
		return 0;
	if (FAST_SHADOWS)
		RenderShadowQuad (0);
	}
return 1;
}

//------------------------------------------------------------------------------

int OOF_Render (tObject *objP, tOOFObject *po, float *fLight, int bCloaked)
{
	float	dt;

#if SHADOWS
if (FAST_SHADOWS && (gameStates.render.nShadowPass == 3))
	return 1;
#endif
if (po->bCloaked != bCloaked) {
	po->bCloaked = bCloaked;
	po->nCloakPulse = 0;
	po->nCloakChangedTime = gameStates.app.nSDLTicks;
	}
dt = (float) (gameStates.app.nSDLTicks - po->nCloakChangedTime) / 1000.0f;
if (bCloaked) {
	if (po->nCloakPulse) {
		//dt = 0.001f;
		po->nCloakChangedTime = gameStates.app.nSDLTicks;
		po->fAlpha += dt * po->nCloakPulse / 10.0f;
		if (po->nCloakPulse < 0) {
			if (po->fAlpha <= 0.01f)
				po->nCloakPulse = -po->nCloakPulse;
			}
		else if (po->fAlpha >= 0.1f)
			po->nCloakPulse = -po->nCloakPulse;
		}
	else {
		po->fAlpha = 1.0f - dt;
		if (po->fAlpha <= 0.01f) {
			po->nCloakPulse = 1;
			po->nCloakChangedTime = gameStates.app.nSDLTicks;
			}
		}
	}
else {
	po->fAlpha += dt;
	if (po->fAlpha > 1.0f)
		po->fAlpha = 1.0f;
	}
if (po->fAlpha < 0.01f)
	po->fAlpha = 0.01f;
#if SHADOWS
return (!gameStates.render.bShadowMaps && (gameStates.render.nShadowPass == 2)) ? 
	OOF_RenderShadow (objP, po, fLight) :
	OOF_RenderModel (objP, po, fLight);
#else
return OOF_RenderModel (objP, po, fLight);
#endif
}

//------------------------------------------------------------------------------
//eof

