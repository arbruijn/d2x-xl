//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_RENDER_H
#define _OGL_RENDER_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

#include "ogl_defs.h"
#include "ogl_lib.h"
#include "lightmap.h"

void OglDrawEllipse (int nSides, int nType, float xsc, float xo, float ysc, float yo, tSinCosf *sinCosP);
void OglDrawCircle (int nSides, int nType);
int G3DrawWhitePoly (int nv, CRenderPoint **pointList);
int G3DrawPolyAlpha (int nv, CRenderPoint **pointlist, CFloatVector *color, char bDepthMask, short nSegment);
void G3FlushFaceBuffer (int bForce);

int G3DrawTexPolyMulti (
	int			nVerts, 
	CRenderPoint		**pointList, 
	tUVL			*uvlList, 
	tUVL			*uvlLMap, 
	CBitmap		*bmBot, 
	CBitmap		*bmTop, 
	tLightmap	*lightmap, 
	CFixVector	*pvNormal,
	int			orient, 
	int			bBlend,
	int			bAdditive,
	short			nSegment);

int G3DrawTexPolyLightmap (
	int			nVerts, 
	CRenderPoint		**pointList, 
	tUVL			*uvlList, 
	tUVL			*uvlLMap, 
	CBitmap		*bmBot, 
	CBitmap		*bmTop, 
	tLightmap	*lightmap, 
	CFixVector	*pvNormal,
	int			orient, 
	int			bBlend,
	short			nSegment);

int G3DrawTexPolyFlat (
	int			nVerts, 
	CRenderPoint		**pointList, 
	tUVL			*uvlList, 
	tUVL			*uvlLMap, 
	CBitmap		*bmBot, 
	CBitmap		*bmTop, 
	tLightmap	*lightmap, 
	CFixVector	*pvNormal,
	int			orient, 
	int			bBlend,
	int			bAdditive,
	short			nSegment);

int G3DrawTexPolySimple (
	int			nVertices, 
	CRenderPoint		**pointList, 
	tUVL			*uvlList, 
	CBitmap		*bmP, 
	CFixVector	*pvNormal,
	int			bBlend);

void OglCachePolyModelTextures (int nModel);

void DrawTexPolyFlat (CBitmap *bm,int nv,CRenderPoint **vertlist);

void OglDrawFilledPoly (int* x, int* y, int nVerts, CCanvasColor *colorP = NULL, int nColors = 1);
void OglDrawFilledRect (int left,int top, int right,int bot, CCanvasColor* colorP = NULL);
void OglDrawPixel (int x, int y, CCanvasColor* colorP = NULL);
void OglDrawLine (int left,int top, int right,int bot, CCanvasColor* colorP = NULL);
void OglDrawEmptyRect (int left, int top, int right, int bot, CCanvasColor* colorP = NULL);

void InitGrayScaleShader (void);

//------------------------------------------------------------------------------

typedef	int tTexPolyMultiDrawer (int, CRenderPoint **, tUVL *, tUVL *, CBitmap *, CBitmap *, tLightmap *, CFixVector *, int, int, int, short);

extern tTexPolyMultiDrawer	*fpDrawTexPolyMulti;

//------------------------------------------------------------------------------

extern GLhandleARB	activeShaderProg;

//------------------------------------------------------------------------------

static inline int G3DrawTexPoly (int nVerts, CRenderPoint **points, tUVL *uvls,
											CBitmap *bmP, CFixVector *pvNormal, int bBlend, int bAdditive, short nSegment)
{
return fpDrawTexPolyMulti (nVerts, points, uvls, NULL, bmP, NULL, NULL, pvNormal, 0, bBlend, bAdditive, nSegment);
}

//------------------------------------------------------------------------------

#endif
