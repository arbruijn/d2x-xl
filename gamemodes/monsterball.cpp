#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"
#include "error.h"
#include "text.h"
#include "interp.h"
#include "segmath.h"
#include "fireball.h"
#include "objeffects.h"
#include "dropobject.h"
#include "network.h"
#include "monsterball.h"

//------------------------------------------------------------------------------

void RemoveMonsterball (void)
{
if (gameData.hoardData.pMonsterBall) {
	ReleaseObject (OBJ_IDX (gameData.hoardData.pMonsterBall));
	gameData.hoardData.pMonsterBall = NULL;
	}
}

//------------------------------------------------------------------------------

int32_t CreateMonsterball (void)
{
	int16_t	nDropSeg, nObject;

RemoveMonsterball ();
if (!(IsMultiGame && (gameData.appData.GameMode (GM_MONSTERBALL))))
	return 0;
#if 0 //DBG
nDropSeg = gameData.hoardData.nMonsterballSeg;
#else
nDropSeg = gameData.hoardData.nMonsterballSeg;
ResetMonsterball (false);
if (nDropSeg >= 0) 
	gameData.hoardData.nMonsterballSeg = nDropSeg;
else {
	nDropSeg = ChooseDropSegment (NULL, NULL, EXEC_DROP);
	gameData.hoardData.vMonsterballPos = SEGMENT (nDropSeg)->Center ();
	}
#endif
if (nDropSeg >= 0) {
	nObject = DropPowerup (OBJ_POWERUP, POW_MONSTERBALL, -1, 0, CFixVector::ZERO, gameData.hoardData.vMonsterballPos, nDropSeg);
	if ((nObject >= 0) && gameData.renderData.monsterball) {
		gameData.renderData.monsterball->SetupPulse (0.005f, 0.9f);
		gameData.renderData.monsterball->SetupSurface (gameData.renderData.monsterball->Pulse (), &gameData.hoardData.monsterball.bm);
		gameData.hoardData.pMonsterBall = OBJECT (nObject);
		gameData.hoardData.pMonsterBall->SetType (OBJ_MONSTERBALL);
		gameData.hoardData.pMonsterBall->SetLife (IMMORTAL_TIME);
		gameData.hoardData.pMonsterBall->Position () =  gameData.hoardData.vMonsterballPos;
		gameData.hoardData.pMonsterBall->mType.physInfo.mass = I2X (10);
		gameData.hoardData.pMonsterBall->mType.physInfo.thrust.SetZero ();
		gameData.hoardData.pMonsterBall->mType.physInfo.rotThrust.SetZero ();
		gameData.hoardData.pMonsterBall->mType.physInfo.velocity.SetZero ();
		gameData.hoardData.nLastHitter = -1;
		gameData.hoardData.pMonsterBall->CreateAppearanceEffect ();
		return 1;
		}
	}
#if !DBG
Warning (TXT_NO_MONSTERBALL);
#endif
if (IAmGameHost ())
	gameData.appData.nGameMode &= ~GM_MONSTERBALL;
return 0;
}

//------------------------------------------------------------------------------

CObject* FindMonsterball (void)
{
if (!gameData.hoardData.pMonsterBall) {
	CObject*	pObj;

	FORALL_POWERUP_OBJS (pObj)
		if ((pObj->info.nType == OBJ_POWERUP) && (pObj->info.nId == POW_MONSTERBALL)) 
			return gameData.hoardData.pMonsterBall = pObj;

	FORALL_ACTOR_OBJS (pObj)
		if (pObj->info.nType == OBJ_MONSTERBALL) 
			return gameData.hoardData.pMonsterBall = pObj;
	}
return gameData.hoardData.pMonsterBall;
}

//------------------------------------------------------------------------------

int32_t ResetMonsterball (bool bCreate)
{
gameData.hoardData.pMonsterBall = NULL;
gameData.hoardData.nMonsterballSeg = -1;
gameData.hoardData.nLastHitter = -1;

CObject*	pObj = FindMonsterball ();
if (!pObj)
	return 0;

gameData.hoardData.nMonsterballSeg = pObj->info.nSegment;
gameData.hoardData.vMonsterballPos = OBJPOS (pObj)->vPos;
RemoveMonsterball ();

#if 1 //!DBG
if (!IAmGameHost ())
	return 0;
#endif
if (bCreate) {
	if (!CreateMonsterball ())
		return 0;
	MultiSendMonsterball (1, 1);
	}
return 1;
}

//	-----------------------------------------------------------------------------

int32_t CheckMonsterballScore (void)
{
if (!(gameData.appData.GameMode (GM_MONSTERBALL)))
	return 0;
if (!gameData.hoardData.pMonsterBall)
	return 0;
if (gameData.hoardData.nLastHitter != LOCALPLAYER.nObject)
	return 0;
uint8_t segFunc = SEGMENT (gameData.hoardData.pMonsterBall->info.nSegment)->m_function;
if ((segFunc != SEGMENT_FUNC_GOAL_BLUE) && (segFunc != SEGMENT_FUNC_GOAL_RED))
	return 0;
if ((GetTeam (N_LOCALPLAYER) == TEAM_RED) == (segFunc == SEGMENT_FUNC_GOAL_RED))
	MultiSendCaptureBonus (-N_LOCALPLAYER - 1);
else
	MultiSendCaptureBonus (N_LOCALPLAYER);
gameData.hoardData.pMonsterBall->CreateAppearanceEffect ();
RemoveMonsterball ();
CreateMonsterball ();
MultiSendMonsterball (1, 1);
return 1;
}

//	-----------------------------------------------------------------------------

int16_t nMonsterballForces [100];

int16_t nMonsterballPyroForce;

void SetMonsterballForces (void)
{
	tMonsterballForce* pForce = extraGameInfo [IsMultiGame].monsterball.forces;

memset (nMonsterballForces, 0, sizeof (nMonsterballForces));
for (int32_t i = 0; i < MAX_MONSTERBALL_FORCES - 1; i++, pForce++)
	nMonsterballForces [pForce->nWeaponId] = pForce->nForce;
nMonsterballPyroForce = pForce->nForce;
gameData.objData.pwrUp.info [POW_MONSTERBALL].size =
	(gameData.objData.pwrUp.info [POW_SHIELD_BOOST].size * extraGameInfo [IsMultiGame].monsterball.nSizeMod) / 2;
}

//------------------------------------------------------------------------------
//eof
