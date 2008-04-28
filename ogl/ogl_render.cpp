/* $Id: ogl.c, v 1.14 204/05/11 23:15:55 btb Exp $ */
/*
 *
 * Graphics support functions for OpenGL.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef _WIN32
#	include <windows.h>
#	include <stddef.h>
#	include <io.h>
#endif
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef __macosx__
# include <stdlib.h>
# include <SDL/SDL.h>
#else
# include <malloc.h>
# include <SDL.h>
#endif

#include "inferno.h"
#include "error.h"
#include "maths.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_defs.h"
#include "ogl_color.h"
#include "ogl_shader.h"
#include "ogl_hudstuff.h"
#include "ogl_render.h"
#include "ogl_tmu.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "texmerge.h"
#include "transprender.h"

//------------------------------------------------------------------------------

#define OGL_CLEANUP			1
#define USE_VERTNORMS		1
#define G3_DRAW_ARRAYS		0
#define G3_MULTI_TEXTURE	0
#define G3_BUFFER_FACES		0

//------------------------------------------------------------------------------

GLhandleARB	lmProg = (GLhandleARB) 0;
GLhandleARB	tmProg = (GLhandleARB) 0;

tTexPolyMultiDrawer	*fpDrawTexPolyMulti = NULL;

//------------------------------------------------------------------------------

inline void SetTexCoord (tUVL *uvlList, int nOrient, int bMulti, tTexCoord2f *texCoord, int bMask)
{
	float u1, v1;

if (nOrient == 1) {
	u1 = 1.0f - f2fl (uvlList->v);
	v1 = f2fl (uvlList->u);
	}
else if (nOrient == 2) {
	u1 = 1.0f - f2fl (uvlList->u);
	v1 = 1.0f - f2fl (uvlList->v);
	}
else if (nOrient == 3) {
	u1 = f2fl (uvlList->v);
	v1 = 1.0f - f2fl (uvlList->u);
	}
else {
	u1 = f2fl (uvlList->u);
	v1 = f2fl (uvlList->v);
	}
if (texCoord) {
	texCoord->v.u = u1;
	texCoord->v.v = v1;
	}
else {
#if OGL_MULTI_TEXTURING
	if (bMulti) {
		glMultiTexCoord2f (GL_TEXTURE1, u1, v1);
		if (bMask)
			glMultiTexCoord2f (GL_TEXTURE2, u1, v1);
	}
else
#endif
		glTexCoord2f (u1, v1);
	}
}

//------------------------------------------------------------------------------

inline int G3BindTex (grsBitmap *bmP, GLint nTexId, GLhandleARB lmProg, char *pszTexId, 
						    pInitTMU initTMU, int bShaderVar, int bVertexArrays)
{
if (bmP || (nTexId >= 0)) {
	initTMU (bVertexArrays);
	if (nTexId >= 0)
		OGL_BINDTEX (nTexId);
	else {
		if (OglBindBmTex (bmP, 1, 3))
			return 1;
		OglTexWrap (bmP->glTexture, GL_REPEAT);
		}
	if (bShaderVar)
		glUniform1i (glGetUniformLocation (lmProg, pszTexId), 0);
	}
return 0;
}

//------------------------------------------------------------------------------

int G3DrawLine (g3sPoint *p0, g3sPoint *p1)
{
glDisable (GL_TEXTURE_2D);
OglGrsColor (&grdCurCanv->cvColor);
glBegin (GL_LINES);
OglVertex3x (p0->p3_vec.p.x, p0->p3_vec.p.y, p0->p3_vec.p.z);
OglVertex3x (p1->p3_vec.p.x, p1->p3_vec.p.y, p1->p3_vec.p.z);
if (grdCurCanv->cvColor.rgb)
	glDisable (GL_BLEND);
glEnd ();
return 1;
}

//------------------------------------------------------------------------------

void OglDrawEllipse (int nSides, int nType, float xsc, float xo, float ysc, float yo, tSinCosf *sinCosP)
{
	int		i;
	double	ang;

glBegin (nType);
if (sinCosP) {
	for (i = 0; i < nSides; i++, sinCosP++)
		glVertex2f (sinCosP->fCos * xsc + xo, sinCosP->fSin * ysc + yo);
	}
else {
	for (i = 0; i < nSides; i++) {
		ang = 2.0 * Pi * i / nSides;
		glVertex2f ((float) cos (ang) * xsc + xo, (float) sin (ang) * ysc + yo);
		}
	}
glEnd ();
}

//------------------------------------------------------------------------------

void OglDrawCircle (int nSides, int nType)
{
	int		i;
	double	ang;

glBegin (nType);
for (i = 0; i < nSides; i++) {
	ang = 2.0 * Pi * i / nSides;
	glVertex2d (cos (ang), sin (ang));
	}
glEnd ();
}

//------------------------------------------------------------------------------

int G3DrawSphere (g3sPoint *pnt, fix rad, int bBigSphere)
{
	double r;

glDisable (GL_TEXTURE_2D);
OglGrsColor (&grdCurCanv->cvColor);
glPushMatrix ();
glTranslatef (f2fl (pnt->p3_vec.p.x), f2fl (pnt->p3_vec.p.y), f2fl (pnt->p3_vec.p.z));
r = f2fl (rad);
glScaled (r, r, r);
if (bBigSphere)
	if (hBigSphere)
		glCallList (hBigSphere);
	else
		hBigSphere = CircleListInit (20, GL_POLYGON, GL_COMPILE_AND_EXECUTE);
else
	if (hSmallSphere)
		glCallList (hSmallSphere);
	else
		hSmallSphere = CircleListInit (12, GL_POLYGON, GL_COMPILE_AND_EXECUTE);
glPopMatrix ();
if (grdCurCanv->cvColor.rgb)
	glDisable (GL_BLEND);
return 0;
}

//------------------------------------------------------------------------------

int GrUCircle (fix xc1, fix yc1, fix r1)
{//dunno if this really works, radar doesn't seem to.. hm..
glDisable (GL_TEXTURE_2D);
//	glPointSize (f2fl (rad);
OglGrsColor (&grdCurCanv->cvColor);
glPushMatrix ();
glTranslatef (
			(f2fl (xc1) + grdCurCanv->cvBitmap.bmProps.x) / (float) gameStates.ogl.nLastW, 
		1.0f - (f2fl (yc1) + grdCurCanv->cvBitmap.bmProps.y) / (float) gameStates.ogl.nLastH, 0);
glScalef (f2fl (r1), f2fl (r1), f2fl (r1));
if (r1<=i2f (5)){
	if (!circleh5) 
		circleh5 = CircleListInit (5, GL_LINE_LOOP, GL_COMPILE_AND_EXECUTE);
	else 
		glCallList (circleh5);
	}
else{
	if (!circleh10) 
		circleh10 = CircleListInit (10, GL_LINE_LOOP, GL_COMPILE_AND_EXECUTE);
	else 
		glCallList (circleh10);
}
glPopMatrix ();
if (grdCurCanv->cvColor.rgb)
	glDisable (GL_BLEND);
return 0;
}

//------------------------------------------------------------------------------

int G3DrawWhitePoly (int nVertices, g3sPoint **pointList)
{
#if 1
	int i;

r_polyc++;
glDisable (GL_TEXTURE_2D);
glDisable (GL_BLEND);
glColor4d (1.0, 1.0, 1.0, 1.0);
glBegin (GL_TRIANGLE_FAN);
for (i = 0; i < nVertices; i++, pointList++)
	OglVertex3f (*pointList);
glEnd ();
#endif
return 0;
}

//------------------------------------------------------------------------------

int G3DrawPoly (int nVertices, g3sPoint **pointList)
{
	int i;

if (gameStates.render.nShadowBlurPass == 1) {
	G3DrawWhitePoly (nVertices, pointList);
	return 0;
	}
r_polyc++;
glDisable (GL_TEXTURE_2D);
OglGrsColor (&grdCurCanv->cvColor);
glBegin (GL_TRIANGLE_FAN);
for (i = 0; i < nVertices; i++, pointList++) {
//	glVertex3f (f2fl (pointList [c]->p3_vec.p.x), f2fl (pointList [c]->p3_vec.p.y), f2fl (pointList [c]->p3_vec.p.z);
	OglVertex3f (*pointList);
	}
#if 1
if (grdCurCanv->cvColor.rgb || (gameStates.render.grAlpha < GR_ACTUAL_FADE_LEVELS))
	glDisable (GL_BLEND);
#endif
glEnd ();
return 0;
}

//------------------------------------------------------------------------------

int G3DrawPolyAlpha (int nVertices, g3sPoint **pointList, tRgbaColorf *color, char bDepthMask, short nSegment)
{
	int		i;
	GLint		depthFunc; 

if (gameStates.render.nShadowBlurPass == 1) {
	G3DrawWhitePoly (nVertices, pointList);
	return 0;
	}
if (color->alpha < 0)
	color->alpha = (float) gameStates.render.grAlpha / (float) GR_ACTUAL_FADE_LEVELS;
#if 1
if (gameOpts->render.bDepthSort > 0) {
	fVector	vertices [8];

	for (i = 0; i < nVertices; i++)
		vertices [i] = gameData.render.pVerts [pointList [i]->p3_index];
	RIAddPoly (NULL, NULL, vertices, nVertices, NULL, color, NULL, 1, bDepthMask, GL_TRIANGLE_FAN, GL_REPEAT, 0, nSegment);
	}
else 
#endif
	{
	r_polyc++;
	glGetIntegerv (GL_DEPTH_FUNC, &depthFunc);
#if OGL_QUERY
	if (gameStates.render.bQueryOcclusion)
		glDepthFunc (GL_LESS);
	else
#endif
	if (!bDepthMask)
		glDepthMask (0);
	glDepthFunc (GL_LEQUAL);
	glEnable (GL_BLEND);
	glDisable (GL_TEXTURE_2D);
	glColor4fv ((GLfloat *) color);
	glBegin (GL_TRIANGLE_FAN);
	for (i = 0; i < nVertices; i++)
		OglVertex3f (*pointList++);
	glEnd ();
	glDepthFunc (depthFunc);
	if (!bDepthMask)
		glDepthMask (1);
	}
return 0;
}

//------------------------------------------------------------------------------

void gr_upoly_tmap (int nverts, int *vert )
{
#if TRACE
con_printf (CONDBG, "gr_upoly_tmap: unhandled\n");//should never get called
#endif
}

//------------------------------------------------------------------------------

void DrawTexPolyFlat (grsBitmap *bmP, int nVertices, g3sPoint **vertlist)
{
#if TRACE
con_printf (CONDBG, "DrawTexPolyFlat: unhandled\n");//should never get called
#endif
}

//------------------------------------------------------------------------------

int G3DrawTexPolyFlat (
	int			nVertices, 
	g3sPoint		**pointList, 
	tUVL			*uvlList, 
	tUVL			*uvlLMap, 
	grsBitmap	*bmBot, 
	grsBitmap	*bmTop, 
	tLightMap	*lightMap, 
	vmsVector	*pvNormal,
	int			orient, 
	int			bBlend,
	short			nSegment)
{
	int			i;
	g3sPoint		**ppl;

if (FAST_SHADOWS) {
	if (bBlend)
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	else
		glDisable (GL_BLEND);
	}
else {
	if (gameStates.render.nShadowPass == 3) {
		glEnable (GL_BLEND);
		glBlendFunc (GL_ONE, GL_ONE);
		}
	}
OglActiveTexture (GL_TEXTURE0, 0);
glDisable (GL_TEXTURE_2D);
glColor4d (0, 0, 0, GrAlpha ());
glBegin (GL_TRIANGLE_FAN);
for (i = 0, ppl = pointList; i < nVertices; i++, ppl++)
	OglVertex3f (*ppl);
glEnd ();
return 0;
}

//------------------------------------------------------------------------------

#define	G3VERTPOS(_dest,_src) \
			if ((_src)->p3_index < 0) \
				VmVecFixToFloat (&(_dest), &((_src)->p3_vec)); \
			else \
				_dest = gameData.render.pVerts [(_src)->p3_index]; 

//------------------------------------------------------------------------------

int G3DrawTexPolyMulti (
	int			nVertices, 
	g3sPoint		**pointList, 
	tUVL			*uvlList, 
	tUVL			*uvlLMap, 
	grsBitmap	*bmBot, 
	grsBitmap	*bmTop, 
	tLightMap	*lightMap, 
	vmsVector	*pvNormal,
	int			orient, 
	int			bBlend,
	short			nSegment)
{
	int			i, nShader, nFrame;
	int			bShaderMerge = 0,
					bSuperTransp = 0;
	int			bLight = 1, 
					bDynLight = gameStates.render.bApplyDynLight && (gameStates.app.bEndLevelSequence < EL_OUTSIDE), 
					bDepthSort,
					bResetColor = 0,
					bOverlay = 0;
	tFaceColor	*pc;
	grsBitmap	*bmP = NULL, *bmMask = NULL;
	g3sPoint		*pl, **ppl;
#if USE_VERTNORMS
	fVector		vNormal, vVertPos;
#endif
#if G3_DRAW_ARRAYS
	int			bVertexArrays = gameData.render.pVerts != NULL;
#else
	int			bVertexArrays = 0;
#endif

if (gameStates.render.nShadowBlurPass == 1) {
	G3DrawWhitePoly (nVertices, pointList);
	return 0;
	}
if (!bmBot)
	return 1;
r_tpolyc++;
if (FAST_SHADOWS) {
	if (!bBlend)
		glDisable (GL_BLEND);
#if 0
	else
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
	}
else {
	if (gameStates.render.nShadowPass == 1)
		bLight = !bDynLight;
	else if (gameStates.render.nShadowPass == 3) {
		glEnable (GL_BLEND);
		glBlendFunc (GL_ONE, GL_ONE);
		}
	}
glDepthFunc (GL_LEQUAL);
bmBot = BmOverride (bmBot, -1);
bDepthSort = (!bmTop && (gameOpts->render.bDepthSort > 0) && 
				  ((gameStates.render.grAlpha < GR_ACTUAL_FADE_LEVELS) || 
				   (bmBot->bmProps.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SEE_THRU | BM_FLAG_TGA)) == (BM_FLAG_TRANSPARENT | BM_FLAG_TGA)));
if ((bmTop = BmOverride (bmTop, -1)) && BM_FRAMES (bmTop)) {
	nFrame = (int) (BM_CURFRAME (bmTop) - BM_FRAMES (bmTop));
	bmP = bmTop;
	bmTop = BM_CURFRAME (bmTop);
	}
else
	nFrame = -1;
if (bmTop) {
	if (nFrame < 0)
      bSuperTransp = (bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
	else
		bSuperTransp = (bmP->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
	bShaderMerge = bSuperTransp && gameStates.ogl.bGlTexMerge;
	bOverlay = !bShaderMerge;
	}
else
	bOverlay = -1;
#if G3_DRAW_ARRAYS
retry:
#endif
if (bShaderMerge) {
	bmMask = gameStates.render.textures.bHaveMaskShader ? BM_MASK (bmTop) : NULL;
	nShader = bSuperTransp ? bmMask ? 2 : 1 : 0;
	glUseProgramObject (tmProg = tmShaderProgs [nShader]);
	INIT_TMU (InitTMU0, GL_TEXTURE0, bmBot, lightMapData.buffers, bVertexArrays, 0);
	glUniform1i (glGetUniformLocation (tmProg, "btmTex"), 0);
	INIT_TMU (InitTMU1, GL_TEXTURE1, bmTop, lightMapData.buffers, bVertexArrays, 0);
	glUniform1i (glGetUniformLocation (tmProg, "topTex"), 1);
	if (bmMask) {
#ifdef _DEBUG
		InitTMU2 (bVertexArrays);
		G3_BIND (GL_TEXTURE2, bmMask, lightMapData.buffers, bVertexArrays);
#else
		INIT_TMU (InitTMU2, GL_TEXTURE2, bmMask, bVertexArrays, 0);
#endif
		glUniform1i (glGetUniformLocation (tmProg, "maskTex"), 2);
		}
	glUniform1f (glGetUniformLocation (tmProg, "grAlpha"), 
					 gameStates.render.grAlpha / (float) GR_ACTUAL_FADE_LEVELS);
	}
else if (!bDepthSort) {
	if (bmBot == gameData.endLevel.satellite.bmP) {
		glActiveTexture (GL_TEXTURE0);
		glEnable (GL_TEXTURE_2D);
		}
	else
		InitTMU0 (bVertexArrays);
	if (OglBindBmTex (bmBot, 1, 3))
		return 1;
	bmBot = BmCurFrame (bmBot, -1);
	if (bmBot == bmpDeadzone)
		OglTexWrap (bmBot->glTexture, GL_CLAMP);
	else
		OglTexWrap (bmBot->glTexture, GL_REPEAT);
	}

if (!bDepthSort) {
	if (SHOW_DYN_LIGHT) {
#if USE_VERTNORMS
		if (pvNormal) {
			VmVecFixToFloat (&vNormal, pvNormal);
			G3RotatePoint (&vNormal, &vNormal, 0);
			}
	else
			G3CalcNormal (pointList, &vNormal);
#else
			G3Normal (pointList, pvNormal);
#endif
		}
	if (gameStates.render.bFullBright) {
		glColor3f (1,1,1);
		bLight = 0;
		}
	else if (!gameStates.render.nRenderPass)
		bLight = 0;
	else if (!bLight)
		glColor3i (0,0,0);
	if (!bLight)
		bDynLight = 0;
	gameStates.ogl.bDynObjLight = bDynLight;
	}

gameStates.ogl.fAlpha = gameStates.render.grAlpha / (float) GR_ACTUAL_FADE_LEVELS;
if (bVertexArrays || bDepthSort) {
		fVector		vertices [8];
		tFaceColor	vertColors [8];
		tTexCoord2f	texCoord [2][8];
		int			vertIndex [8];
		//int			colorIndex [8];

	for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
		pl = *ppl;
		vertIndex [i] = pl->p3_index;
		//colorIndex [i] = i;
		if (pl->p3_index < 0)
			VmVecFixToFloat (vertices + i, &pl->p3_vec);
		else
			vertices [i] = gameData.render.pVerts [pl->p3_index];
		texCoord [0][i].v.u = f2fl (uvlList [i].u);
		texCoord [0][i].v.v = f2fl (uvlList [i].v);
		SetTexCoord (uvlList + i, orient, 1, texCoord [1] + i, 0);
		G3VERTPOS (vVertPos, pl);
		if (bDynLight)
			G3VertexColor (G3GetNormal (pl, &vNormal), &vVertPos.v3, vertIndex [i], vertColors + i, NULL,
								gameStates.render.nState ? f2fl (uvlList [i].l) : 1, 0, 0);
		else if (bLight)
			SetTMapColor (uvlList + i, i, bmBot, !bOverlay, vertColors + i);
		}
#if 1
	if (gameOpts->render.bDepthSort > 0) {
		OglLoadBmTexture (bmBot, 1, 3, 0);
		RIAddPoly (NULL, bmBot, vertices, nVertices, texCoord [0], NULL, vertColors, nVertices, 1, GL_TRIANGLE_FAN, GL_REPEAT, 0, nSegment);
		return 0;
		}
#endif
	}
#if G3_DRAW_ARRAYS
if (bVertexArrays) {
	if (!G3EnableClientStates (1, 1, 0, GL_TEXTURE0)) {
		bVertexArrays = 0;
		goto retry;
		}
	glVertexPointer (3, GL_FLOAT, sizeof (fVector), vertices);
//	glIndexPointer (GL_INT, 0, colorIndex);
	glTexCoordPointer (2, GL_FLOAT, sizeof (tTexCoord3f), texCoord [0]);
	if (bLight)
		glColorPointer (4, GL_FLOAT, sizeof (tFaceColor), vertColors);
	if (bmTop && !bOverlay) {
		if (!G3EnableClientStates (1, 1, 0, GL_TEXTURE1)) {
			G3DisableClientStates (1, 1, 0, GL_TEXTURE0);
			bVertexArrays = 0;
			goto retry;
			}
		glVertexPointer (3, GL_FLOAT, sizeof (fVector), vertices);
		if (bLight)
			glColorPointer (4, GL_FLOAT, sizeof (tFaceColor), vertColors);
//		glIndexPointer (GL_INT, 0, colorIndex);
		glTexCoordPointer (2, GL_FLOAT, sizeof (tTexCoord3f), texCoord [1]);
		}
	glDrawArrays (GL_TRIANGLE_FAN, 0, nVertices);
	G3DisableClientStates (1, 1, 0, GL_TEXTURE0);
	if (bmTop && !bOverlay)
		G3DisableClientStates (GL_TEXTURE1);
	}
else 
#endif
	{
	glBegin (GL_TRIANGLE_FAN);
	if (bDynLight) {
		if (bOverlay) {
			for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
				pl = *ppl;
				G3VERTPOS (vVertPos, pl);
				G3VertexColor (G3GetNormal (pl, &vNormal), &vVertPos.v3, pl->p3_index, NULL, NULL,
									gameStates.render.nState ? f2fl (uvlList [i].l) : 1, 1, 0);
				glTexCoord2f (f2fl (uvlList [i].u), f2fl (uvlList [i].v));
				glVertex3fv ((GLfloat *) &vVertPos);
				}
			}
		else {
			for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
				pl = *ppl;
				G3VERTPOS (vVertPos, pl);
				G3VertexColor (G3GetNormal (pl, &vNormal), &vVertPos.v3, pl->p3_index, NULL, NULL,
									/*gameStates.render.nState ? f2fl (uvlList [i].l) :*/ 1, 1, 0);
				glMultiTexCoord2f (GL_TEXTURE0, f2fl (uvlList [i].u), f2fl (uvlList [i].v));
				SetTexCoord (uvlList + i, orient, 1, NULL, bmMask != NULL);
				glVertex3fv ((GLfloat *) &vVertPos);
				}
			}
		}
	else if (bLight) {
		if (bOverlay) {
			for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
				if (gameStates.render.nState || !gameOpts->render.nPath || gameStates.app.bEndLevelSequence)
					SetTMapColor (uvlList + i, i, bmBot, 1, NULL);
				else {
					pc = gameData.render.color.vertices + (*ppl)->p3_index;
					glColor3fv ((GLfloat *) &pc->color);
					}
				glTexCoord2f (f2fl (uvlList [i].u), f2fl (uvlList [i].v));
				OglVertex3f (*ppl);
				}
			}
		else {
			bResetColor = (bOverlay != 1);
			for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
				if (gameStates.render.nState || !gameOpts->render.nPath)
					SetTMapColor (uvlList + i, i, bmBot, 1, NULL);
				else {
					pc = gameData.render.color.vertices + (*ppl)->p3_index;
					glColor3fv ((GLfloat *) &pc->color);
					}
				glMultiTexCoord2f (GL_TEXTURE0, f2fl (uvlList [i].u), f2fl (uvlList [i].v));
				SetTexCoord (uvlList + i, orient, 1, NULL, bmMask != NULL);
				OglVertex3f (*ppl);
				}
			}
		}
	else {
		if (bOverlay) {
			for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
				glTexCoord2f (f2fl (uvlList [i].u), f2fl (uvlList [i].v));
				OglVertex3f (*ppl);
				}
			}
		else {
			for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
				glMultiTexCoord2f (GL_TEXTURE0, f2fl (uvlList [i].u), f2fl (uvlList [i].v));
				SetTexCoord (uvlList + i, orient, 1, NULL, bmMask != NULL);
				OglVertex3f (*ppl);
				}
			}
		}
	glEnd ();
	}
