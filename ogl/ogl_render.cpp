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
#include "addon_bitmaps.h"

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
	if (bMulti) {
		glMultiTexCoord2f (GL_TEXTURE1, u1, v1);
		if (bMask)
			glMultiTexCoord2f (GL_TEXTURE2, u1, v1);
	}
else
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
		ogl.BindTexture (nTexId);
	else {
		if (bmP->Bind (1))
			return 1;
		bmP->Texture ()->Wrap (GL_REPEAT);
		}
	if (bShaderVar)
		glUniform1i (glGetUniformLocation (lmProg, pszTexId), 0);
	}
return 0;
}

//------------------------------------------------------------------------------

int G3DrawLine (CRenderPoint *p0, CRenderPoint *p1)
{
if (ogl.SizeVertexBuffer (2)) {
	ogl.SetTexturing (false);
	OglCanvasColor (&CCanvas::Current ()->Color ());
	ogl.VertexBuffer () [0].Assign (p0->ViewPos ());
	ogl.VertexBuffer () [1].Assign (p1->ViewPos ());
	ogl.FlushBuffers (GL_LINES, 2, 3);
	if (CCanvas::Current ()->Color ().rgb)
		ogl.SetBlending (false);
	}
return 1;
}

//------------------------------------------------------------------------------

void OglDrawEllipse (int nSides, int nType, float xScale, float xOffset, float yScale, float yOffset, tSinCosf *sinCosP)
{
	int		i;
	double	ang;

glPushMatrix ();
ogl.SetLineSmooth (true);
glTranslatef (xOffset, yOffset, 0.0f);
glScalef (xScale, yScale, 1.0f);
if (nType == GL_LINES) {	// implies a dashed circle
	if (ogl.SizeVertexBuffer (nSides * 2)) {
		if (sinCosP) {
			for (i = 0; i < nSides; i++, sinCosP++) {
				ogl.VertexBuffer () [i].v.coord.x = sinCosP->fCos;
				ogl.VertexBuffer () [i].v.coord.y = sinCosP->fSin;
				i++, sinCosP++;
				ogl.VertexBuffer () [i].v.coord.x = sinCosP->fCos;
				ogl.VertexBuffer () [i].v.coord.y = sinCosP->fSin;
				}
			}
		else {
			for (i = 0; i < nSides; i++) {
				ang = 2.0 * PI * i / nSides;
				ogl.VertexBuffer () [i].v.coord.x = float (cos (ang));
				ogl.VertexBuffer () [i].v.coord.y = float (sin (ang));
				i++;
				ang = 2.0 * PI * i / nSides;
				ogl.VertexBuffer () [i].v.coord.x = float (cos (ang));
				ogl.VertexBuffer () [i].v.coord.y = float (sin (ang));
				}
			}
		ogl.FlushBuffers (GL_LINES, nSides * 2, 2);
		}
	}
else {
	if (sinCosP) {
		ogl.EnableClientStates (0, 0, 0, -1);
		OglVertexPointer (2, GL_FLOAT, 2 * sizeof (float), reinterpret_cast<GLfloat*> (sinCosP));
		OglDrawArrays (nType, 0, nSides);
		ogl.DisableClientStates (0, 0, 0, -1);
		}
	else {
		if (ogl.SizeVertexBuffer (nSides)) {
			for (i = 0; i < nSides; i++) {
				ang = 2.0 * PI * i / nSides;
				ogl.VertexBuffer () [i].v.coord.x = float (cos (ang));
				ogl.VertexBuffer () [i].v.coord.y = float (sin (ang));
				}
			ogl.FlushBuffers (nType, nSides, 2);
			}
		}
	}
ogl.SetLineSmooth (false);
glPopMatrix ();
}

//------------------------------------------------------------------------------

void OglDrawCircle (int nSides, int nType)
{
	int		i;
	double	ang;

if (ogl.SizeVertexBuffer (nSides)) {
	for (i = 0; i < nSides; i++) {
		ang = 2.0 * PI * i / nSides;
		ogl.VertexBuffer () [i].v.coord.x = float (cos (ang));
		ogl.VertexBuffer () [i].v.coord.y = float (sin (ang));
		}
	ogl.FlushBuffers (nType, nSides, 2);
	}
}

//------------------------------------------------------------------------------

int G3DrawSphere (CRenderPoint *pnt, fix rad, int bBigSphere)
{
	double r;

ogl.SetTexturing (false);
OglCanvasColor (&CCanvas::Current ()->Color ());
glPushMatrix ();
glTranslatef (X2F (pnt->ViewPos ().v.coord.x), X2F (pnt->ViewPos ().v.coord.y), X2F (pnt->ViewPos ().v.coord.z));
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
	ogl.SetBlending (false);
return 0;
}

//------------------------------------------------------------------------------

int G3DrawSphere3D (CRenderPoint *p0, int nSides, int rad)
{
	CCanvasColor	c = CCanvas::Current ()->Color ();
	CRenderPoint			p = *p0;
	int				i;
	CFloatVector	v;
	float				x, y, z, r;
	float				ang;

if (ogl.SizeVertexBuffer (nSides + 1)) {
	ogl.SetTexturing (false);
	OglCanvasColor (&CCanvas::Current ()->Color ());
	x = X2F (p.ViewPos ().v.coord.x);
	y = X2F (p.ViewPos ().v.coord.y);
	z = X2F (p.ViewPos ().v.coord.z);
	r = X2F (rad);
	v.v.coord.z = z;
	for (i = 0; i <= nSides; i++) {
		ang = 2.0f * float (PI * (i % nSides) / nSides);
		v.v.coord.x = x + float (cos (ang) * r);
		v.v.coord.y = y + float (sin (ang) * r);
		ogl.VertexBuffer () [i] = v;
		}
	ogl.FlushBuffers (GL_POLYGON, nSides + 1);
	if (c.rgb)
		ogl.SetBlending (false);
	}
return 1;
}

