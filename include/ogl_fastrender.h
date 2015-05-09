//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_FASTRENDER_H
#define _OGL_FASTRENDER_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

#include "ogl_defs.h"

//------------------------------------------------------------------------------

int32_t DrawFaceSimple (CSegFace *pFace, CBitmap *bmBot, CBitmap *bmTop, int32_t bTextured);
int32_t RenderFace (CSegFace *pFace, CBitmap *bmBot, CBitmap *bmTop, int32_t bTextured, int32_t bLightmaps);
int32_t DrawHeadlights (CSegFace *pFace, CBitmap *bmBot, CBitmap *bmTop, int32_t bTextured, int32_t bLightmaps);
void FlushFaceBuffer (int32_t bForce);
int32_t SetupPerPixelLightingShader (CSegFace *pFace, int32_t nType, bool bHeadlight);
int32_t SetupLightmapShader (CSegFace *pFace, int32_t nType, bool bHeadlight);
int32_t SetupTexMergeShader (int32_t bColorKey, int32_t bColored, int32_t nType);
int32_t SetupGrayScaleShader (int32_t nType, CFloatVector *pColor);
int32_t SetupShader (CSegFace *pFace, int32_t bColorKey, int32_t bMultiTexture, int32_t bTextured, int32_t bColored, CFloatVector *pColor);
void InitGrayScaleShader (void);

//------------------------------------------------------------------------------

static inline int32_t FaceIsAdditive (CSegFace *pFace)
{
return (int32_t) ((pFace->m_info.bAdditive == 1) ? !(pFace->bmBot && pFace->bmBot->FromPog ()) : pFace->m_info.bAdditive);
}

//------------------------------------------------------------------------------

typedef int32_t (*tFaceRenderFunc) (CSegFace *pFace, CBitmap *bmBot, CBitmap *bmTop, int32_t bTextured, int32_t bLightmaps);

extern tFaceRenderFunc faceRenderFunc;

static inline void SetFaceDrawer (int32_t nType)
{
faceRenderFunc = RenderFace;
}

//------------------------------------------------------------------------------

#endif