if (bOverlay > 0) {
	r_tpolyc++;
	OglActiveTexture (GL_TEXTURE0, 0);
	glEnable (GL_TEXTURE_2D);
	if (OglBindBmTex (bmTop, 1, 3))
		return 1;
	bmTop = BmCurFrame (bmTop, -1);
	OglTexWrap (bmTop->glTexture, GL_REPEAT);
	glBegin (GL_TRIANGLE_FAN);
	if (bDynLight) {
		for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
			G3VertexColor (G3GetNormal (*ppl, &vNormal), VmVecFixToFloat (&vVertPos.v3, &((*ppl)->p3_vec)), (*ppl)->p3_index, NULL, NULL, 1, 1, 0);
			SetTexCoord (uvlList + i, orient, 0, NULL, bmMask != NULL);
			OglVertex3f (*ppl);
			}
		}
	else if (bLight) {
		for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
			SetTMapColor (uvlList + i, i, bmTop, 1, NULL);
			SetTexCoord (uvlList + i, orient, 0, NULL, bmMask != NULL);
			OglVertex3f (*ppl);
			}
		}
	else {
		for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
			SetTexCoord (uvlList + i, orient, 0, NULL, bmMask != NULL);
			OglVertex3f (*ppl);
			}
		}
	glEnd ();
	glDepthFunc (GL_LESS);
