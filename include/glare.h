#ifndef _GLARE_H
#define _GLARE_H

void CalcSpriteCoords (CFloatVector *vSprite, CFloatVector *vCenter, CFloatVector *vEye, float dx, float dy, CFloatMatrix *r);
int FaceHasCorona (short nSegment, short nSide, int *bAdditiveP, float *dimP);
void RenderCorona (short nSegment, short nSide, float fIntensity, float fSize);
float CoronaVisibility (int nQuery);
void LoadGlareShader (float dMax, int bAdditive = 1);
void UnloadGlareShader (void);
void InitGlareShader (void);
int CoronaStyle (void);
void DestroyGlareDepthTexture (void);

extern float coronaIntensities [];
extern GLuint hDepthBuffer;

#endif /* _GLARE_H */
