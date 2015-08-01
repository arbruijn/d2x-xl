#ifndef _RENDERSHADOWS_H
#define _RENDERSHADOWS_H

#include "descent.h"
#include "renderlib.h"

void RenderFaceShadow (tFaceProps *pProps);
void RenderShadowQuad (void);
void CreateShadowTexture (void);
void RenderShadowTexture (void);
int32_t RenderShadowMap (CDynLight *pLight);
void RenderObjectShadows (void);
void DestroyShadowMaps (void);
void ApplyShadowMaps (int16_t nStartSeg, fix xStereoSeparation, int32_t nWindow);
int32_t GatherShadowLightSources (void);
void RenderFastShadows (fix xStereoSeparation, int32_t nWindow, int16_t nStartSeg);
void RenderNeatShadows (fix xStereoSeparation, int32_t nWindow, int16_t nStartSeg);

#if DBG_SHADOWS
extern int32_t bShadowTest;
extern int32_t bFrontCap;
extern int32_t bRearCap;
extern int32_t bShadowVolume;
extern int32_t bFrontFaces;
extern int32_t bBackFaces;
extern int32_t bSWCulling;
extern int32_t bWallShadows;
#endif
extern int32_t bZPass;

#endif //_RENDERSHADOWS_H

// eof