#if OGL_CLEANUP
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
#endif
	}
else if (bShaderMerge) {
#if OGL_CLEANUP
	OglActiveTexture (GL_TEXTURE1, bVertexArrays);
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D); // Disable the 2nd texture
#endif
	glUseProgramObject (tmProg = 0);
	}
OglActiveTexture (GL_TEXTURE0, bVertexArrays);
OGL_BINDTEX (0);
glDisable (GL_TEXTURE_2D);
tMapColor.index =
lightColor.index = 0;
if (!bBlend)
	glEnable (GL_BLEND);
return 0;
}

//------------------------------------------------------------------------------

int G3DrawTexPolyLightmap (
	int			nVertices, 
	g3sPoint		**pointList, 
	tUVL			*uvlList, 
	tUVL			*uvlLMap, 
	grsBitmap	*bmBot, 
	grsBitmap	*bmTop, 
	tLightMap	*lightMap, 
	vmsVector	*pvNormal,
	int			orient, 
	int			bBlend,
	short			nSegment)
{
	int			i, nFrame, bShaderMerge;
	grsBitmap	*bmP = NULL;
	g3sPoint		**ppl;

if (gameStates.render.nShadowBlurPass == 1) {
	G3DrawWhitePoly (nVertices, pointList);
	return 0;
	}
if (!bmBot)
	return 1;
r_tpolyc++;
//if (gameStates.render.nShadowPass != 3)
	glDepthFunc (GL_LEQUAL);
if (FAST_SHADOWS) {
	if (bBlend)
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	else
		glDisable (GL_BLEND);
	}
else {
	if (gameStates.render.nShadowPass == 3) {
		glEnable (GL_BLEND);
		glBlendFunc (GL_ONE, GL_ONE);
		}
	}
glDepthFunc (GL_LEQUAL);
bmBot = BmOverride (bmBot, -1);
if ((bmTop = BmOverride (bmTop, -1)) && BM_FRAMES (bmTop)) {
	nFrame = (int) (BM_CURFRAME (bmTop) - BM_FRAMES (bmTop));
	bmP = bmTop;
	bmTop = BM_CURFRAME (bmTop);
	}
else
	nFrame = -1;
if (!lightMap) //lightMapping enabled
	return fpDrawTexPolyMulti (nVertices, pointList, uvlList, uvlLMap, bmBot, bmTop, lightMap, pvNormal, orient, bBlend, nSegment);
// chose shaders depending on whether overlay bitmap present or not
if ((bShaderMerge = bmTop && gameOpts->ogl.bGlTexMerge)) {
	lmProg = lmShaderProgs [(bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0];
	glUseProgramObject (lmProg);
	}
InitTMU0 (0);	// use render pipeline 0 for bottom texture
if (OglBindBmTex (bmBot, 1, 3))
	return 1;
bmBot = BmCurFrame (bmBot, -1);
OglTexWrap (bmBot->glTexture, GL_REPEAT);
if (bShaderMerge)
	glUniform1i (glGetUniformLocation (lmProg, "btmTex"), 0);
if (bmTop) { // use render pipeline 1 for overlay texture
	InitTMU1 (0);
	if (OglBindBmTex (bmTop, 1, 3))
		return 1;
	bmTop = BmCurFrame (bmTop, -1);
	OglTexWrap (bmTop->glTexture, GL_REPEAT);
	glUniform1i (glGetUniformLocation (lmProg, "topTex"), 1);
	}
// use render pipeline 2 for lightmap texture
InitTMU2 (0);
//OGL_BINDTEX (lightMap->handle);
if (bShaderMerge)
	glUniform1i (glGetUniformLocation (lmProg, "lMapTex"), 2);
glBegin (GL_TRIANGLE_FAN);
ppl = pointList;
if (gameStates.render.bFullBright)
	glColor3d (1,1,1);
for (i = 0; i < nVertices; i++, ppl++) {
	if (!gameStates.render.bFullBright)
		SetTMapColor (uvlList + i, i, bmBot, 1, NULL);
	glMultiTexCoord2f (GL_TEXTURE0, f2fl (uvlList [i].u), f2fl (uvlList [i].v));
	if (bmTop)
		SetTexCoord (uvlList + i, orient, 1, NULL, 0);
	glMultiTexCoord2f (GL_TEXTURE2_ARB, f2fl (uvlLMap [i].u), f2fl (uvlLMap [i].v));
	OglVertex3f (*ppl);
	}
glEnd ();
ExitTMU (0);
if (bShaderMerge)
	glUseProgramObject (lmProg = 0);
return 0;
}

//------------------------------------------------------------------------------

int G3DrawTexPolySimple (
	int			nVertices, 
	g3sPoint		**pointList, 
	tUVL			*uvlList, 
	grsBitmap	*bmP, 
	vmsVector	*pvNormal,
	int			bBlend)
{
	int			i;
	int			bLight = 1, 
					bDynLight = gameStates.render.bApplyDynLight && !gameStates.app.bEndLevelSequence;
	g3sPoint		*pl, **ppl;
#if USE_VERTNORMS
	fVector		vNormal, vVertPos;
#endif

if (gameStates.render.nShadowBlurPass == 1) {
	G3DrawWhitePoly (nVertices, pointList);
	return 0;
	}
r_tpolyc++;
if (FAST_SHADOWS) {
	if (!bBlend)
		glDisable (GL_BLEND);
#if 0
	else
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
	}
else {
	if (gameStates.render.nShadowPass == 1)
		bLight = !bDynLight;
	else if (gameStates.render.nShadowPass == 3) {
		glEnable (GL_BLEND);
		glBlendFunc (GL_ONE, GL_ONE);
		}
	}
glDepthFunc (GL_LEQUAL);
bmP = BmOverride (bmP, -1);
if (bmP == gameData.endLevel.satellite.bmP) {
	glActiveTexture (GL_TEXTURE0);
	glEnable (GL_TEXTURE_2D);
	}
else
	InitTMU0 (0);
if (OglBindBmTex (bmP, 1, 3))
	return 1;
//bmP = BmCurFrame (bmP, -1);
if (bmP == bmpDeadzone)
	OglTexWrap (bmP->glTexture, GL_CLAMP);
else
	OglTexWrap (bmP->glTexture, GL_REPEAT);

if (SHOW_DYN_LIGHT) {
#if USE_VERTNORMS
	if (pvNormal)
		VmVecFixToFloat (&vNormal, pvNormal);
else
		G3CalcNormal (pointList, &vNormal);
#else
		G3Normal (pointList, pvNormal);
#endif
	}
if (gameStates.render.bFullBright) {
	glColor3d (1,1,1);
	bLight = 0;
	}
else if (!gameStates.render.nRenderPass)
	bLight = 0;
else if (!bLight)
	glColor3i (0,0,0);
if (!bLight)
	bDynLight = 0;
gameStates.ogl.bDynObjLight = bDynLight;
gameStates.ogl.fAlpha = gameStates.render.grAlpha / (float) GR_ACTUAL_FADE_LEVELS;
glBegin (GL_TRIANGLE_FAN);
if (bDynLight) {
	for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
		pl = *ppl;
		G3VERTPOS (vVertPos, pl);
		G3VertexColor (G3GetNormal (pl, &vNormal), &vVertPos.v3, pl->p3_index, NULL, NULL,
							/*gameStates.render.nState ? f2fl (uvlList [i].l) :*/ 1, 1, 0);
		glTexCoord2f (f2fl (uvlList [i].u), f2fl (uvlList [i].v));
		glVertex3fv ((GLfloat *) &vVertPos);
		}
	}
else if (bLight) {
	for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
		SetTMapColor (uvlList + i, i, bmP, 1, NULL);
		glTexCoord2f (f2fl (uvlList [i].u), f2fl (uvlList [i].v));
		OglVertex3f (*ppl);
		}
	}
