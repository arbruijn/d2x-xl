//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_RENDER_H
#define _OGL_RENDER_H

#ifdef _WIN32
#	pragma pack(push)
#	pragma pack(8)
#	include <windows.h>
#	pragma pack(pop)
#	include <stddef.h>
#endif

#include "ogl_defs.h"
#include "ogl_lib.h"
#include "lightmap.h"

void OglDrawEllipse (int32_t nSides, int32_t nType, float xsc, float xo, float ysc, float yo, tSinCosf *pSinCos);
void OglDrawCircle (int32_t nSides, int32_t nType);
void OglDrawCircle (CFloatVector vCenter, int32_t nSides, int32_t nType);
int32_t G3DrawWhitePoly (int32_t nv, CRenderPoint **pointList);
int32_t G3DrawPolyAlpha (int32_t nv, CRenderPoint **pointlist, CFloatVector *color, char bDepthMask, int16_t nSegment);
void G3FlushFaceBuffer (int32_t bForce);

int32_t G3DrawTexPolyMulti (
	int32_t			nVerts, 
	CRenderPoint	**pointList, 
	tUVL				*uvlList, 
	tUVL				*uvlLMap, 
	CBitmap			*bmBot, 
	CBitmap			*bmTop, 
	tLightmap		*lightmap, 
	CFixVector		*pvNormal,
	int32_t			orient, 
	int32_t			bBlend,
	int32_t			bAdditive,
	int16_t			nSegment);

int32_t G3DrawTexPolyLightmap (
	int32_t			nVerts, 
	CRenderPoint	**pointList, 
	tUVL				*uvlList, 
	tUVL				*uvlLMap, 
	CBitmap			*bmBot, 
	CBitmap			*bmTop, 
	tLightmap		*lightmap, 
	CFixVector		*pvNormal,
	int32_t			orient, 
	int32_t			bBlend,
	int16_t			nSegment);

int32_t G3DrawTexPolyFlat (
	int32_t			nVerts, 
	CRenderPoint	**pointList, 
	tUVL				*uvlList, 
	tUVL				*uvlLMap, 
	CBitmap			*bmBot, 
	CBitmap			*bmTop, 
	tLightmap		*lightmap, 
	CFixVector		*pvNormal,
	int32_t			orient, 
	int32_t			bBlend,
	int32_t			bAdditive,
	int16_t			nSegment);

int32_t G3DrawTexPolySimple (
	int32_t			nVertices, 
	CRenderPoint		**pointList, 
	tUVL			*uvlList, 
	CBitmap		*pBm, 
	CFixVector	*pvNormal,
	int32_t			bBlend);

void OglCachePolyModelTextures (int32_t nModel);

void DrawTexPolyFlat (CBitmap *bm,int32_t nv,CRenderPoint **vertlist);

void OglDrawFilledPoly (int32_t* x, int32_t* y, int32_t nVerts, CCanvasColor *pColor = NULL, int32_t nColors = 1);
void OglDrawFilledRect (int32_t left,int32_t top, int32_t right,int32_t bot, CCanvasColor* pColor = NULL);
void OglDrawPixel (int32_t x, int32_t y, CCanvasColor* pColor = NULL);
void OglDrawLine (int32_t left,int32_t top, int32_t right,int32_t bot, CCanvasColor* pColor = NULL);
void OglDrawEmptyRect (int32_t left, int32_t top, int32_t right, int32_t bot, CCanvasColor* pColor = NULL);

void InitGrayScaleShader (void);

//------------------------------------------------------------------------------

typedef	int32_t tTexPolyMultiDrawer (int32_t, CRenderPoint **, tUVL *, tUVL *, CBitmap *, CBitmap *, tLightmap *, CFixVector *, int32_t, int32_t, int32_t, int16_t);

extern tTexPolyMultiDrawer	*fpDrawTexPolyMulti;

//------------------------------------------------------------------------------

extern GLhandleARB	activeShaderProg;

//------------------------------------------------------------------------------

static inline int32_t G3DrawTexPoly (int32_t nVerts, CRenderPoint **points, tUVL *uvls,
											CBitmap *pBm, CFixVector *pvNormal, int32_t bBlend, int32_t bAdditive, int16_t nSegment)
{
return fpDrawTexPolyMulti (nVerts, points, uvls, NULL, pBm, NULL, NULL, pvNormal, 0, bBlend, bAdditive, nSegment);
}

//------------------------------------------------------------------------------

#endif
