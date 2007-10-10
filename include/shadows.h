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

void G3SetCullAndStencil (int bCullFront, int bZPass);
void G3RenderShadowVolumeFace (tOOF_vector *pv);
void G3RenderFarShadowCapFace (tOOF_vector *pv, int nVertices);
int POFGatherPolyModelItems (tObject *objP, void *modelP, vmsAngVec *pAnimAngles, tPOFObject *po, int bShadowData);
int POFFreePolyModelItems (tPOFObject *po);
void POFFreeAllPolyModelItems (void);

#endif //_SHADOWS_H