else {
	for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
		glTexCoord2f (f2fl (uvlList [i].u), f2fl (uvlList [i].v));
		OglVertex3f (*ppl);
		}
	}
glEnd ();
glDisable (GL_TEXTURE_2D);
tMapColor.index =
lightColor.index = 0;
if (!bBlend)
	glEnable (GL_BLEND);
return 0;
}

//------------------------------------------------------------------------------

int G3DrawSprite (
	vmsVector	*vPos, 
	fix			xWidth, 
	fix			xHeight, 
	grsBitmap	*bmP, 
	tRgbaColorf	*colorP,
	float			alpha,
	int			bAdditive)
{
	vmsVector	pv, v1;
	GLdouble		h, w, u, v, x, y, z;

if (gameOpts->render.bDepthSort > 0) { //&& ((colorP && (colorP->alpha < 0)) || (alpha < 0))) {
	tRgbaColorf color;
	if (!colorP) {
		color.red =
		color.green =
		color.blue = 1;
		color.alpha = alpha;
		colorP = &color;
		}
	RIAddSprite (bmP, vPos, colorP, xWidth, xHeight, 0, bAdditive);
	}
else {
	OglActiveTexture (GL_TEXTURE0, 0);
	VmVecSub (&v1, vPos, &viewInfo.pos);
	VmVecRotate (&pv, &v1, &viewInfo.view [0]);
	x = (double) f2fl (pv.p.x);
	y = (double) f2fl (pv.p.y);
	z = (double) f2fl (pv.p.z);
	w = (double) f2fl (xWidth); 
	h = (double) f2fl (xHeight); 
	if (gameStates.render.nShadowBlurPass == 1) {
		glDisable (GL_TEXTURE_2D);
		glColor4d (1,1,1,1);
		glBegin (GL_QUADS);
		glVertex3d (x - w, y + h, z);
		glVertex3d (x + w, y + h, z);
		glVertex3d (x + w, y - h, z);
		glVertex3d (x - w, y - h, z);
		glEnd ();
		}
	else {
		glDepthMask (0);
		glEnable (GL_TEXTURE_2D);
		if (OglBindBmTex (bmP, 1, 1)) 
			return 1;
		bmP = BmOverride (bmP, -1);
		OglTexWrap (bmP->glTexture, GL_CLAMP);
		glEnable (GL_BLEND);
		if (bAdditive == 2)
			glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		else if (bAdditive == 1)
			glBlendFunc (GL_ONE, GL_ONE);
		else
			glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		if (colorP)
			glColor4f (colorP->red, colorP->green, colorP->blue, colorP->alpha);
		else
			glColor4d (1, 1, 1, (double) alpha);
		glBegin (GL_QUADS);
		u = bmP->glTexture->u;
		v = bmP->glTexture->v;
		glTexCoord2d (0, 0);
		glVertex3d (x - w, y + h, z);
		glTexCoord2d (u, 0);
		glVertex3d (x + w, y + h, z);
		glTexCoord2d (u, v);
		glVertex3d (x + w, y - h, z);
		glTexCoord2d (0, v);
		glVertex3d (x - w, y - h, z);
		glEnd ();
		glDepthMask (1);
		if (bAdditive)
			glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glDisable (GL_BLEND);
		}
	}
return 0;
} 

