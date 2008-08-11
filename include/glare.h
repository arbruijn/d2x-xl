#ifndef _GLARE_H
#define _GLARE_H

void CalcSpriteCoords (fVector *vSprite, fVector *vCenter, fVector *vEye, float dx, float dy, fMatrix *r);
int FaceHasCorona (short nSegment, short nSide, int *bAdditiveP, float *dimP);
void RenderCorona (short nSegment, short nSide, float fIntensity, float fSize);
float CoronaVisibility (int nQuery);
void LoadGlareShader (float dMax);
void UnloadGlareShader (void);
void InitGlareShader (void);
int CoronaStyle (void);
void DestroyGlareDepthTexture (void);

extern float coronaIntensities [];
extern GLuint hDepthBuffer; 

#endif /* _GLARE_H */
