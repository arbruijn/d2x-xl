//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_FASTRENDER_H
#define _OGL_FASTRENDER_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

#include "ogl_defs.h"

//------------------------------------------------------------------------------

int DrawFaceSimple (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop, int bTextured);
int RenderFace (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop, int bTextured, int bLightmaps);
int DrawHeadlights (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop, int bTextured, int bLightmaps);
void FlushFaceBuffer (int bForce);
int SetupPerPixelLightingShader (CSegFace *faceP, int nType, bool bHeadlight);
int SetupLightmapShader (CSegFace *faceP, int nType, bool bHeadlight);
int SetupTexMergeShader (int bColorKey, int bColored, int nType);
int SetupGrayScaleShader (int nType, CFloatVector *colorP);
int SetupShader (CSegFace *faceP, int bColorKey, int bMultiTexture, int bTextured, int bColored, CFloatVector *colorP);
void InitGrayScaleShader (void);

//------------------------------------------------------------------------------

static inline int FaceIsAdditive (CSegFace *faceP)
{
return (int) ((faceP->m_info.bAdditive == 1) ? !(faceP->bmBot && faceP->bmBot->FromPog ()) : faceP->m_info.bAdditive);
}

//------------------------------------------------------------------------------

typedef int (*tFaceRenderFunc) (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop, int bTextured, int bLightmaps);

extern tFaceRenderFunc faceRenderFunc;

static inline void SetFaceDrawer (int nType)
{
faceRenderFunc = RenderFace;
}

//------------------------------------------------------------------------------

#endif
