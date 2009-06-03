#ifndef _OBJEFFECTS_H
#define _OBJEFFECTS_H

typedef struct tThrusterInfo {
	CFixVector			vPos [2];
	CFixVector			vDir [2];
	float					fSize;
	float					fLength;
	tPathPoint			*pp;
	CModelThrusters	*mtP;
} tThrusterInfo;

void TransformHitboxf (CObject *objP, CFloatVector *vertList, int iSubObj);
#if DBG
void RenderHitbox (CObject *objP, float red, float green, float blue, float alpha);
#endif
int CalcThrusterPos (CObject *objP, tThrusterInfo *tiP, int bAfterburnerBlob);
void RenderObjectHalo (CFixVector *vPos, fix xSize, float red, float green, float blue, float alpha, int bCorona);
void RenderPowerupCorona (CObject *objP, float red, float green, float blue, float alpha);
void RenderPlayerShield (CObject *objP);
void RenderRobotShield (CObject *objP);
void RenderDamageIndicator (CObject *objP, tRgbColorf *pc);
void RenderMslLockIndicator (CObject *objP);
void RenderTargetIndicator (CObject *objP, tRgbColorf *pc);
void RenderTowedFlag (CObject *objP);
void CalcShipThrusterPos (CObject *objP, CFixVector *vPos);
int CalcThrusterPos (CObject *objP, tThrusterInfo *tiP, int bAfterburnerBlob);
void RenderThrusterFlames (CObject *objP);
void RenderLaserCorona (CObject *objP, tRgbaColorf *colorP, float alpha, float fScale);
int RenderWeaponCorona (CObject *objP, tRgbaColorf *colorP, float alpha, fix xOffset, float fScale, int bSimple, int bViewerOffset, int bDepthSort);
void RenderShockwave (CObject *objP);
void RenderTracers (CObject *objP);
void RenderLightTrail (CObject *objP);
void DrawDebrisCorona (CObject *objP);

#define SHIELD_EFFECT_TIME		2000

#endif //_OBJEFFECTS_H
//eof
