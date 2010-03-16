#ifndef __OBJSMOKE_H
#define __OBJSMOKE_H

#include "particles.h"

//------------------------------------------------------------------------------

#define	SHOW_SMOKE	\
			(gameOpts->render.effects.bEnabled && !gameStates.app.bNostalgia && EGI_FLAG (bUseParticles, 1, 1, 0))

#define STATIC_SMOKE_MAX_PARTS	1000
#define STATIC_SMOKE_PART_LIFE	-3200
#define STATIC_SMOKE_PART_SPEED	1000

//------------------------------------------------------------------------------

void KillPlayerSmoke (int i);
void ResetPlayerSmoke (void);
void InitObjectSmoke (void);
void ResetObjectSmoke (void);
void KillPlayerBullets (CObject *objP);
void KillGatlingSmoke (CObject *objP);
//static inline int RandN (int n);
void CreateDamageExplosion (int h, int i);
void DoPlayerSmoke (CObject *objP, int i);
void DoRobotSmoke (CObject *objP);
void DoMissileSmoke (CObject *objP);
int DoObjectSmoke (CObject *objP);
void PlayerSmokeFrame (void);
void RobotSmokeFrame (void);
void DoParticleFrame (void);
void InitObjectSmoke (void);
void ResetPlayerSmoke (void);
void ResetRobotSmoke (void);

int CreateShrapnels (CObject *parentObjP);
void DestroyShrapnels (CObject *objP);
int UpdateShrapnels (CObject *objP);
void DrawShrapnels (CObject *objP);

//------------------------------------------------------------------------------

#if DBG

void KillObjectParticleSystem (int i);

#else

static inline void KillObjectSmoke (int i)
{
	int	j;

if ((i >= 0) && ((j = particleManager.GetObjectSystem (i)) >= 0)) {
	particleManager.SetLife (j, 0);
	particleManager.SetObjectSystem (i, -1);
	}
}

#endif

//------------------------------------------------------------------------------
#endif //__OBJSMOKE_H
