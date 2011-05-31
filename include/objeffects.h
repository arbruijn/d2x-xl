#ifndef _OBJEFFECTS_H
#define _OBJEFFECTS_H

void TransformHitboxf (CObject *objP, CFloatVector *vertList, int iSubObj);
#if DBG
void RenderHitbox (CObject *objP, float red, float green, float blue, float alpha);
#endif
void RenderObjectHalo (CFixVector *vPos, fix xSize, float red, float green, float blue, float alpha, int bCorona);
void RenderPowerupCorona (CObject *objP, float red, float green, float blue, float alpha);
void RenderPlayerShield (CObject *objP);
void RenderRobotShield (CObject *objP);
void RenderDamageIndicator (CObject *objP, CFloatVector3 *pc);
void RenderMslLockIndicator (CObject *objP);
void RenderTargetIndicator (CObject *objP, CFloatVector3 *pc);
void RenderTowedFlag (CObject *objP);
void RenderLaserCorona (CObject *objP, CFloatVector *colorP, float alpha, float fScale);
int RenderWeaponCorona (CObject *objP, CFloatVector *colorP, float alpha, fix xOffset, float fScale, int bSimple, int bViewerOffset, int bDepthSort);
void RenderLightTrail (CObject *objP);
void DrawDebrisCorona (CObject *objP);

#define SHIELD_EFFECT_TIME		((gameOpts->render.bUseShaders && ogl.m_features.bShaders.Available ()) ? 2000 : 500)

#endif //_OBJEFFECTS_H
//eof
