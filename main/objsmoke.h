#ifndef __OBJSMOKE_H
#define __OBJSMOKE_H

#include "particles.h"

//------------------------------------------------------------------------------

#define	SHOW_SMOKE	\
	(!gameStates.app.bNostalgia && \
	 extraGameInfo [0].bUseSmoke && \
	 gameStates.app.bHaveExtraGameInfo [IsMultiGame] && \
	 extraGameInfo [IsMultiGame].bUseSmoke /*&& \
	 (gameData.demo.nState != ND_STATE_PLAYBACK)*/)

//------------------------------------------------------------------------------

void KillPlayerSmoke (int i);
void ResetPlayerSmoke (void);
void InitObjectSmoke (void);
void ResetObjectSmoke (void);
static inline int RandN (int n);
void CreateDamageExplosion (int h, int i);
void DoPlayerSmoke (tObject *objP, int i);
void DoRobotSmoke (tObject *objP);
void DoMissileSmoke (tObject *objP);
void DoObjectSmoke (tObject *objP);
void PlayerSmokeFrame (void);
void RobotSmokeFrame (void);
void DoSmokeFrame (void);

//------------------------------------------------------------------------------

static inline void KillObjectSmoke (int i)
{
if ((i >= 0) && (gameData.smoke.objects [i] >= 0)) {
	SetSmokeLife (gameData.smoke.objects [i], 0);
	gameData.smoke.objects [i] = -1;
	}
}

//------------------------------------------------------------------------------
#endif //__OBJSMOKE_H
