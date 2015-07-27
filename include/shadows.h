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

void SetCullAndStencil (int bCullFront, int bZPass);
void RenderShadowVolumeFace (CFloatVector *pv);
void RenderFarShadowCapFace (CFloatVector *pv, int nVertices);
int POFGatherPolyModelItems (CObject *objP, void *modelP, CAngleVector *pAnimAngles, POF::CModel *po, int bShadowData);
int POFFreePolyModelItems (POF::CModel *po);
void POFFreeAllPolyModelItems (void);

#endif //_SHADOWS_H

