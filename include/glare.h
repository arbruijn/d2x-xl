#ifndef _GLARE_H
#define _GLARE_H

void CalcSpriteCoords (fVector *vSprite, fVector *vCenter, fVector *vEye, float dx, float dy, fMatrix *r);
int FaceHasCorona (short nSegment, short nSide, int *bAdditiveP, float *dimP);
void RenderCorona (short nSegment, short nSide, float fIntensity);
float CoronaVisibility (int nQuery);
void LoadGlareShader (void);
void UnloadGlareShader (void);
void InitGlareShader (void);
int CoronaStyle (void);
void DestroyGlareDepthTexture (void);

extern float coronaIntensities [];

#endif /* _GLARE_H */
