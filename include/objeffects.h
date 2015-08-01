#ifndef _OBJEFFECTS_H
#define _OBJEFFECTS_H

void TransformHitboxf (CObject *pObj, CFloatVector *vertList, int32_t iSubObj);
#if DBG
void RenderHitbox (CObject *pObj, float red, float green, float blue, float alpha);
#endif
void RenderObjectHalo (CFixVector *vPos, fix xSize, float red, float green, float blue, float alpha, int32_t bCorona);
void RenderPowerupCorona (CObject *pObj, float red, float green, float blue, float alpha);
void RenderPlayerShield (CObject *pObj);
void RenderRobotShield (CObject *pObj);
void RenderDamageIndicator (CObject *pObj, CFloatVector3 *pc);
void RenderMslLockIndicator (CObject *pObj);
void RenderTargetIndicator (CObject *pObj, CFloatVector3 *pc);
void RenderTowedFlag (CObject *pObj);
void RenderLaserCorona (CObject *pObj, CFloatVector *pColor, float alpha, float fScale);
int32_t RenderWeaponCorona (CObject *pObj, CFloatVector *pColor, float alpha, fix xOffset, float fScale, int32_t bSimple, int32_t bViewerOffset, int32_t bDepthSort);
void RenderLightTrail (CObject *pObj);
void DrawDebrisCorona (CObject *pObj);

#define SHIELD_EFFECT_TIME		((gameOpts->render.bUseShaders && ogl.m_features.bShaders.Available ()) ? 2000 : 500)

#endif //_OBJEFFECTS_H
//eof