//------------------------------------------------------------------------------

int G3DrawCircle3D (CRenderPoint *p0, int nSides, int rad)
{
	CRenderPoint			p = *p0;
	int				i, j;
	CFloatVector	v;
	float				x, y, r;
	float				ang;

if (ogl.SizeVertexBuffer (2 * (nSides + 1))) {
	ogl.SetTexturing (false);
	OglCanvasColor (&CCanvas::Current ()->Color ());
	x = X2F (p.ViewPos ().v.coord.x);
	y = X2F (p.ViewPos ().v.coord.y);
	v.v.coord.z = X2F (p.ViewPos ().v.coord.z);
	r = X2F (rad);
	for (i = 0; i <= nSides; i++) {
		for (j = i; j <= i + 1; j++) {
			ang = 2.0f * (float) PI * (j % nSides) / nSides;
			v.v.coord.x = x + (float) cos (ang) * r;
			v.v.coord.y = y + (float) sin (ang) * r;
			ogl.VertexBuffer () [i] = v;
			}
		}
	ogl.FlushBuffers (GL_LINES, 2 * (nSides + 1), 2);
	if (CCanvas::Current ()->Color ().rgb)
		ogl.SetBlending (false);
	}
return 1;
}

//------------------------------------------------------------------------------

int G3DrawWhitePoly (int nVertices, CRenderPoint **pointList)
{
	int			i;
	CRenderPoint*	p;

if (ogl.SizeVertexBuffer (nVertices)) {
	ogl.SetTexturing (false);
	ogl.SetBlending (false);
	glColor4d (1.0, 1.0, 1.0, 1.0);
	for (i = 0; i < nVertices; i++) {
		p = pointList [i];
		if (p->Index () < 0)
			ogl.VertexBuffer () [i].Assign ((*pointList)->ViewPos ());
		else
			ogl.VertexBuffer () [i] = gameData.render.vertP [p->Index ()];
		}
	ogl.FlushBuffers (GL_TRIANGLE_FAN, nVertices);
	}
return 0;
}

//------------------------------------------------------------------------------

int G3DrawPoly (int nVertices, CRenderPoint **pointList)
{
	int				i;
	CRenderPoint*	p;

if (gameStates.render.nShadowBlurPass == 1) {
	G3DrawWhitePoly (nVertices, pointList);
	return 0;
	}
if (ogl.SizeVertexBuffer (nVertices)) {
	ogl.SetTexturing (false);
	OglCanvasColor (&CCanvas::Current ()->Color ());
	for (i = 0; i < nVertices; i++) {
		p = pointList [i];
		if (p->Index () < 0)
			ogl.VertexBuffer () [i].Assign ((*pointList)->ViewPos ());
		else
			ogl.VertexBuffer () [i] = gameData.render.vertP [p->Index ()];
		}
	ogl.FlushBuffers (GL_TRIANGLE_FAN, nVertices);
	if (CCanvas::Current ()->Color ().rgb || (gameStates.render.grAlpha < 1.0f))
		ogl.SetBlending (false);
	}
return 0;
}

//------------------------------------------------------------------------------

