#ifndef __OBJRENDER_H
#define __OBJRENDER_H

#include "inferno.h"

#ifdef _DEBUG
#	define	RENDER_HITBOX	0
#else
#	define	RENDER_HITBOX	0
#endif

void DrawObjectBlob (tObject *obj, tBitmapIndex bmi0, tBitmapIndex bmi, int iFrame, tRgbaColorf *color, float alpha);
// draw an tObject that is a texture-mapped rod
void DrawObjectRodTexPoly (tObject *obj, tBitmapIndex bitmap, int bLit, int iFrame);
void DrawPolygonObject (tObject *objP);
void CalcShipThrusterPos (tObject *objP, vmsVector *vPos);
void ConvertWeaponToPowerup (tObject *objP);
int ConvertPowerupToWeapon (tObject *objP);
void ConvertAllPowerupsToWeapons (void);
int RenderObject(tObject *objP, int nWindowNum, int bForce);
void TransformHitboxf (tObject *objP, fVector *vertList, int iSubObj);

#endif //OBJJRENDER_H
