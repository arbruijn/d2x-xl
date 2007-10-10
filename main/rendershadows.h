#ifndef _RENDERSHADOWS_H
#define _RENDERSHADOWS_H

#include "inferno.h"
#include "renderlib.h"

void RenderFaceShadow (tFaceProps *propsP);
void RenderShadowQuad (int bWhite);
void CreateShadowTexture (void);
void RenderShadowTexture (void);
int RenderShadowMap (tDynLight *pLight);
void RenderObjectShadows (void);
void DestroyShadowMaps (void);
void ApplyShadowMaps (short nStartSeg, fix nEyeOffset, int nWindow);
int GatherShadowLightSources (void);
void RenderFastShadows (fix nEyeOffset, int nWindow, short nStartSeg);
void RenderNeatShadows (fix nEyeOffset, int nWindow, short nStartSeg);

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
#if SHADOWS
extern int bZPass;
#endif

#endif //_RENDERSHADOWS_H

// eof