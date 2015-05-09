#ifndef __OBJRENDER_H
#define __OBJRENDER_H

#include "descent.h"

//------------------------------------------------------------------------------

#if DBG
#	define	RENDER_HITBOX	0
#else
#	define	RENDER_HITBOX	0
#endif

//------------------------------------------------------------------------------

typedef struct tCloakInfo {
	int32_t	bFading;
	int32_t	nFadeValue;
	fix	xFadeinDuration;
	fix	xFadeoutDuration;
	fix	xLightScale;
	fix	xDeltaTime;
	fix	xTotalTime;
	} __pack__ tCloakInfo;

//------------------------------------------------------------------------------

void DrawObjectBitmap (CObject *obj, int32_t bmi0, int32_t bmi, int32_t iFrame, CFloatVector *color, float alpha);
// draw an CObject that is a texture-mapped rod
void DrawObjectRodTexPoly (CObject *obj, tBitmapIndex bitmap, int32_t bLit, int32_t iFrame);
int32_t DrawPolygonObject (CObject *pObj, int32_t bForce);
void CalcShipThrusterPos (CObject *pObj, CFixVector *vPos);
int32_t InitAddonPowerup (CObject *pObj);
void ConvertWeaponToPowerup (CObject *pObj);
int32_t ConvertPowerupToWeapon (CObject *pObj);
void ConvertAllPowerupsToWeapons (void);
int32_t RenderObject(CObject *pObj, int32_t nWindowNum, int32_t bForce);
void TransformHitboxf (CObject *pObj, CFloatVector *vertList, int32_t iSubObj);
int32_t GetCloakInfo (CObject *pObj, fix xCloakStartTime, fix xCloakEndTime, tCloakInfo *ciP);

//------------------------------------------------------------------------------

#endif //OBJJRENDER_H
