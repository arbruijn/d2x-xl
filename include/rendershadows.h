#ifndef _RENDERSHADOWS_H
#define _RENDERSHADOWS_H

#include "descent.h"
#include "renderlib.h"

void RenderFaceShadow (tFaceProps *propsP);
void RenderShadowQuad (void);
void CreateShadowTexture (void);
void RenderShadowTexture (void);
int RenderShadowMap (CDynLight *pLight);
void RenderObjectShadows (void);
void DestroyShadowMaps (void);
void ApplyShadowMaps (short nStartSeg, fix xStereoSeparation, int nWindow);
int GatherShadowLightSources (void);
void RenderFastShadows (fix xStereoSeparation, int nWindow, short nStartSeg);
void RenderNeatShadows (fix xStereoSeparation, int nWindow, short nStartSeg);

#if DBG_SHADOWS
extern int bShadowTest;
extern int bFrontCap;
extern int bRearCap;
extern int bShadowVolume;
extern int bFrontFaces;
extern int bBackFaces;
extern int bSWCulling;
extern int bWallShadows;
#endif
extern int bZPass;

#endif //_RENDERSHADOWS_H

// eof
