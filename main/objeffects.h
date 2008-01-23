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

void TransformHitboxf (tObject *objP, fVector *vertList, int iSubObj);
#ifdef _DEBUG
void RenderHitbox (tObject *objP, float red, float green, float blue, float alpha);
#endif
int CalcThrusterPos (tObject *objP, tThrusterInfo *tiP, int bAfterburnerBlob);
void RenderObjectHalo (vmsVector *vPos, fix xSize, float red, float green, float blue, float alpha, int bCorona);
void RenderPowerupCorona (tObject *objP, float red, float green, float blue, float alpha);
void RenderPlayerShield (tObject *objP);
void RenderRobotShield (tObject *objP);
void RenderDamageIndicator (tObject *objP, tRgbColorf *pc);
void RenderMslLockIndicator (tObject *objP);
void RenderTargetIndicator (tObject *objP, tRgbColorf *pc);
void RenderTowedFlag (tObject *objP);
void CalcShipThrusterPos (tObject *objP, vmsVector *vPos);
int CalcThrusterPos (tObject *objP, tThrusterInfo *tiP, int bAfterburnerBlob);
void RenderThrusterFlames (tObject *objP);
void RenderLaserCorona (tObject *objP, tRgbaColorf *colorP, float alpha, float fScale);
void RenderWeaponCorona (tObject *objP, tRgbaColorf *colorP, float alpha, fix xOffset, float fScale, int bSimple, int bViewerOffset, int bDepthSort);
void RenderShockwave (tObject *objP);
void RenderTracers (tObject *objP);
void RenderLightTrail (tObject *objP);
void DrawDebrisCorona (tObject *objP);

#endif //_OBJEFFECTS_H
//eof
