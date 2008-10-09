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

void DrawObjectBlob (tObject *obj, int bmi0, int bmi, int iFrame, tRgbaColorf *color, float alpha);
// draw an tObject that is a texture-mapped rod
void DrawObjectRodTexPoly (tObject *obj, tBitmapIndex bitmap, int bLit, int iFrame);
int DrawPolygonObject (tObject *objP, int bDepthSort, int bForce);
void CalcShipThrusterPos (tObject *objP, vmsVector *vPos);
int InitAddonPowerup (tObject *objP);
void ConvertWeaponToPowerup (tObject *objP);
int ConvertPowerupToWeapon (tObject *objP);
void ConvertAllPowerupsToWeapons (void);
int RenderObject(tObject *objP, int nWindowNum, int bForce);
void TransformHitboxf (tObject *objP, fVector *vertList, int iSubObj);
int GetCloakInfo (tObject *objP, fix xCloakStartTime, fix xCloakEndTime, tCloakInfo *ciP);

//------------------------------------------------------------------------------

#endif //OBJJRENDER_H
