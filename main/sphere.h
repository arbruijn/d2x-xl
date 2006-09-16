#ifndef __SPHERE_H
#define __SPHERE_H

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "oof.h"

int CreateSphere (tOOF_vector **pSphere);

int RenderSphere (tOOF_vector *pPos, tOOF_vector *pSphere, int nFaces, float fRad,
					   float red, float green, float blue, float alpha);

void DrawObjectSphere (object *objP, float red, float green, float blue, float alpha);

void DestroySphere (void);

void SetSpherePulse (tPulseData *pPulse, float fSpeed, float fMin);

void UseSpherePulse (tPulseData *pPulse);

#endif //__SPHERE_H

//eof
