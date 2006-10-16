#ifndef __OBJSMOKE_H
#define __OBJSMOKE_H

#include "particles.h"

//------------------------------------------------------------------------------

#define	PLR_PART_LIFE	-4000
#define	PLR_PART_SPEED	40
#define	OBJ_PART_LIFE	-4000
#define	OBJ_PART_SPEED	40
#define	MSL_PART_LIFE	-3000
#define	MSL_PART_SPEED	30
#define	DEBRIS_PART_LIFE	-2000
#define	DEBRIS_PART_SPEED	30

#define	SHOW_SMOKE	\
	(extraGameInfo [0].bUseSmoke && \
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
void DoPlayerSmoke (object *objP, int i);
void DoRobotSmoke (object *objP);
void DoMissileSmoke (object *objP);
void DoObjectSmoke (object *objP);
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
