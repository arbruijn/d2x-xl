#ifndef __LIGHTNING_H
#define __LIGHTNING_H

#include "inferno.h"

int CreateLightning (int nLightnings, vmsVector *vPos, vmsVector *vEnd, vmsVector *vDelta,
							short nObject, int nLife, int nDelay, int nLength, int nAmplitude, int nOffset,
							short nNodeC, short nChildC, char nDepth, short nSteps, short nSmoothe, 
							char bClamp, char bPlasma, tRgbaColorf *colorP);
void DestroyLightnings (int iLightning, tLightning *pf, int bDestroy);
int DestroyAllLightnings (void);
void MoveLightnings (int i, tLightning *pl, vmsVector *vNewPos, short nSegment, int bStretch, int bFromEnd);
void RenderLightning (tLightning *pl, int nLightnings, short nDepth, int bDepthSort);
void RenderLightnings (void);
void RenderDamageLightnings (tObject *objP, g3sPoint **pointlist, int nVertices);
void MoveObjectLightnings (tObject *objP);
void DestroyObjectLightnings (tObject *objP);
void SetLightningLights (void);
void ResetLightningLights (void);
void DoLightningFrame (void);
void CreateShakerLightnings (tObject *objP);
void CreateShakerMegaLightnings (tObject *objP);
void CreateMegaLightnings (tObject *objP);
void CreateDamageLightnings (tObject *objP, tRgbaColorf *colorP);
void CreateRobotLightnings (tObject *objP, tRgbaColorf *colorP);

#define	SHOW_LIGHTNINGS \
			(!(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bUseLightnings, 1, 1, 0))

#endif //__LIGHTNING_H
