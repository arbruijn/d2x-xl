#ifndef __SPHERE_H
#define __SPHERE_H

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "oof.h"

int CreateSphere (tSphereData *sdP);

int RenderSphere (tSphereData *sdP, tOOF_vector *pPos, float fRad,
					   float red, float green, float blue, float alpha, grsBitmap *bmP);

void DrawShieldSphere (tObject *objP, float red, float green, float blue, float alpha);

void DrawMonsterball (tObject *objP, float red, float green, float blue, float alpha);

void DestroySphere (tSphereData *sdP);

void SetSpherePulse (tPulseData *pPulse, float fSpeed, float fMin);

void UseSpherePulse (tSphereData *sdP, tPulseData *pPulse);

#endif //__SPHERE_H

//eof
