#ifndef __OBJSMOKE_H
#define __OBJSMOKE_H

#include "particles.h"

//------------------------------------------------------------------------------

#define	SHOW_SMOKE	\
			(!gameStates.app.bNostalgia && EGI_FLAG (bUseSmoke, 1, 1, 0))

#define MAX_SHRAPNEL_LIFE	(2 * F1_0)

//------------------------------------------------------------------------------

void KillPlayerSmoke (int i);
void ResetPlayerSmoke (void);
void InitObjectSmoke (void);
void ResetObjectSmoke (void);
//static inline int RandN (int n);
void CreateDamageExplosion (int h, int i);
void DoPlayerSmoke (tObject *objP, int i);
void DoRobotSmoke (tObject *objP);
void DoMissileSmoke (tObject *objP);
int DoObjectSmoke (tObject *objP);
void PlayerSmokeFrame (void);
void RobotSmokeFrame (void);
void DoSmokeFrame (void);

int CreateShrapnels (tObject *parentObjP);
void DestroyShrapnels (tObject *objP);
int UpdateShrapnels (tObject *objP);
void DrawShrapnels (tObject *objP);

//------------------------------------------------------------------------------

#ifdef _DEBUG

void KillObjectSmoke (int i);

#else

static inline void KillObjectSmoke (int i)
{
if ((i >= 0) && (gameData.smoke.objects [i] >= 0)) {
	SetSmokeLife (gameData.smoke.objects [i], 0);
	gameData.smoke.objects [i] = -1;
	}
}

#endif

//------------------------------------------------------------------------------
#endif //__OBJSMOKE_H
