#ifndef __OBJRENDER_H
#define __OBJRENDER_H

#include "inferno.h"

//------------------------------------------------------------------------------

#if DBG
#	define	RENDER_HITBOX	0
#else
#	define	RENDER_HITBOX	0
#endif

//------------------------------------------------------------------------------

typedef struct tCloakInfo {
	int	bFading;
	int	nFadeValue;
	fix	xFadeinDuration;
	fix	xFadeoutDuration;
	fix	xLightScale;
	fix	xDeltaTime;
	fix	xTotalTime;
	} tCloakInfo;

//------------------------------------------------------------------------------

void DrawObjectBlob (CObject *obj, int bmi0, int bmi, int iFrame, tRgbaColorf *color, float alpha);
// draw an CObject that is a texture-mapped rod
void DrawObjectRodTexPoly (CObject *obj, tBitmapIndex bitmap, int bLit, int iFrame);
int DrawPolygonObject (CObject *objP, int bDepthSort, int bForce);
void CalcShipThrusterPos (CObject *objP, vmsVector *vPos);
int InitAddonPowerup (CObject *objP);
void ConvertWeaponToPowerup (CObject *objP);
int ConvertPowerupToWeapon (CObject *objP);
void ConvertAllPowerupsToWeapons (void);
int RenderObject(CObject *objP, int nWindowNum, int bForce);
void TransformHitboxf (CObject *objP, fVector *vertList, int iSubObj);
int GetCloakInfo (CObject *objP, fix xCloakStartTime, fix xCloakEndTime, tCloakInfo *ciP);

//------------------------------------------------------------------------------

#endif //OBJJRENDER_H
