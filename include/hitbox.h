#ifndef _HITBOX_H
#define _HITBOX_H

#include "fix.h"
//#include "vecmat.h" //the vector/matrix library
#include "gr.h"
#include "cfile.h"
#include "3d.h"

void ComputeHitbox (int nModel, int iSubObj);
void TransformHitboxes (CObject *objP, CFixVector *vPos, tBox *phb);
int GetPolyModelMinMax (void *modelP, tHitbox *phb, int nSubModels);

#endif //_HITBOX_H