//------------------------------------------------------------------------------

int OglRenderArrays (grsBitmap *bmP, int nFrame, fVector *vertexP, int nVertices, tTexCoord2f *texCoordP, 
							tRgbaColorf *colorP, int nColors, int nPrimitive, int nWrap)
{
	int	bVertexArrays = G3EnableClientStates (bmP && texCoordP, colorP && (nColors == nVertices), 0, GL_TEXTURE0);

if (bmP)
	glEnable (GL_TEXTURE_2D);
else
	glDisable (GL_TEXTURE_2D);
if (bmP) {
	if (OglBindBmTex (bmP, 1, 1))
		return 0;
	bmP = BmOverride (bmP, -1);
	if (BM_FRAMES (bmP))
		bmP = BM_FRAMES (bmP) + nFrame;
	OglTexWrap (bmP->glTexture, nWrap);
	}
if (bVertexArrays) {
	if (texCoordP)
		glTexCoordPointer (2, GL_FLOAT, sizeof (tTexCoord2f), texCoordP);
	if (colorP) {
		if (nColors == nVertices)
			glColorPointer (4, GL_FLOAT, sizeof (tRgbaColorf), colorP);
		else
			glColor4fv ((GLfloat *) colorP);
		}
	glVertexPointer (3, GL_FLOAT, sizeof (fVector), vertexP);
	glDrawArrays (nPrimitive, 0, nVertices);
	glDisableClientState (GL_VERTEX_ARRAY);
	if (texCoordP)
		glDisableClientState (GL_TEXTURE_COORD_ARRAY);
	if (colorP)
		glDisableClientState (GL_COLOR_ARRAY);
	}
else {
	int i = nVertices;
	glBegin (nPrimitive);
	if (colorP && (nColors == nVertices)) {
		if (bmP) {
			for (i = 0; i < nVertices; i++) {
				glColor4fv ((GLfloat *) (colorP + i));
				glVertex3fv ((GLfloat *) (vertexP + i));
				glTexCoord2fv ((GLfloat *) (texCoordP + i));
				}
			}
		else {
			for (i = 0; i < nVertices; i++) {
				glColor4fv ((GLfloat *) (colorP + i));
				glVertex3fv ((GLfloat *) (vertexP + i));
				}
			}
		}
	else {
		if (colorP)
			glColor4fv ((GLfloat *) colorP);
		else
			glColor3d (1, 1, 1);
		if (bmP) {
			for (i = 0; i < nVertices; i++) {
				glVertex3fv ((GLfloat *) (vertexP + i));
				glTexCoord2fv ((GLfloat *) (texCoordP + i));
				}
			}
		else {
			for (i = 0; i < nVertices; i++) {
				glVertex3fv ((GLfloat *) (vertexP + i));
				}
			}
		}
	glEnd ();
	}
return 1;
}

//------------------------------------------------------------------------------
