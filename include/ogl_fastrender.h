//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_FASTRENDER_H
#define _OGL_FASTRENDER_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

#include "ogl_defs.h"

//------------------------------------------------------------------------------

int RenderDepth (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop);
int RenderLightmaps (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop);
int RenderColor (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop);
int RenderLights (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop);
int RenderHeadlights (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop);
int RenderSky (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop);
int RenderFace (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop, int bBlend, int bTextured);
void FlushFaceBuffer (int bForce);

int LoadPerPixelLightingShader (void);
int SetupPerPixelLightingShader (CSegFace* faceP);
int SetupLightmapShader (CSegFace* faceP, int nType, bool bHeadlight);
int SetupColorShader (void);
int SetupHardwareLighting (CSegFace *faceP);
//int G3SetupHeadlightShader (int nType, int bLightmaps, tRgbaColorf *colorP);
int SetupTexMergeShader (int bColored, int nType);
int SetupGrayScaleShader (int nType, tRgbaColorf *colorP);
int SetupRenderShader (CSegFace *faceP, int bColorKey, int bMultiTexture, int bTextured, int bColored, tRgbaColorf *colorP);
void InitGrayScaleShader (void);

//------------------------------------------------------------------------------

static inline int FaceIsAdditive (CSegFace *faceP)
{
return (int) ((faceP->m_info.bAdditive == 1) ? !(faceP->bmBot && faceP->bmBot->FromPog ()) : faceP->m_info.bAdditive);
}

//------------------------------------------------------------------------------

#endif
