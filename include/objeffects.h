#ifndef _OBJEFFECTS_H
#define _OBJEFFECTS_H

typedef struct tThrusterInfo {
	vmsVector			vPos [2];
	vmsVector			vDir [2];
	float					fSize;
	float					fLength;
	tPathPoint			*pp;
	tModelThrusters	*mtP;
} tThrusterInfo;

void TransformHitboxf (CObject *objP, fVector *vertList, int iSubObj);
#if DBG
void RenderHitbox (CObject *objP, float red, float green, float blue, float alpha);
#endif
int CalcThrusterPos (CObject *objP, tThrusterInfo *tiP, int bAfterburnerBlob);
void RenderObjectHalo (vmsVector *vPos, fix xSize, float red, float green, float blue, float alpha, int bCorona);
void RenderPowerupCorona (CObject *objP, float red, float green, float blue, float alpha);
void RenderPlayerShield (CObject *objP);
void RenderRobotShield (CObject *objP);
void RenderDamageIndicator (CObject *objP, tRgbColorf *pc);
void RenderMslLockIndicator (CObject *objP);
void RenderTargetIndicator (CObject *objP, tRgbColorf *pc);
void RenderTowedFlag (CObject *objP);
void CalcShipThrusterPos (CObject *objP, vmsVector *vPos);
int CalcThrusterPos (CObject *objP, tThrusterInfo *tiP, int bAfterburnerBlob);
void RenderThrusterFlames (CObject *objP);
void RenderLaserCorona (CObject *objP, tRgbaColorf *colorP, float alpha, float fScale);
int RenderWeaponCorona (CObject *objP, tRgbaColorf *colorP, float alpha, fix xOffset, float fScale, int bSimple, int bViewerOffset, int bDepthSort);
void RenderShockwave (CObject *objP);
void RenderTracers (CObject *objP);
void RenderLightTrail (CObject *objP);
void DrawDebrisCorona (CObject *objP);
void CreatePlayerAppearanceEffect (CObject *playerObjP);

#endif //_OBJEFFECTS_H
//eof
