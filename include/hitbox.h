#ifndef _HITBOX_H
#define _HITBOX_H

#include "fix.h"
//#include "vecmat.h" //the vector/matrix library
#include "gr.h"
#include "cfile.h"
#include "3d.h"

void ComputeHitbox (int32_t nModel, int32_t iSubObj);
tHitbox* TransformHitboxes (CObject *objP, CFixVector *vPos);
int32_t GetPolyModelMinMax (void *modelP, tHitbox *phb, int32_t nSubModels);

#endif //_HITBOX_H

