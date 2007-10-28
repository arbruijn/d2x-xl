//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_RENDER_H
#define _OGL_RENDER_H

#ifdef _MSC_VER
#include <windows.h>
#include <stddef.h>
#endif

#include "ogl_defs.h"
#include "ogl_lib.h"

void OglDrawEllipse (int nSides, int nType, float xsc, float xo, float ysc, float yo, tSinCosf *sinCosP);
void OglDrawCircle (int nSides, int nType);
bool G3DrawWhitePoly (int nv, g3sPoint **pointList);
bool G3DrawPolyAlpha (int nv, g3sPoint **pointlist, tRgbaColorf *color, char bDepthMask);
bool G3DrawFace (grsFace *faceP, grsBitmap *bmBot, grsBitmap *bmTop, int bBlend, int bDrawArrays, int bTextured, int bDepthOnly);
void G3FlushFaceBuffer (int bForce);

bool G3DrawTexPolyMulti (
	int			nVerts, 
	g3sPoint		**pointList, 
	tUVL			*uvlList, 
	tUVL			*uvlLMap, 
	grsBitmap	*bmBot, 
	grsBitmap	*bmTop, 
	tOglTexture	*lightMap, 
	vmsVector	*pvNormal,
	int			orient, 
	int			bBlend);

bool G3DrawTexPolyLightmap (
	int			nVerts, 
	g3sPoint		**pointList, 
	tUVL			*uvlList, 
	tUVL			*uvlLMap, 
	grsBitmap	*bmBot, 
	grsBitmap	*bmTop, 
	tOglTexture	*lightMap, 
	vmsVector	*pvNormal,
	int			orient, 
	int			bBlend);

bool G3DrawTexPolyFlat (
	int			nVerts, 
	g3sPoint		**pointList, 
	tUVL			*uvlList, 
	tUVL			*uvlLMap, 
	grsBitmap	*bmBot, 
	grsBitmap	*bmTop, 
	tOglTexture	*lightMap, 
	vmsVector	*pvNormal,
	int			orient, 
	int			bBlend);

bool G3DrawTexPolySimple (
	int			nVertices, 
	g3sPoint		**pointList, 
	tUVL			*uvlList, 
	grsBitmap	*bmP, 
	vmsVector	*pvNormal,
	int			bBlend);

void OglCachePolyModelTextures (int nModel);

void DrawTexPolyFlat (grsBitmap *bm,int nv,g3sPoint **vertlist);
int OglRenderArrays (grsBitmap *bmP, int nFrame, fVector *vertexP, int nVertices, tTexCoord3f *texCoordP, 
							tRgbaColorf *colorP, int nColors, int nPrimitive, int nWrap);

void OglURect(int left,int top,int right,int bot);

//------------------------------------------------------------------------------

typedef	bool tTexPolyMultiDrawer (int, g3sPoint **, tUVL *, tUVL *, grsBitmap *, grsBitmap *, tOglTexture *, vmsVector *, int, int);

extern tTexPolyMultiDrawer	*fpDrawTexPolyMulti;

//------------------------------------------------------------------------------

extern GLhandleARB	lmProg;
extern GLhandleARB	tmProg;

//------------------------------------------------------------------------------

static inline int G3DrawTexPoly (int nVerts, g3sPoint **pointList, tUVL *uvlList,
											grsBitmap *bmP, vmsVector *pvNormal, int bBlend)
{
return fpDrawTexPolyMulti (nVerts, pointList, uvlList, NULL, bmP, NULL, NULL, pvNormal, 0, bBlend);
}

//------------------------------------------------------------------------------

#endif