int G3DrawPolyAlpha (int nVertices, CRenderPoint **pointList, CFloatVector *color, char bDepthMask, short nSegment)
{
if (gameStates.render.nShadowBlurPass == 1) {
	G3DrawWhitePoly (nVertices, pointList);
	return 0;
	}
if (color->Alpha () < 0)
	color->Alpha () = gameStates.render.grAlpha;
CFloatVector	vertices [8];

for (int i = 0; i < nVertices; i++)
	vertices [i] = gameData.render.vertP [pointList [i]->Index ()];
transparencyRenderer.AddPoly (NULL, NULL, NULL, vertices, nVertices, NULL, color, NULL, 1, bDepthMask, GL_TRIANGLE_FAN, GL_REPEAT, 0, nSegment);
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

void DrawTexPolyFlat (CBitmap *bmP, int nVertices, CRenderPoint **vertlist)
{
#if TRACE
console.printf (CON_DBG, "DrawTexPolyFlat: unhandled\n");//should never get called
#endif
}

//------------------------------------------------------------------------------

int G3DrawTexPolyFlat (
	int			nVertices,
	CRenderPoint**	pointList,
	tUVL*			uvlList,
	tUVL*			uvlLMap,
	CBitmap*		bmBot,
	CBitmap*		bmTop,
	tLightmap*	lightmap,
	CFixVector*	vNormalP,
	int			orient,
	int			bBlend,
	int			bAdditive,
	short			nSegment)
{
	int			i;
	CRenderPoint*	p;

if (ogl.SizeVertexBuffer (nVertices)) {
	if (FAST_SHADOWS) {
		if (bBlend)
			ogl.SetBlendMode (OGL_BLEND_ALPHA);
		else
			ogl.SetBlending (false);
		}
	else {
		if (gameStates.render.nShadowPass == 3) {
			ogl.SetBlending (true);
			ogl.SetBlendMode (OGL_BLEND_ADD);
			}
		}
	ogl.SelectTMU (GL_TEXTURE0, true);
	ogl.SetTexturing (false);
	glColor4d (0, 0, 0, gameStates.render.grAlpha);
	for (i = 0; i < nVertices; i++) {
		p = pointList [i];
		if (p->Index () < 0)
			ogl.VertexBuffer () [i].Assign ((*pointList)->ViewPos ());
		else
			ogl.VertexBuffer () [i] = gameData.render.vertP [p->Index ()];
		}
	ogl.FlushBuffers (GL_TRIANGLE_FAN, nVertices);
	}
return 0;
}

//------------------------------------------------------------------------------

#define	G3VERTPOS(_dest,_src) \
			if ((_src)->Index () < 0) \
				(_dest).Assign ((_src)->ViewPos ()); \
			else \
				_dest = gameData.render.vertP [(_src)->Index ()];

#define	G3VERTPOS3(_dest,_src) \
			if ((_src)->Index () < 0) \
				(_dest).Assign ((_src)->ViewPos ()); \
			else \
				_dest = gameData.render.vertP [(_src)->Index ()];

//------------------------------------------------------------------------------

int G3DrawTexPolyMulti (
	int			nVertices,
	CRenderPoint**	pointList,
	tUVL*			uvlList,
	tUVL*			uvlLMap,
	CBitmap*		bmBot,
	CBitmap*		bmTop,
	tLightmap*	lightmap,
	CFixVector*	vNormalP,
	int			orient,
	int			bBlend,
	int			bAdditive,
	short			nSegment)
{
	int				i, nShader, nFrame;
	int				bShaderMerge = 0,
						bSuperTransp = 0;
	int				bLight = 1,
						bDynLight = gameStates.render.bApplyDynLight && (gameStates.app.bEndLevelSequence < EL_OUTSIDE),
						bDepthSort,
						bOverlay = 0;
	CFaceColor*		colorP;
	CBitmap*			bmP = NULL, *mask = NULL;
	CRenderPoint*	pointP, **pointPP;
#if USE_VERTNORMS
	CFloatVector	vNormal, vVertPos;
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
		ogl.SetBlending (false);
#if 0
	else
		ogl.SetBlendMode (OGL_BLEND_ALPHA);
#endif
	}
else {
	if (gameStates.render.nShadowPass == 1)
		bLight = !bDynLight;
	else if (gameStates.render.nShadowPass == 3) {
		ogl.SetBlending (true);
		ogl.SetBlendMode (OGL_BLEND_ADD);
		}
	}
ogl.SetDepthMode (GL_LEQUAL);
bmBot = bmBot->Override (-1);
bDepthSort = (!bmTop && 
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
	bShaderMerge = bSuperTransp && gameOpts->ogl.bGlTexMerge;
	bOverlay = !bShaderMerge;
	}
else
	bOverlay = 0;

#if G3_DRAW_ARRAYS
retry:
#endif
if (bShaderMerge) {
	mask = gameStates.render.textures.bHaveMaskShader ? bmTop->Mask () : NULL;
	nShader = bSuperTransp ? mask ? 2 : 1 : 0;
	activeShaderProg = GLhandleARB (abs (int (shaderManager.Deploy (tmShaderProgs [nShader]))));
	INIT_TMU (InitTMU0, GL_TEXTURE0, bmBot, lightmapManager.Buffer (), 1, 0);
	glUniform1i (glGetUniformLocation (activeShaderProg, "btmTex"), 0);
	INIT_TMU (InitTMU1, GL_TEXTURE1, bmTop, lightmapManager.Buffer (), 1, 0);
	glUniform1i (glGetUniformLocation (activeShaderProg, "topTex"), 1);
	if (mask) {
#if DBG
		InitTMU2 (1);
		G3_BIND (GL_TEXTURE2, mask, lightmapManager.Buffer (), 1);
#else
		INIT_TMU (InitTMU2, GL_TEXTURE2, mask, lightmapManager.Buffer (), 1, 0);
#endif
		glUniform1i (glGetUniformLocation (activeShaderProg, "maskTex"), 2);
		}
	glUniform1f (glGetUniformLocation (activeShaderProg, "grAlpha"), gameStates.render.grAlpha);
	}
else if (!bDepthSort) {
	if (bmBot == gameData.endLevel.satellite.bmP) {
		ogl.SelectTMU (GL_TEXTURE0, true);
		ogl.SetTexturing (true);
		}
	else
		InitTMU0 (1);
	if (bmBot->Bind (1))
		return 1;
	bmBot = bmBot->CurFrame (-1);
	bmBot->Texture ()->Wrap ((bmBot == deadzone.Bitmap ()) ? GL_CLAMP : GL_REPEAT);
	}

if (!bDepthSort) {
	if (SHOW_DYN_LIGHT) {
#if USE_VERTNORMS
		if (vNormalP) {
			vNormal.Assign (*vNormalP);
			transformation.Rotate (vNormal, vNormal, 0);
			}
	else
			G3CalcNormal (pointList, &vNormal);
#else
			G3Normal (pointList, vNormalP);
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
	ogl.m_states.bDynObjLight = bDynLight;
	}

ogl.m_states.fAlpha = gameStates.render.grAlpha;
if (bDepthSort) {
		CFloatVector	vertices [8];
		CFaceColor		vertColors [8];
		tTexCoord2f		texCoord [2][8];
		int				vertIndex [8];
		//int				colorIndex [8];

	for (i = 0, pointPP = pointList; i < nVertices; i++, pointPP++) {
		pointP = *pointPP;
		vertIndex [i] = pointP->Index ();
		//colorIndex [i] = i;
		if (pointP->Index () < 0)
			vertices[i].Assign (pointP->ViewPos ());
		else
			vertices [i] = gameData.render.vertP [pointP->Index ()];
		texCoord [0][i].v.u = X2F (uvlList [i].u);
		texCoord [0][i].v.v = X2F (uvlList [i].v);
		SetTexCoord (uvlList + i, orient, 1, texCoord [1] + i, 0);
		G3VERTPOS (vVertPos, pointP);
		if (bDynLight)
			GetVertexColor (-1, -1, vertIndex [i], pointP->GetNormal ()->XYZ (), vVertPos.XYZ (), vertColors + i, NULL,
								gameStates.render.nState ? X2F (uvlList [i].l) : 1, -1, 0);
		else if (bLight)
			SetTMapColor (uvlList + i, i, bmBot, !bOverlay, &vertColors [i]);
		}
	bmBot->SetupTexture (1, 0);
	transparencyRenderer.AddPoly (NULL, NULL, bmBot, vertices, nVertices, texCoord [0], NULL, vertColors, nVertices, 1, GL_TRIANGLE_FAN, GL_REPEAT, bAdditive, nSegment);
	return 0;
	}


if (!ogl.SizeBuffers (nVertices))
	return 0;

CFaceColor	faceColor;

if (bDynLight) {
	if (bShaderMerge) {
		for (i = 0, pointPP = pointList; i < nVertices; i++, pointPP++) {
			pointP = *pointPP;
			G3VERTPOS (vVertPos, pointP);
			GetVertexColor (-1, -1, pointP->Index (), pointP->GetNormal ()->XYZ (), vVertPos.XYZ (), &faceColor, NULL, 1, 0, 0);
			ogl.ColorBuffer () [i].Assign (faceColor);
			ogl.TexCoordBuffer () [i].v.u = X2F (uvlList [i].u);
			ogl.TexCoordBuffer () [i].v.v = X2F (uvlList [i].v);
			SetTexCoord (uvlList + i, orient, 1, ogl.TexCoordBuffer (1) + i, mask != NULL);
			ogl.VertexBuffer () [i] = vVertPos;
			}
		}
	else {
		for (i = 0, pointPP = pointList; i < nVertices; i++, pointPP++) {
			pointP = *pointPP;
			G3VERTPOS (vVertPos, pointP);
			GetVertexColor (-1, -1, pointP->Index (), pointP->GetNormal ()->XYZ (), vVertPos.XYZ (), &faceColor, NULL,
								gameStates.render.nState ? X2F (uvlList [i].l) : 1, 0, 0);
			ogl.ColorBuffer () [i].Assign (faceColor);
			ogl.TexCoordBuffer () [i].v.u = X2F (uvlList [i].u);
			ogl.TexCoordBuffer () [i].v.v = X2F (uvlList [i].v);
			ogl.VertexBuffer () [i] = vVertPos;
			}
		}
	}
else if (bLight) {
	if (bShaderMerge) {
		for (i = 0, pointPP = pointList; i < nVertices; i++, pointPP++) {
			if (gameStates.render.nState)
				SetTMapColor (uvlList + i, i, bmBot, 1, ogl.ColorBuffer () + i);
			else {
				colorP = gameData.render.color.vertices + (*pointPP)->Index ();
				ogl.ColorBuffer () [i].Assign (*colorP);
				}
			ogl.TexCoordBuffer () [i].v.u = X2F (uvlList [i].u);
			ogl.TexCoordBuffer () [i].v.v = X2F (uvlList [i].v);
			SetTexCoord (uvlList + i, orient, 1, ogl.TexCoordBuffer (1) + i, mask != NULL);
			OglVertex3f (*pointPP, ogl.VertexBuffer () + i);
			}
		}
	else {
		for (i = 0, pointPP = pointList; i < nVertices; i++, pointPP++) {
			if (gameStates.render.nState || gameStates.app.bEndLevelSequence)
				SetTMapColor (uvlList + i, i, bmBot, 1, ogl.ColorBuffer () + i);
			else {
				colorP = gameData.render.color.vertices + (*pointPP)->Index ();
				glColor3fv (reinterpret_cast<GLfloat*> (colorP));
				}
			ogl.TexCoordBuffer () [i].v.u = X2F (uvlList [i].u);
			ogl.TexCoordBuffer () [i].v.v = X2F (uvlList [i].v);
			OglVertex3f (*pointPP, ogl.VertexBuffer () + i);
			}
		}
	}
else {
	if (bShaderMerge) {
		for (i = 0, pointPP = pointList; i < nVertices; i++, pointPP++) {
			ogl.TexCoordBuffer () [i].v.u = X2F (uvlList [i].u);
			ogl.TexCoordBuffer () [i].v.v = X2F (uvlList [i].v);
			SetTexCoord (uvlList + i, orient, 1, ogl.TexCoordBuffer (1) + i, mask != NULL);
			OglVertex3f (*pointPP, ogl.VertexBuffer () + i);
			}
		}
	else {
		for (i = 0, pointPP = pointList; i < nVertices; i++, pointPP++) {
			ogl.TexCoordBuffer (1) [i].v.u = X2F (uvlList [i].u);
			ogl.TexCoordBuffer (1) [i].v.v = X2F (uvlList [i].v);
			OglVertex3f (*pointPP, ogl.VertexBuffer () + i);
			}
		}
	}
if (bShaderMerge) {
	ogl.EnableClientStates (1, 0, 0, GL_TEXTURE1);
	OglTexCoordPointer (2, GL_FLOAT, 0, ogl.TexCoordBuffer (1).Buffer ());
	}
ogl.EnableClientStates (1, 1, 0, GL_TEXTURE0);
ogl.FlushBuffers (GL_TRIANGLE_FAN, nVertices, 3, 1, bDynLight || bLight);
if (bShaderMerge)
	ogl.DisableClientStates (1, 0, 0, GL_TEXTURE1);

if (bOverlay) {
	ogl.SelectTMU (GL_TEXTURE0, true);
	ogl.SetTexturing (true);
	if (bmTop->Bind (1))
		return 1;
	bmTop = bmTop->CurFrame (-1);
	bmTop->Texture ()->Wrap (GL_REPEAT);

	if (bDynLight) {
		for (i = 0, pointPP = pointList; i < nVertices; i++, pointPP++) {
			CRenderPoint* pointP = *pointPP;
			vVertPos.Assign (pointP->ViewPos ());
			GetVertexColor (-1, -1, pointP->Index (), pointP->GetNormal ()->XYZ (), vVertPos.XYZ (), &faceColor, NULL, 1, 0, 0);
			ogl.ColorBuffer () [i].Assign (faceColor);
			SetTexCoord (uvlList + i, orient, 0, ogl.TexCoordBuffer () + i, mask != NULL);
			OglVertex3f (*pointPP, ogl.VertexBuffer () + i);
			}
		}
	else if (bLight) {
		for (i = 0, pointPP = pointList; i < nVertices; i++, pointPP++) {
			SetTMapColor (uvlList + i, i, bmTop, 1, ogl.ColorBuffer () + i);
			SetTexCoord (uvlList + i, orient, 0, ogl.TexCoordBuffer () + i, mask != NULL);
			OglVertex3f (*pointPP, ogl.VertexBuffer () + i);
			}
		}
	else {
		for (i = 0, pointPP = pointList; i < nVertices; i++, pointPP++) {
			SetTexCoord (uvlList + i, orient, 0, ogl.TexCoordBuffer () + i, mask != NULL);
			OglVertex3f (*pointPP, ogl.VertexBuffer () + i);
			}
		}
	ogl.FlushBuffers (GL_TRIANGLE_FAN, nVertices, 3, 1, bDynLight || bLight);

	ogl.SetDepthMode (GL_LEQUAL);
#if OGL_CLEANUP
	ogl.BindTexture (0);
	ogl.SetTexturing (false);
#endif
	}
else if (bShaderMerge) {
#if OGL_CLEANUP
	ogl.SelectTMU (GL_TEXTURE1, true);
	ogl.BindTexture (0);
	ogl.SetTexturing (false); // Disable the 2nd texture
#endif
	activeShaderProg = 0;
	shaderManager.Deploy (-1);
	}
ogl.SelectTMU (GL_TEXTURE0, true);
ogl.BindTexture (0);
ogl.SetTexturing (false);
tMapColor.index =
lightColor.index = 0;
if (!bBlend)
	ogl.SetBlending (true);
return 0;
}

//------------------------------------------------------------------------------

int G3DrawTexPolyLightmap (
	int			nVertices,
	CRenderPoint**	pointList,
	tUVL*			uvlList,
	tUVL*			uvlLMap,
	CBitmap*		bmBot,
	CBitmap*		bmTop,
	tLightmap*	lightmap,
	CFixVector*	vNormalP,
	int			orient,
	int			bBlend,
	short			nSegment)
{
	int				i, bShaderMerge;
	CRenderPoint**	pointPP;

if (gameStates.render.nShadowBlurPass == 1) {
	G3DrawWhitePoly (nVertices, pointList);
	return 0;
	}
if (!bmBot)
	return 1;
r_tpolyc++;
//if (gameStates.render.nShadowPass != 3)
	ogl.SetDepthMode (GL_LEQUAL);
if (FAST_SHADOWS) {
	if (bBlend)
		ogl.SetBlendMode (OGL_BLEND_ALPHA);
	else
		ogl.SetBlending (false);
	}
else {
	if (gameStates.render.nShadowPass == 3) {
		ogl.SetBlending (true);
		ogl.SetBlendMode (OGL_BLEND_ADD);
		}
	}
ogl.SetDepthMode (GL_LEQUAL);
bmBot = bmBot->Override (-1);
if (bmTop && (bmTop = bmTop->Override (-1)) && bmTop->Frames ()) {
	bmTop = bmTop->CurFrame ();
	}
if (!lightmap) //lightmapping enabled
	return G3DrawTexPolyMulti (nVertices, pointList, uvlList, uvlLMap, bmBot, bmTop, lightmap, vNormalP, orient, bBlend, 0, nSegment);
// chose shaders depending on whether overlay bitmap present or not
if ((bShaderMerge = bmTop && gameOpts->ogl.bGlTexMerge)) {
	lmProg = lmShaderProgs [(bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT) != 0];
	glUseProgramObjectARB (lmProg);
	}
InitTMU0 (1);	// use render pipeline 0 for bottom texture
if (bmBot->Bind (1))
	return 1;
bmBot = bmBot->CurFrame (-1);
bmBot->Texture ()->Wrap (GL_REPEAT);
if (bShaderMerge)
	glUniform1i (glGetUniformLocation (lmProg, "btmTex"), 0);
if (bmTop) { // use render pipeline 1 for overlay texture
	InitTMU1 (1);
	if (bmTop->Bind (1))
		return 1;
	bmTop = bmTop->CurFrame (-1);
	bmTop->Texture ()->Wrap (GL_REPEAT);
	glUniform1i (glGetUniformLocation (lmProg, "topTex"), 1);
	}
// use render pipeline 2 for lightmap texture
if (ogl.SizeBuffers (nVertices)) {
	InitTMU2 (1);
	//ogl.BindTexture (lightmap->handle);
	if (bShaderMerge)
		glUniform1i (glGetUniformLocation (lmProg, "lMapTex"), 2);
	pointPP = pointList;
	if (gameStates.render.bFullBright)
		glColor3d (1,1,1);
	for (i = 0; i < nVertices; i++, pointPP++) {
		if (!gameStates.render.bFullBright)
			SetTMapColor (uvlList + i, i, bmBot, 1, ogl.ColorBuffer () + i);
		ogl.TexCoordBuffer () [i].v.u = X2F (uvlList [i].u);
		ogl.TexCoordBuffer () [i].v.v = X2F (uvlList [i].v);
		if (bmTop)
			SetTexCoord (uvlList + i, orient, 1, ogl.TexCoordBuffer (1) + i, 0);
		glMultiTexCoord2f (GL_TEXTURE2, X2F (uvlLMap [i].u), X2F (uvlLMap [i].v));
		OglVertex3f (*pointPP, ogl.VertexBuffer () + i);
		}
	ogl.EnableClientStates (1, 1, 0, 1);
	OglColorPointer (4, GL_FLOAT, 0, &ogl.ColorBuffer ());
	OglTexCoordPointer (2, GL_FLOAT, 0, ogl.TexCoordBuffer (1).Buffer ());
	ogl.EnableClientStates (1, 0, 0, 2);
	OglTexCoordPointer (2, GL_FLOAT, 0, &ogl.TexCoordBuffer ());
	ogl.FlushBuffers (GL_TRIANGLE_FAN, nVertices, 3, 1, !gameStates.render.bFullBright);
	ogl.DisableClientStates (1, 0, 0, 2);
	ogl.DisableClientStates (1, 1, 0, 1);
	ogl.SelectTMU (GL_TEXTURE0, true);
	}
ExitTMU (0);
if (bShaderMerge)
	glUseProgramObjectARB (lmProg = 0);
return 0;
}

//------------------------------------------------------------------------------

int G3DrawTexPolySimple (
	int				nVertices,
	CRenderPoint**	pointList,
	tUVL*				uvlList,
	CBitmap*			bmP,
	CFixVector*		vNormalP,
	int				bBlend)
{
	int				i;
	int				bLight = 1,
						bDynLight = gameStates.render.bApplyDynLight && !gameStates.app.bEndLevelSequence;
	CRenderPoint*	pointP, ** pointPP;
#if USE_VERTNORMS
	CFloatVector	vNormal, vVertPos;
#endif

if (gameStates.render.nShadowBlurPass == 1) {
	G3DrawWhitePoly (nVertices, pointList);
	return 0;
	}
r_tpolyc++;
if (FAST_SHADOWS) {
	if (!bBlend)
		ogl.SetBlending (false);
#if 0
	else
		ogl.SetBlendMode (OGL_BLEND_ALPHA);
#endif
	}
else {
	if (gameStates.render.nShadowPass == 1)
		bLight = !bDynLight;
	else if (gameStates.render.nShadowPass == 3) {
		ogl.SetBlending (true);
		ogl.SetBlendMode (OGL_BLEND_ADD);
		}
	}
ogl.SetDepthMode (GL_LEQUAL);
bmP = bmP->Override (-1);
if (bmP == gameData.endLevel.satellite.bmP) {
	ogl.SelectTMU (GL_TEXTURE0, true);
	ogl.SetTexturing (true);
	}
else
	InitTMU0 (1);
if (bmP->Bind (1))
	return 1;
if (bmP == deadzone.Bitmap ())
	bmP->Texture ()->Wrap (GL_CLAMP);
else
	bmP->Texture ()->Wrap (GL_REPEAT);

if (SHOW_DYN_LIGHT) {
#if USE_VERTNORMS
	if (vNormalP)
		vNormal.Assign (*vNormalP);
else
		G3CalcNormal (pointList, &vNormal);
#else
		G3Normal (pointList, vNormalP);
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
ogl.m_states.bDynObjLight = bDynLight;
ogl.m_states.fAlpha = gameStates.render.grAlpha;
if (ogl.SizeBuffers (nVertices)) {
	if (bDynLight) {
		CFaceColor	faceColor;
		for (i = 0, pointPP = pointList; i < nVertices; i++, pointPP++) {
			pointP = *pointPP;
			G3VERTPOS (vVertPos, pointP);
			GetVertexColor (-1, -1, pointP->Index (), pointP->GetNormal ()->XYZ (), vVertPos.XYZ (), NULL, &faceColor, 1, 0, 0);
			ogl.ColorBuffer () [i].Assign (faceColor);
			ogl.TexCoordBuffer () [i].v.u = X2F (uvlList [i].u);
			ogl.TexCoordBuffer () [i].v.v = X2F (uvlList [i].v);
			ogl.VertexBuffer () [i] = vVertPos;
			}
		}
	else if (bLight) {
		for (i = 0, pointPP = pointList; i < nVertices; i++, pointPP++) {
			SetTMapColor (uvlList + i, i, bmP, 1, ogl.ColorBuffer () + i);
			ogl.TexCoordBuffer () [i].v.u = X2F (uvlList [i].u);
			ogl.TexCoordBuffer () [i].v.v = X2F (uvlList [i].v);
			OglVertex3f (*pointPP, ogl.VertexBuffer () + i);
			}
		}
	else {
		for (i = 0, pointPP = pointList; i < nVertices; i++, pointPP++) {
			ogl.TexCoordBuffer () [i].v.u = X2F (uvlList [i].u);
			ogl.TexCoordBuffer () [i].v.v = X2F (uvlList [i].v);
			OglVertex3f (*pointPP, ogl.VertexBuffer () + i);
			}
		}
	ogl.FlushBuffers (GL_TRIANGLE_FAN, nVertices, 3, 1, bLight || bDynLight);
	}
ogl.SetTexturing (false);
tMapColor.index =
lightColor.index = 0;
if (!bBlend)
	ogl.SetBlending (true);
return 0;
}

//------------------------------------------------------------------------------

int COGL::BindBitmap (CBitmap* bmP, int nFrame, int nWrap, int bTextured)
{
if (bmP) {
	SelectTMU (GL_TEXTURE0);
	SetTexturing (true);
	if (!bmP->IsBound ()) {
		if (bmP->Bind (1))
			return 0;
		bmP = bmP->Override (-1);
		if (bmP->Frames ())
			bmP = bmP->Frames () + nFrame;
		bmP->Texture ()->Wrap (nWrap);
		}
	}
else if (!(bTextured || (ogl.DrawBuffer () && ogl.DrawBuffer ()->IsBound ())))
	SetTexturing (false);
return 1;
}

//------------------------------------------------------------------------------

int COGL::BindBuffers (CFloatVector *vertexP, int nVertices, int nDimensions,
							  tTexCoord2f *texCoordP, 
							  CFloatVector *colorP, int nColors,
							  CBitmap *bmP,
							  int nTMU)
{
if (!ogl.EnableClientStates (m_data.bClientTexCoord = texCoordP != NULL, m_data.bClientColor = ((colorP != NULL) && (nColors == nVertices)), 0, nTMU))
	return 0;
if (texCoordP)
	OglTexCoordPointer (2, GL_FLOAT, sizeof (tTexCoord2f), texCoordP);
if (colorP) {
	if (nColors == nVertices)
		OglColorPointer (4, GL_FLOAT, sizeof (CFloatVector), colorP);
	else
		glColor4fv (reinterpret_cast<GLfloat*> (colorP));
	}
OglVertexPointer (nDimensions, GL_FLOAT, sizeof (CFloatVector), vertexP);
return 1;
}

//------------------------------------------------------------------------------

void COGL::ReleaseBuffers (void)
{
ogl.DisableClientStates (m_data.bClientTexCoord, m_data.bClientColor, 0);
}

//------------------------------------------------------------------------------

int COGL::RenderArrays (int nPrimitive, 
							   CFloatVector *vertexP, int nVertices, int nDimensions,
							   tTexCoord2f *texCoordP, 
							   CFloatVector *colorP, int nColors, 
							   CBitmap *bmP, int nFrame, int nWrap)
{
if (!BindBitmap (bmP, nFrame, nWrap, texCoordP != NULL))
	return 0;
if (!BindBuffers (vertexP, nVertices, nDimensions, texCoordP, colorP, nColors, bmP))
	return 0;
OglDrawArrays (nPrimitive, 0, nVertices);
ogl.ReleaseBuffers ();
return 1;
}

//------------------------------------------------------------------------------

int COGL::RenderQuad (CBitmap* bmP, CFloatVector* vertexP, int nDimensions, tTexCoord2f* texCoordP, CFloatVector* colorP, int nColors, int nWrap)
{
if (!bmP)
	RenderArrays (GL_QUADS, vertexP, 4, nDimensions, texCoordP, colorP, nColors, bmP, 0, GL_CLAMP);
else if (texCoordP)
	RenderArrays (GL_QUADS, vertexP, 4, nDimensions, texCoordP, colorP, nColors, bmP, 0, nWrap);
else {
	if (!(bmP->Texture () || BindBitmap (bmP, 0, nWrap, true)))
		return 0;
#if DBG
	if (!bmP->Texture ()) {
		BindBitmap (bmP, 0, nWrap, true);
		return 0;
		}
#endif
	GLfloat			u = bmP->Texture ()->U ();
	GLfloat			v = bmP->Texture ()->V ();
	tTexCoord2f		texCoords [4] = {{{0,0}},{{u,0}},{{u,v}},{{0,v}}};

	RenderArrays (GL_QUADS, vertexP, 4, nDimensions, texCoords, colorP, nColors, bmP, 0, nWrap);
	}
return 0;
}

//------------------------------------------------------------------------------

int COGL::RenderQuad (CBitmap* bmP, CFloatVector& vPosf, float width, float height, int nDimensions, int nWrap)
{
CFloatVector verts [4];
verts [0].v.coord.x =
verts [3].v.coord.x = vPosf.v.coord.x - width;
verts [1].v.coord.x =
verts [2].v.coord.x = vPosf.v.coord.x + width;
verts [0].v.coord.y =
verts [1].v.coord.y = vPosf.v.coord.y + height;
verts [2].v.coord.y =
verts [3].v.coord.y = vPosf.v.coord.y - height;
if (nDimensions == 3)
	verts [0].v.coord.z =
	verts [1].v.coord.z =
	verts [2].v.coord.z =
	verts [3].v.coord.z = vPosf.v.coord.z;
int nColors = 0;
CFloatVector* colorP = bmP ? bmP->GetColor (&nColors) : NULL;
return RenderQuad (bmP, verts, nDimensions, bmP ?  bmP->GetTexCoord () : NULL, colorP, nColors, nWrap);
}

//------------------------------------------------------------------------------

int COGL::RenderBitmap (CBitmap* bmP, const CFixVector& vPos, fix xWidth, fix xHeight, CFloatVector* colorP, float alpha, int bAdditive)
{
	CFloatVector	vPosf;
	CFloatVector		color = {1, 1, 1, alpha};

SelectTMU (GL_TEXTURE0);
SetBlendMode (bAdditive);
vPosf.Assign (vPos);
transformation.Transform (vPosf, vPosf, 0);
if (gameStates.render.nShadowBlurPass == 1)
	RenderQuad (NULL, vPosf, X2F (xWidth), X2F (xHeight), 2);
else {
	bmP->SetColor (colorP ? colorP : &color);
	RenderQuad (bmP, vPosf, X2F (xWidth), X2F (xHeight));
	}
SetBlendMode (OGL_BLEND_ALPHA);
return 0;
}

//------------------------------------------------------------------------------

int COGL::RenderSprite (CBitmap* bmP, const CFixVector& vPos,
								fix xWidth,	fix xHeight,
								float alpha,
								int bAdditive, 
								float fSoftRad)
{
	CFixVector		pv, v1;
	CFloatVector*	colorP = bmP->GetColor ();

if (alpha < 1.0f) {
	CFloatVector	color;
	if (!colorP) {
		color.Red () =
		color.Green () =
		color.Blue () = 1;
		color.Alpha () = alpha;
		colorP = &color;
		}
	transparencyRenderer.AddSprite (bmP, vPos, colorP, xWidth, xHeight, 0, bAdditive, fSoftRad);
	}
else {
	ogl.SelectTMU (GL_TEXTURE0, true);
	v1 = vPos - transformation.m_info.pos;
	pv = transformation.m_info.view [0] * v1;
	CFloatVector vPosf;
	vPosf.Assign (pv);
	if (gameStates.render.nShadowBlurPass == 1) {
		glColor4f (1,1,1,1);
		ogl.RenderQuad (NULL, vPosf, X2F (xWidth), X2F (xHeight), 3);
		}
	else {
		if (!colorP)
			glColor4f (1, 1, 1, alpha);
		SetBlendMode (abs (bAdditive));
		ogl.SetDepthWrite (false);
		ogl.RenderQuad (bmP, vPosf, X2F (xWidth), X2F (xHeight), 3);
		ogl.SetDepthWrite (true);
		SetBlendMode (OGL_BLEND_ALPHA);
		}
	}
return 0;
}

//------------------------------------------------------------------------------

void COGL::RenderScreenQuad (GLuint nTexture)
{
	static tTexCoord2f texCoord [4] = {{{0,0}},{{0,1}},{{1,1}},{{1,0}}};
	static float verts [4][2] = {{0,0},{0,1},{1,1},{1,0}};

EnableClientStates (int (nTexture != 0), 0, 0, GL_TEXTURE0);
BindTexture (nTexture);
if (nTexture)
	OglTexCoordPointer (2, GL_FLOAT, 0, texCoord);
OglVertexPointer (2, GL_FLOAT, 0, verts);
OglDrawArrays (GL_QUADS, 0, 4);
}

//------------------------------------------------------------------------------

bool COglBuffers::SizeVertices (int nVerts)
{
if (int (vertices.Length ()) >= nVerts)
	return true;
vertices.Destroy ();
return vertices.Create (nVerts) != NULL;
}

bool COglBuffers::SizeColor (int nVerts)
{
if (int (color.Length ()) >= nVerts)
	return true;
color.Destroy ();
return color.Create (nVerts) != NULL;
}

bool COglBuffers::SizeTexCoord (int nVerts)
{
if ((int (texCoord [0].Length ()) >= nVerts) && (int (texCoord [1].Length ()) >= nVerts))
	return true;
texCoord [0].Destroy ();
texCoord [1].Destroy ();
return ((texCoord [0].Create (nVerts) != NULL) && (texCoord [1].Create (nVerts) != NULL));
}

bool COglBuffers::SizeIndex (int nVerts)
{
if (int (indices.Length ()) >= nVerts)
	return true;
indices.Destroy ();
return indices.Create (nVerts) != NULL;
}

bool COglBuffers::SizeBuffers (int nVerts)
{
return SizeVertices (nVerts) && SizeColor (nVerts) && SizeTexCoord (nVerts) && SizeIndex (nVerts);
}


//------------------------------------------------------------------------------

void COglBuffers::Flush (GLenum nPrimitive, int nVerts, int nDimensions, int bTextured, int bColored)
{
if (nVerts > 0)
	m_nVertices = nVerts;
if (vertices.Buffer () && m_nVertices) {
	if (bTextured && texCoord [0].Buffer ()) {
		ogl.EnableClientState (GL_TEXTURE_COORD_ARRAY);
		OglTexCoordPointer (2, GL_FLOAT, 0, texCoord [0].Buffer ());
		}
	if (bColored && color.Buffer ()) {
		ogl.EnableClientState (GL_COLOR_ARRAY);
		OglColorPointer (4, GL_FLOAT, 0, color.Buffer ());
		}
	ogl.EnableClientState (GL_VERTEX_ARRAY);
	OglVertexPointer (nDimensions, GL_FLOAT, sizeof (CFloatVector), vertices.Buffer ());
	OglDrawArrays (nPrimitive, 0, m_nVertices);
	ogl.DisableClientStates (bTextured, bColored, 0);
	}
}

//------------------------------------------------------------------------------
