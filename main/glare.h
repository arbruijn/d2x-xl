#ifndef _GLARE_H
#define _GLARE_H

void CalcSpriteCoords (fVector *vSprite, fVector *vCenter, fVector *vEye, float dx, float dy, fMatrix *r);
void RenderCorona (short nSegment, short nSide);

extern float coronaIntensities [];

#endif /* _GLARE_H */
