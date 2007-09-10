#ifndef __LIGHTNING_H
#define __LIGHTNING_H

#include "inferno.h"

int CreateLightning (int nLightnings, vmsVector *vPos, vmsVector *vEnd, short nObject, int nLife, int nDelay, int nLength, int nAmplitude, int nOffset,
							short nNodeC, short nChildC, short nDepth, short nSteps, short nSmoothe, char bClamp, tRgbaColorf *colorP);
void DestroyLightnings (int iLightning, tLightning *pf, int bDestroy);
int DestroyAllLightnings (void);
void MoveLightnings (int i, tLightning *pl, vmsVector *vNewPos, short nSegment, int bStretch, int bFromEnd);
void RenderLightnings (void);
void MoveObjectLightnings (tObject *objP);
void DestroyObjectLightnings (tObject *objP);
void SetLightningLights (void);
void ResetLightningLights (void);
void DoLightningFrame (void);
void CreateShakerLightnings (tObject *objP);
void CreateShakerMegaLightnings (tObject *objP);
void CreateMegaLightnings (tObject *objP);
void CreateRobotLightnings (tObject *objP, tRgbaColorf *colorP);
void RenderLightning (tLightning *pl, int nLightnings, short nDepth, int bDepthSort);

#define	SHOW_LIGHTNINGS \
			(!(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bUseLightnings, 1, 1, 0))

#endif //__LIGHTNING_H
