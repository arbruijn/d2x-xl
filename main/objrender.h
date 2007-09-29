#ifndef __OBJRENDER_H
#define __OBJRENDER_H

#include "inferno.h"

typedef struct tThrusterInfo {
	vmsVector			vPos [2];
	vmsVector			vDir [2];
	float					fSize;
	float					fLength;
	tPathPoint			*pp;
	tModelThrusters	*mtP;
} tThrusterInfo;

int CalcThrusterPos (tObject *objP, tThrusterInfo *tiP, int bAfterburnerBlob);
void DrawObjectBlob (tObject *obj, tBitmapIndex bmi0, tBitmapIndex bmi, int iFrame, tRgbaColorf *color, float alpha);

// draw an tObject that is a texture-mapped rod
void DrawObjectRodTexPoly (tObject *obj, tBitmapIndex bitmap, int bLit, int iFrame);

void DrawPolygonObject (tObject *objP);
void CalcShipThrusterPos (tObject *objP, vmsVector *vPos);
void ConvertWeaponToPowerup (tObject *objP);
int RenderObject(tObject *objP, int nWindowNum, int bForce);
void RenderObjectHalo (tObject *objP, fix xSize, float red, float green, float blue, float alpha, int bCorona);

void ConvertAllPowerupsToWeapons (void);

#endif //OBJJRENDER_H
