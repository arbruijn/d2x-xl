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

#include "descent.h"
#include "error.h"
#include "maths.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "ogl_shader.h"
#include "ogl_hudstuff.h"
#include "ogl_render.h"
#include "ogl_tmu.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "texmerge.h"
#include "glare.h"
#include "renderlib.h"
#include "transprender.h"

//------------------------------------------------------------------------------

#define OGL_CLEANUP			1
#define USE_VERTNORMS		1
#define G3_DRAW_ARRAYS		0
#define G3_MULTI_TEXTURE	0

//------------------------------------------------------------------------------

GLhandleARB	lmProg = (GLhandleARB) 0;
GLhandleARB	activeShaderProg = (GLhandleARB) 0;

tTexPolyMultiDrawer	*fpDrawTexPolyMulti = NULL;

//------------------------------------------------------------------------------

inline void SetTexCoord (tUVL *uvlList, int nOrient, int bMulti, tTexCoord2f *texCoord, int bMask)
{
	float u1, v1;

if (nOrient == 1) {
	u1 = 1.0f - X2F (uvlList->v);
	v1 = X2F (uvlList->u);
	}
else if (nOrient == 2) {
	u1 = 1.0f - X2F (uvlList->u);
	v1 = 1.0f - X2F (uvlList->v);
	}
else if (nOrient == 3) {
	u1 = X2F (uvlList->v);
	v1 = 1.0f - X2F (uvlList->u);
	}
else {
	u1 = X2F (uvlList->u);
	v1 = X2F (uvlList->v);
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

inline int G3BindTex (CBitmap *bmP, GLint nTexId, GLhandleARB lmProg, char *pszTexId,
						    pInitTMU initTMU, int bShaderVar, int bVertexArrays)
{
if (bmP || (nTexId >= 0)) {
	initTMU (bVertexArrays);
	if (nTexId >= 0)
		OGL_BINDTEX (nTexId);
	else {
		if (bmP->Bind (1, 3))
			return 1;
		bmP->Texture ()->Wrap (GL_REPEAT);
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
OglCanvasColor (&CCanvas::Current ()->Color ());
glBegin (GL_LINES);
OglVertex3x (p0->p3_vec[X], p0->p3_vec[Y], p0->p3_vec[Z]);
OglVertex3x (p1->p3_vec[X], p1->p3_vec[Y], p1->p3_vec[Z]);
if (CCanvas::Current ()->Color ().rgb)
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
if (nType == GL_LINES) {	// implies a dashed circle
	if (sinCosP) {
		for (i = 0; i < nSides; i++, sinCosP++) {
			glVertex2f (sinCosP->fCos * xsc + xo, sinCosP->fSin * ysc + yo);
			i++, sinCosP++;
			glVertex2f (sinCosP->fCos * xsc + xo, sinCosP->fSin * ysc + yo);
			}
		}
	else {
		for (i = 0; i < nSides; i++) {
			ang = 2.0 * Pi * i / nSides;
			glVertex2f ((float) cos (ang) * xsc + xo, (float) sin (ang) * ysc + yo);
			i++;
			ang = 2.0 * Pi * i / nSides;
			glVertex2f ((float) cos (ang) * xsc + xo, (float) sin (ang) * ysc + yo);
			}
		}
	}
else {
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
OglCanvasColor (&CCanvas::Current ()->Color ());
glPushMatrix ();
glTranslatef (X2F (pnt->p3_vec[X]), X2F (pnt->p3_vec[Y]), X2F (pnt->p3_vec[Z]));
r = X2F (rad);
glScaled (r, r, r);
if (bBigSphere) {
#if 1
	OglDrawCircle (20, GL_POLYGON);
#else
	if (hBigSphere)
		glCallList (hBigSphere);
	else
		hBigSphere = CircleListInit (20, GL_POLYGON, GL_COMPILE_AND_EXECUTE);
#endif
	}
else {
	if (hSmallSphere)
		glCallList (hSmallSphere);
	else
		hSmallSphere = CircleListInit (12, GL_POLYGON, GL_COMPILE_AND_EXECUTE);
	}
glPopMatrix ();
if (CCanvas::Current ()->Color ().rgb)
	glDisable (GL_BLEND);
return 0;
}

//------------------------------------------------------------------------------

int G3DrawSphere3D (g3sPoint *p0, int nSides, int rad)
{
	tCanvasColor	c = CCanvas::Current ()->Color ();
	g3sPoint			p = *p0;
	int				i;
	float				hx, hy, x, y, z, r;
	float				ang;

glDisable (GL_TEXTURE_2D);
OglCanvasColor (&CCanvas::Current ()->Color ());
x = X2F (p.p3_vec[X]);
y = X2F (p.p3_vec[Y]);
z = X2F (p.p3_vec[Z]);
r = X2F (rad);
glBegin (GL_POLYGON);
for (i = 0; i <= nSides; i++) {
	ang = 2.0f * (float) Pi * (i % nSides) / nSides;
	hx = x + (float) cos (ang) * r;
	hy = y + (float) sin (ang) * r;
	glVertex3f (hx, hy, z);
	}
if (c.rgb)
	glDisable (GL_BLEND);
glEnd ();
return 1;
}

//------------------------------------------------------------------------------

int G3DrawCircle3D (g3sPoint *p0, int nSides, int rad)
{
	g3sPoint		p = *p0;
	int			i, j;
	CFloatVector		v;
	float			x, y, r;
	float			ang;

glDisable (GL_TEXTURE_2D);
OglCanvasColor (&CCanvas::Current ()->Color ());
x = X2F (p.p3_vec[X]);
y = X2F (p.p3_vec[Y]);
v[Z] = X2F (p.p3_vec[Z]);
r = X2F (rad);
glBegin (GL_LINES);
for (i = 0; i <= nSides; i++)
	for (j = i; j <= i + 1; j++) {
		ang = 2.0f * (float) Pi * (j % nSides) / nSides;
		v[X] = x + (float) cos (ang) * r;
		v[Y] = y + (float) sin (ang) * r;
		glVertex3fv (reinterpret_cast<GLfloat*> (&v));
		}
if (CCanvas::Current ()->Color ().rgb)
	glDisable (GL_BLEND);
glEnd ();
return 1;
}

//------------------------------------------------------------------------------

int GrUCircle (fix xc1, fix yc1, fix r1)
{//dunno if this really works, radar doesn't seem to.. hm..
glDisable (GL_TEXTURE_2D);
//	glPointSize (X2F (rad);
OglCanvasColor (&CCanvas::Current ()->Color ());
glPushMatrix ();
glTranslatef (
			(X2F (xc1) + CCanvas::Current ()->Left ()) / (float) gameStates.ogl.nLastW,
		1.0f - (X2F (yc1) + CCanvas::Current ()->Top ()) / (float) gameStates.ogl.nLastH, 0);
glScalef (X2F (r1), X2F (r1), X2F (r1));
if (r1<=I2X (5)){
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
if (CCanvas::Current ()->Color ().rgb)
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
OglCanvasColor (&CCanvas::Current ()->Color ());
glBegin (GL_TRIANGLE_FAN);
for (i = 0; i < nVertices; i++, pointList++) {
//	glVertex3f (X2F (pointList [c]->p3_vec[X]), X2F (pointList [c]->p3_vec[Y]), X2F (pointList [c]->p3_vec[Z]);
	OglVertex3f (*pointList);
	}
#if 1
if (CCanvas::Current ()->Color ().rgb || (gameStates.render.grAlpha < 1.0f))
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
	color->alpha = gameStates.render.grAlpha;
#if 1
if (gameOpts->render.bDepthSort > 0) {
	CFloatVector	vertices [8];

	for (i = 0; i < nVertices; i++)
		vertices [i] = gameData.render.vertP [pointList [i]->p3_index];
	transparencyRenderer.AddPoly (NULL, NULL, NULL, vertices, nVertices, NULL, color, NULL, 1, bDepthMask, GL_TRIANGLE_FAN, GL_REPEAT, 0, nSegment);
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
	glColor4fv (reinterpret_cast<GLfloat*> (color));
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
console.printf (CON_DBG, "gr_upoly_tmap: unhandled\n");//should never get called
#endif
}

//------------------------------------------------------------------------------

void DrawTexPolyFlat (CBitmap *bmP, int nVertices, g3sPoint **vertlist)
{
#if TRACE
console.printf (CON_DBG, "DrawTexPolyFlat: unhandled\n");//should never get called
#endif
}

//------------------------------------------------------------------------------

int G3DrawTexPolyFlat (
	int			nVertices,
	g3sPoint		**pointList,
	tUVL			*uvlList,
	tUVL			*uvlLMap,
	CBitmap	*bmBot,
	CBitmap	*bmTop,
	tLightmap	*lightmap,
	CFixVector	*pvNormal,
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
glColor4d (0, 0, 0, gameStates.render.grAlpha);
glBegin (GL_TRIANGLE_FAN);
for (i = 0, ppl = pointList; i < nVertices; i++, ppl++)
	OglVertex3f (*ppl);
glEnd ();
return 0;
}

//------------------------------------------------------------------------------

#define	G3VERTPOS(_dest,_src) \
			if ((_src)->p3_index < 0) \
				(_dest).Assign ((_src)->p3_vec); \
			else \
				_dest = gameData.render.vertP [(_src)->p3_index];

#define	G3VERTPOS3(_dest,_src) \
			if ((_src)->p3_index < 0) \
				(_dest).Assign ((_src)->p3_vec); \
			else \
				_dest = gameData.render.vertP [(_src)->p3_index];

//------------------------------------------------------------------------------

int G3DrawTexPolyMulti (
	int			nVertices,
	g3sPoint**	pointList,
	tUVL*			uvlList,
	tUVL*			uvlLMap,
	CBitmap*		bmBot,
	CBitmap*		bmTop,
	tLightmap*	lightmap,
	CFixVector*	pvNormal,
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
	CBitmap		*bmP = NULL, *mask = NULL;
	g3sPoint		*pl, **ppl;
#if USE_VERTNORMS
	CFloatVector	vNormal, vVertPos;
#endif
#if G3_DRAW_ARRAYS
	int			bVertexArrays = gameData.render.vertP != NULL;
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
bmBot = bmBot->Override (-1);
bDepthSort = (!bmTop && (gameOpts->render.bDepthSort > 0) &&
				  ((gameStates.render.grAlpha < 1.0f) ||
				   (bmBot->Flags () & (BM_FLAG_TRANSPARENT | BM_FLAG_SEE_THRU | BM_FLAG_TGA)) == (BM_FLAG_TRANSPARENT | BM_FLAG_TGA)));
if (bmTop && (bmTop = bmTop->Override (-1)) && bmTop->Frames ()) {
	nFrame = (int) (bmTop->CurFrame () - bmTop->Frames ());
	bmP = bmTop;
	bmTop = bmTop->CurFrame ();
	}
else
	nFrame = -1;
if (bmTop) {
	if (nFrame < 0)
      bSuperTransp = (bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT) != 0;
	else
		bSuperTransp = (bmP->Flags () & BM_FLAG_SUPER_TRANSPARENT) != 0;
	bShaderMerge = bSuperTransp && gameStates.ogl.bGlTexMerge;
	bOverlay = !bShaderMerge;
	}
else
	bOverlay = -1;
#if G3_DRAW_ARRAYS
retry:
#endif
if (bShaderMerge) {
	mask = gameStates.render.textures.bHaveMaskShader ? bmTop->Mask () : NULL;
	nShader = bSuperTransp ? mask ? 2 : 1 : 0;
	glUseProgramObject (activeShaderProg = tmShaderProgs [nShader]);
	INIT_TMU (InitTMU0, GL_TEXTURE0, bmBot, lightmapManager.Buffer (), bVertexArrays, 0);
	glUniform1i (glGetUniformLocation (activeShaderProg, "btmTex"), 0);
	INIT_TMU (InitTMU1, GL_TEXTURE1, bmTop, lightmapManager.Buffer (), bVertexArrays, 0);
	glUniform1i (glGetUniformLocation (activeShaderProg, "topTex"), 1);
	if (mask) {
#if DBG
		InitTMU2 (bVertexArrays);
		G3_BIND (GL_TEXTURE2, mask, lightmapManager.Buffer (), bVertexArrays);
#else
		INIT_TMU (InitTMU2, GL_TEXTURE2, mask, lightmapManager.Buffer (), bVertexArrays, 0);
#endif
		glUniform1i (glGetUniformLocation (activeShaderProg, "maskTex"), 2);
		}
	glUniform1f (glGetUniformLocation (activeShaderProg, "grAlpha"), gameStates.render.grAlpha);
	}
else if (!bDepthSort) {
	if (bmBot == gameData.endLevel.satellite.bmP) {
		glActiveTexture (GL_TEXTURE0);
		glEnable (GL_TEXTURE_2D);
		}
	else
		InitTMU0 (bVertexArrays);
	if (bmBot->Bind (1, 3))
		return 1;
	bmBot = bmBot->CurFrame (-1);
	bmBot->Texture ()->Wrap ((bmBot == bmpDeadzone) ? GL_CLAMP : GL_REPEAT);
	}

if (!bDepthSort) {
	if (SHOW_DYN_LIGHT) {
#if USE_VERTNORMS
		if (pvNormal) {
			vNormal.Assign (*pvNormal);
			transformation.Rotate(vNormal, vNormal, 0);
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

gameStates.ogl.fAlpha = gameStates.render.grAlpha;
if (bVertexArrays || bDepthSort) {
		CFloatVector	vertices [8];
		tFaceColor		vertColors [8];
		tTexCoord2f		texCoord [2][8];
		int				vertIndex [8];
		//int				colorIndex [8];

	for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
		pl = *ppl;
		vertIndex [i] = pl->p3_index;
		//colorIndex [i] = i;
		if (pl->p3_index < 0)
			vertices[i].Assign (pl->p3_vec);
		else
			vertices [i] = gameData.render.vertP [pl->p3_index];
		texCoord [0][i].v.u = X2F (uvlList [i].u);
		texCoord [0][i].v.v = X2F (uvlList [i].v);
		SetTexCoord (uvlList + i, orient, 1, texCoord [1] + i, 0);
		G3VERTPOS (vVertPos, pl);
		if (bDynLight)
			G3VertexColor (G3GetNormal (pl, &vNormal), vVertPos.V3(), vertIndex [i], vertColors + i, NULL,
								gameStates.render.nState ? X2F (uvlList [i].l) : 1, 0, 0);
		else if (bLight)
			SetTMapColor (uvlList + i, i, bmBot, !bOverlay, vertColors + i);
		}
#if 1
	if (gameOpts->render.bDepthSort > 0) {
		bmBot->SetupTexture (1, 0);
		transparencyRenderer.AddPoly (NULL, NULL, bmBot, vertices, nVertices, texCoord [0], NULL, vertColors, nVertices, 1, GL_TRIANGLE_FAN, GL_REPEAT, 0, nSegment);
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
	glVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), vertices);
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
		glVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), vertices);
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
				G3VertexColor (G3GetNormal (pl, &vNormal), vVertPos.V3(), pl->p3_index, NULL, NULL,
									gameStates.render.nState ? X2F (uvlList [i].l) : 1, 1, 0);
				glTexCoord2f (X2F (uvlList [i].u), X2F (uvlList [i].v));
				glVertex3fv (reinterpret_cast<GLfloat*> (&vVertPos));
				}
			}
		else {
			for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
				pl = *ppl;
				G3VERTPOS (vVertPos, pl);
				G3VertexColor (G3GetNormal (pl, &vNormal), vVertPos.V3(), pl->p3_index, NULL, NULL,
									/*gameStates.render.nState ? X2F (uvlList [i].l) :*/ 1, 1, 0);
				glMultiTexCoord2f (GL_TEXTURE0, X2F (uvlList [i].u), X2F (uvlList [i].v));
				SetTexCoord (uvlList + i, orient, 1, NULL, mask != NULL);
				glVertex3fv (reinterpret_cast<GLfloat*> (&vVertPos));
				}
			}
		}
	else if (bLight) {
		if (bOverlay) {
			for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
				if (gameStates.render.nState || !RENDERPATH || gameStates.app.bEndLevelSequence)
					SetTMapColor (uvlList + i, i, bmBot, 1, NULL);
				else {
					pc = gameData.render.color.vertices + (*ppl)->p3_index;
					glColor3fv (reinterpret_cast<GLfloat*> (&pc->color));
					}
				glTexCoord2f (X2F (uvlList [i].u), X2F (uvlList [i].v));
				OglVertex3f (*ppl);
				}
			}
		else {
			bResetColor = (bOverlay != 1);
			for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
				if (gameStates.render.nState || !RENDERPATH)
					SetTMapColor (uvlList + i, i, bmBot, 1, NULL);
				else {
					pc = gameData.render.color.vertices + (*ppl)->p3_index;
					glColor3fv (reinterpret_cast<GLfloat*> (&pc->color));
					}
				glMultiTexCoord2f (GL_TEXTURE0, X2F (uvlList [i].u), X2F (uvlList [i].v));
				SetTexCoord (uvlList + i, orient, 1, NULL, mask != NULL);
				OglVertex3f (*ppl);
				}
			}
		}
	else {
		if (bOverlay) {
			for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
				glTexCoord2f (X2F (uvlList [i].u), X2F (uvlList [i].v));
				OglVertex3f (*ppl);
				}
			}
		else {
			for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
				glMultiTexCoord2f (GL_TEXTURE0, X2F (uvlList [i].u), X2F (uvlList [i].v));
				SetTexCoord (uvlList + i, orient, 1, NULL, mask != NULL);
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
	if (bmTop->Bind (1, 3))
		return 1;
	bmTop = bmTop->CurFrame (-1);
	bmTop->Texture ()->Wrap (GL_REPEAT);
	glBegin (GL_TRIANGLE_FAN);
	if (bDynLight) {
		for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
			vVertPos.Assign ((*ppl)->p3_vec);
			G3VertexColor (G3GetNormal (*ppl, &vNormal), vVertPos.V3(), (*ppl)->p3_index, NULL, NULL, 1, 1, 0);
			SetTexCoord (uvlList + i, orient, 0, NULL, mask != NULL);
			OglVertex3f (*ppl);
			}
		}
	else if (bLight) {
		for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
			SetTMapColor (uvlList + i, i, bmTop, 1, NULL);
			SetTexCoord (uvlList + i, orient, 0, NULL, mask != NULL);
			OglVertex3f (*ppl);
			}
		}
	else {
		for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
			SetTexCoord (uvlList + i, orient, 0, NULL, mask != NULL);
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
	glUseProgramObject (activeShaderProg = 0);
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
	CBitmap	*bmBot,
	CBitmap	*bmTop,
	tLightmap	*lightmap,
	CFixVector	*pvNormal,
	int			orient,
	int			bBlend,
	short			nSegment)
{
	int			i, nFrame, bShaderMerge;
	CBitmap	*bmP = NULL;
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
bmBot = bmBot->Override (-1);
if ((bmTop = bmTop->Override (-1)) && bmTop->Frames ()) {
	nFrame = (int) (bmTop->CurFrame () - bmTop->Frames ());
	bmP = bmTop;
	bmTop = bmTop->CurFrame ();
	}
else
	nFrame = -1;
if (!lightmap) //lightmapping enabled
	return fpDrawTexPolyMulti (nVertices, pointList, uvlList, uvlLMap, bmBot, bmTop, lightmap, pvNormal, orient, bBlend, nSegment);
// chose shaders depending on whether overlay bitmap present or not
if ((bShaderMerge = bmTop && gameOpts->ogl.bGlTexMerge)) {
	lmProg = lmShaderProgs [(bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT) != 0];
	glUseProgramObject (lmProg);
	}
InitTMU0 (0);	// use render pipeline 0 for bottom texture
if (bmBot->Bind (1, 3))
	return 1;
bmBot = bmBot->CurFrame (-1);
bmBot->Texture ()->Wrap (GL_REPEAT);
if (bShaderMerge)
	glUniform1i (glGetUniformLocation (lmProg, "btmTex"), 0);
if (bmTop) { // use render pipeline 1 for overlay texture
	InitTMU1 (0);
	if (bmTop->Bind (1, 3))
		return 1;
	bmTop = bmTop->CurFrame (-1);
	bmTop->Texture ()->Wrap (GL_REPEAT);
	glUniform1i (glGetUniformLocation (lmProg, "topTex"), 1);
	}
// use render pipeline 2 for lightmap texture
InitTMU2 (0);
//OGL_BINDTEX (lightmap->handle);
if (bShaderMerge)
	glUniform1i (glGetUniformLocation (lmProg, "lMapTex"), 2);
glBegin (GL_TRIANGLE_FAN);
ppl = pointList;
if (gameStates.render.bFullBright)
	glColor3d (1,1,1);
for (i = 0; i < nVertices; i++, ppl++) {
	if (!gameStates.render.bFullBright)
		SetTMapColor (uvlList + i, i, bmBot, 1, NULL);
	glMultiTexCoord2f (GL_TEXTURE0, X2F (uvlList [i].u), X2F (uvlList [i].v));
	if (bmTop)
		SetTexCoord (uvlList + i, orient, 1, NULL, 0);
	glMultiTexCoord2f (GL_TEXTURE2_ARB, X2F (uvlLMap [i].u), X2F (uvlLMap [i].v));
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
	CBitmap	*bmP,
	CFixVector	*pvNormal,
	int			bBlend)
{
	int			i;
	int			bLight = 1,
					bDynLight = gameStates.render.bApplyDynLight && !gameStates.app.bEndLevelSequence;
	g3sPoint		*pl, **ppl;
#if USE_VERTNORMS
	CFloatVector		vNormal, vVertPos;
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
bmP = bmP->Override (-1);
if (bmP == gameData.endLevel.satellite.bmP) {
	glActiveTexture (GL_TEXTURE0);
	glEnable (GL_TEXTURE_2D);
	}
else
	InitTMU0 (0);
if (bmP->Bind (1, 3))
	return 1;
if (bmP == bmpDeadzone)
	bmP->Texture ()->Wrap (GL_CLAMP);
else
	bmP->Texture ()->Wrap (GL_REPEAT);

if (SHOW_DYN_LIGHT) {
#if USE_VERTNORMS
	if (pvNormal)
		vNormal.Assign (*pvNormal);
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
gameStates.ogl.fAlpha = gameStates.render.grAlpha;
glBegin (GL_TRIANGLE_FAN);
if (bDynLight) {
	for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
		pl = *ppl;
		G3VERTPOS (vVertPos, pl);
		G3VertexColor (G3GetNormal (pl, &vNormal), vVertPos.V3(), pl->p3_index, NULL, NULL,
							/*gameStates.render.nState ? X2F (uvlList [i].l) :*/ 1, 1, 0);
		glTexCoord2f (X2F (uvlList [i].u), X2F (uvlList [i].v));
		glVertex3fv (reinterpret_cast<GLfloat*> (&vVertPos));
		}
	}
else if (bLight) {
	for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
		SetTMapColor (uvlList + i, i, bmP, 1, NULL);
		glTexCoord2f (X2F (uvlList [i].u), X2F (uvlList [i].v));
		OglVertex3f (*ppl);
		}
	}
else {
	for (i = 0, ppl = pointList; i < nVertices; i++, ppl++) {
		glTexCoord2f (X2F (uvlList [i].u), X2F (uvlList [i].v));
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
	const CFixVector&	vPos,
	fix			xWidth,
	fix			xHeight,
	CBitmap	*bmP,
	tRgbaColorf	*colorP,
	float			alpha,
	int			bAdditive,
	float			fSoftRad)
{
	CFixVector	pv, v1;
	GLdouble		h, w, u, v, x, y, z;

if ((gameOpts->render.bDepthSort > 0) /*|| (gameOpts->render.effects.bSoftParticles & 1)*/) { //&& ((colorP && (colorP->alpha < 0)) || (alpha < 0))) {
	tRgbaColorf color;
	if (!colorP) {
		color.red =
		color.green =
		color.blue = 1;
		color.alpha = alpha;
		colorP = &color;
		}
	transparencyRenderer.AddSprite (bmP, vPos, colorP, xWidth, xHeight, 0, bAdditive, fSoftRad);
	}
else {
	OglActiveTexture (GL_TEXTURE0, 0);
	v1 = vPos - transformation.m_info.pos;
	pv = transformation.m_info.view [0] * v1;
	x = (double) X2F (pv[X]);
	y = (double) X2F (pv[Y]);
	z = (double) X2F (pv[Z]);
	w = (double) X2F (xWidth);
	h = (double) X2F (xHeight);
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
		if (bmP->Bind (1, 1))
			return 1;
		bmP = bmP->Override (-1);
		bmP->Texture ()->Wrap (GL_CLAMP);
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
		u = bmP->Texture ()->U ();
		v = bmP->Texture ()->V ();
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

int OglRenderArrays (CBitmap *bmP, int nFrame, CFloatVector *vertexP, int nVertices, tTexCoord2f *texCoordP,
							tRgbaColorf *colorP, int nColors, int nPrimitive, int nWrap)
{
	int	bVertexArrays = G3EnableClientStates (bmP && texCoordP, colorP && (nColors == nVertices), 0, GL_TEXTURE0);

if (bmP)
	glEnable (GL_TEXTURE_2D);
else
	glDisable (GL_TEXTURE_2D);
if (bmP) {
	if (bmP->Bind (1, 1))
		return 0;
	bmP = bmP->Override (-1);
	if (bmP->Frames ())
		bmP = bmP->Frames () + nFrame;
	bmP->Texture ()->Wrap (nWrap);
	}
if (bVertexArrays) {
	if (texCoordP)
		glTexCoordPointer (2, GL_FLOAT, sizeof (tTexCoord2f), texCoordP);
	if (colorP) {
		if (nColors == nVertices)
			glColorPointer (4, GL_FLOAT, sizeof (tRgbaColorf), colorP);
		else
			glColor4fv (reinterpret_cast<GLfloat*> (colorP));
		}
	glVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), vertexP);
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
				glColor4fv (reinterpret_cast<GLfloat*> (colorP + i));
				glVertex3fv (reinterpret_cast<GLfloat*> (vertexP + i));
				glTexCoord2fv (reinterpret_cast<GLfloat*> (texCoordP + i));
				}
			}
		else {
			for (i = 0; i < nVertices; i++) {
				glColor4fv (reinterpret_cast<GLfloat*> (colorP + i));
				glVertex3fv (reinterpret_cast<GLfloat*> (vertexP + i));
				}
			}
		}
	else {
		if (colorP)
			glColor4fv (reinterpret_cast<GLfloat*> (colorP));
		else
			glColor3d (1, 1, 1);
		if (bmP) {
			for (i = 0; i < nVertices; i++) {
				glVertex3fv (reinterpret_cast<GLfloat*> (vertexP + i));
				glTexCoord2fv (reinterpret_cast<GLfloat*> (texCoordP + i));
				}
			}
		else {
			for (i = 0; i < nVertices; i++) {
				glVertex3fv (reinterpret_cast<GLfloat*> (vertexP + i));
				}
			}
		}
	glEnd ();
	}
return 1;
}

//------------------------------------------------------------------------------
