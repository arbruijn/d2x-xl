#ifndef __OBJSMOKE_H
#define __OBJSMOKE_H

#include "particles.h"

//------------------------------------------------------------------------------

#define	SHOW_SMOKE	\
			(!gameStates.app.bNostalgia && EGI_FLAG (bUseSmoke, 1, 1, 0))

#define MAX_SHRAPNEL_LIFE	(2 * F1_0)

#define SHIP_MAX_PARTS				50
#define PLR_PART_LIFE				-4000
#define PLR_PART_SPEED				40

#define BOT_MAX_PARTS				250
#define BOT_PART_LIFE				-6000
#define BOT_PART_SPEED				300

#define MSL_MAX_PARTS				500
#define MSL_PART_LIFE				-3000
#define MSL_PART_SPEED				30

#define LASER_MAX_PARTS				250
#define LASER_PART_LIFE				-750
#define LASER_PART_SPEED			0

#define BOMB_MAX_PARTS				250
#define BOMB_PART_LIFE				-16000
#define BOMB_PART_SPEED				200

#define DEBRIS_MAX_PARTS			250
#define DEBRIS_PART_LIFE			-2000
#define DEBRIS_PART_SPEED			30

#define STATIC_SMOKE_MAX_PARTS	1000
#define STATIC_SMOKE_PART_LIFE	-3200
#define STATIC_SMOKE_PART_SPEED	1000

#define REACTOR_MAX_PARTS			500

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
