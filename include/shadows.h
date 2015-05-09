/* $Id: interp.h,v 1.6 2003/02/13 22:07:58 btb Exp $ */
/*
 *
 * took out functions declarations from include/3d.h
 * which are implemented in 3d/interp.c
 *
 */

#ifndef _SHADOWS_H
#define _SHADOWS_H

#include "oof.h"

void SetCullAndStencil (int32_t bCullFront, int32_t bZPass);
void RenderShadowVolumeFace (CFloatVector *pv);
void RenderFarShadowCapFace (CFloatVector *pv, int32_t nVertices);
int32_t POFGatherPolyModelItems (CObject *pObj, void *pModel, CAngleVector *pAnimAngles, POF::CModel *po, int32_t bShadowData);
int32_t POFFreePolyModelItems (POF::CModel *po);
void POFFreeAllPolyModelItems (void);

#endif //_SHADOWS_H

