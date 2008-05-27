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
int G3DrawWhitePoly (int nv, g3sPoint **pointList);
int G3DrawPolyAlpha (int nv, g3sPoint **pointlist, tRgbaColorf *color, char bDepthMask, short nSegment);
void G3FlushFaceBuffer (int bForce);

int G3DrawTexPolyMulti (
	int			nVerts, 
	g3sPoint		**pointList, 
	tUVL			*uvlList, 
	tUVL			*uvlLMap, 
	grsBitmap	*bmBot, 
	grsBitmap	*bmTop, 
	tLightmap	*lightmap, 
	vmsVector	*pvNormal,
	int			orient, 
	int			bBlend,
	short			nSegment);

int G3DrawTexPolyLightmap (
	int			nVerts, 
	g3sPoint		**pointList, 
	tUVL			*uvlList, 
	tUVL			*uvlLMap, 
	grsBitmap	*bmBot, 
	grsBitmap	*bmTop, 
	tLightmap	*lightmap, 
	vmsVector	*pvNormal,
	int			orient, 
	int			bBlend,
	short			nSegment);

int G3DrawTexPolyFlat (
	int			nVerts, 
	g3sPoint		**pointList, 
	tUVL			*uvlList, 
	tUVL			*uvlLMap, 
	grsBitmap	*bmBot, 
	grsBitmap	*bmTop, 
	tLightmap	*lightmap, 
	vmsVector	*pvNormal,
	int			orient, 
	int			bBlend,
	short			nSegment);

int G3DrawTexPolySimple (
	int			nVertices, 
	g3sPoint		**pointList, 
	tUVL			*uvlList, 
	grsBitmap	*bmP, 
	vmsVector	*pvNormal,
	int			bBlend);

void OglCachePolyModelTextures (int nModel);

void DrawTexPolyFlat (grsBitmap *bm,int nv,g3sPoint **vertlist);
int OglRenderArrays (grsBitmap *bmP, int nFrame, fVector *vertexP, int nVertices, tTexCoord2f *texCoordP, 
							tRgbaColorf *colorP, int nColors, int nPrimitive, int nWrap);

void OglURect(int left,int top,int right,int bot);

void InitGrayScaleShader (void);

//------------------------------------------------------------------------------

typedef	int tTexPolyMultiDrawer (int, g3sPoint **, tUVL *, tUVL *, grsBitmap *, grsBitmap *, tLightmap *, vmsVector *, int, int, short);

extern tTexPolyMultiDrawer	*fpDrawTexPolyMulti;

//------------------------------------------------------------------------------

extern GLhandleARB	activeShaderProg;

//------------------------------------------------------------------------------

static inline int G3DrawTexPoly (int nVerts, g3sPoint **pointList, tUVL *uvlList,
											grsBitmap *bmP, vmsVector *pvNormal, int bBlend, short nSegment)
{
return fpDrawTexPolyMulti (nVerts, pointList, uvlList, NULL, bmP, NULL, NULL, pvNormal, 0, bBlend, nSegment);
}

//------------------------------------------------------------------------------

#endif
