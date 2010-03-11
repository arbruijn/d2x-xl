//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_FASTRENDER_H
#define _OGL_FASTRENDER_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

#include "ogl_defs.h"

//------------------------------------------------------------------------------

int RenderDepth (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop, int bTransparent);
int RenderLightmaps (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop);
int RenderLights (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop);
int RenderFaceVL (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop, int bBlend, int bTextured, int bDepthOnly);
int RenderFaceLM (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop, int bBlend, int bTextured, int bDepthOnly);
int RenderFacePP (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop, int bBlend, int bTextured, int bDepthOnly);
int RenderHeadlightsVL (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop, int bBlend, int bTextured, int bDepthOnly);
int RenderHeadlightsPP (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop, int bBlend, int bTextured, int bDepthOnly);
void FlushFaceBuffer (int bForce);

int SetupPerPixelLightingShader (CSegFace* faceP, int nType);
int SetupLightmapShader (CSegFace* faceP, int nType, bool bHeadlight);
int SetupStaticLightingShader (CSegFace* faceP, int bColorKey);
//int G3SetupHeadlightShader (int nType, int bLightmaps, tRgbaColorf *colorP);
int SetupTexMergeShader (int bColorKey, int bColored, int nType);
int SetupGrayScaleShader (int nType, tRgbaColorf *colorP);
int SetupRenderShader (CSegFace *faceP, int bColorKey, int bMultiTexture, int bTextured, int bColored, tRgbaColorf *colorP);
void InitGrayScaleShader (void);

typedef int (*tRenderFaceDrawerP) (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop, int bBlend, int bTextured, int bDepthOnly);

extern tRenderFaceDrawerP faceRenderFunc;

//------------------------------------------------------------------------------

static inline int FaceIsAdditive (CSegFace *faceP)
{
return (int) ((faceP->m_info.bAdditive == 1) ? !(faceP->bmBot && faceP->bmBot->FromPog ()) : faceP->m_info.bAdditive);
}

//------------------------------------------------------------------------------

static inline void SetFaceDrawer (int nType)
{
if (nType < 0)
	nType = gameStates.render.bPerPixelLighting;
if (nType == 2)
	faceRenderFunc = RenderFacePP;
else if (nType == 1)
	faceRenderFunc = RenderFaceLM;
else
	faceRenderFunc = RenderFaceVL;
}

//------------------------------------------------------------------------------

#endif
