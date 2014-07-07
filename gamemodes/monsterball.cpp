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
if (gameData.hoard.monsterballP) {
	ReleaseObject (OBJ_IDX (gameData.hoard.monsterballP));
	gameData.hoard.monsterballP = NULL;
	}
}

//------------------------------------------------------------------------------

int32_t CreateMonsterball (void)
{
	int16_t	nDropSeg, nObject;

RemoveMonsterball ();
if (!(IsMultiGame && (gameData.app.GameMode (GM_MONSTERBALL))))
	return 0;
#if 0 //DBG
nDropSeg = gameData.hoard.nMonsterballSeg;
#else
if (gameData.hoard.nMonsterballSeg >= 0)
	nDropSeg = gameData.hoard.nMonsterballSeg;
else {
	nDropSeg = ChooseDropSegment (NULL, NULL, EXEC_DROP);
	gameData.hoard.vMonsterballPos = SEGMENTS [nDropSeg].Center ();
	}
#endif
if (nDropSeg >= 0) {
	ResetMonsterball (false);
	gameData.hoard.nMonsterballSeg = nDropSeg;
	nObject = DropPowerup (OBJ_POWERUP, POW_MONSTERBALL, -1, 0, CFixVector::ZERO, gameData.hoard.vMonsterballPos, nDropSeg);
	if (nObject >= 0) {
		gameData.render.monsterball.SetupPulse (0.005f, 0.9f);
		gameData.render.monsterball.SetPulse (gameData.render.monsterball.Pulse ());
		gameData.hoard.monsterballP = OBJECTS + nObject;
		gameData.hoard.monsterballP->SetType (OBJ_MONSTERBALL);
		gameData.hoard.monsterballP->mType.physInfo.mass = I2X (10);
		gameData.hoard.monsterballP->mType.physInfo.thrust.SetZero ();
		gameData.hoard.monsterballP->mType.physInfo.rotThrust.SetZero ();
		gameData.hoard.monsterballP->mType.physInfo.velocity.SetZero ();
		gameData.hoard.nLastHitter = -1;
		gameData.hoard.monsterballP->CreateAppearanceEffect ();
		return 1;
		}
	}
#if !DBG
Warning (TXT_NO_MONSTERBALL);
#endif
if (IAmGameHost ())
	gameData.app.nGameMode &= ~GM_MONSTERBALL;
return 0;
}

//------------------------------------------------------------------------------

CObject* FindMonsterball (void)
{
if (!gameData.hoard.monsterballP) {
	CObject*	objP;

	FORALL_STATIC_OBJS (objP)
		if ((objP->info.nType == OBJ_MONSTERBALL) || ((objP->info.nType == OBJ_POWERUP) && (objP->info.nId == POW_MONSTERBALL))) {
			gameData.hoard.monsterballP = objP;
			break;
			}
	}
return gameData.hoard.monsterballP;
}

//------------------------------------------------------------------------------

int32_t ResetMonsterball (bool bCreate)
{
	CObject*	objP;

if (gameData.hoard.monsterballP) {
	//ReleaseObject (gameData.hoard.monsterballP->Index ());
	gameData.hoard.monsterballP = NULL;
	}	
gameData.hoard.nMonsterballSeg = -1;
gameData.hoard.nLastHitter = -1;
FORALL_STATIC_OBJS (objP)
	if ((objP->info.nType == OBJ_MONSTERBALL) || ((objP->info.nType == OBJ_POWERUP) && (objP->info.nId == POW_MONSTERBALL))) {
		if (gameData.hoard.nMonsterballSeg < 0) {
			gameData.hoard.nMonsterballSeg = objP->info.nSegment;
			gameData.hoard.vMonsterballPos = OBJPOS (objP)->vPos;
			}
		ReleaseObject (objP->Index ());
		}
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
if (!(gameData.app.GameMode (GM_MONSTERBALL)))
	return 0;
if (!gameData.hoard.monsterballP)
	return 0;
if (gameData.hoard.nLastHitter != LOCALPLAYER.nObject)
	return 0;
uint8_t segFunc = SEGMENTS [gameData.hoard.monsterballP->info.nSegment].m_function;
if ((segFunc != SEGMENT_FUNC_GOAL_BLUE) && (segFunc != SEGMENT_FUNC_GOAL_RED))
	return 0;
if ((GetTeam (N_LOCALPLAYER) == TEAM_RED) == (segFunc == SEGMENT_FUNC_GOAL_RED))
	MultiSendCaptureBonus (-N_LOCALPLAYER - 1);
else
	MultiSendCaptureBonus (N_LOCALPLAYER);
gameData.hoard.monsterballP->CreateAppearanceEffect ();
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
	tMonsterballForce* forceP = extraGameInfo [IsMultiGame].monsterball.forces;

memset (nMonsterballForces, 0, sizeof (nMonsterballForces));
for (int32_t i = 0; i < MAX_MONSTERBALL_FORCES - 1; i++, forceP++)
	nMonsterballForces [forceP->nWeaponId] = forceP->nForce;
nMonsterballPyroForce = forceP->nForce;
gameData.objs.pwrUp.info [POW_MONSTERBALL].size =
	(gameData.objs.pwrUp.info [POW_SHIELD_BOOST].size * extraGameInfo [IsMultiGame].monsterball.nSizeMod) / 2;
}

//------------------------------------------------------------------------------
//eof
