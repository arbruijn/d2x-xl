#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "inferno.h"
#include "error.h"
#include "particles.h"
#include "laser.h"
#include "fireball.h"
#include "network.h"
#include "newdemo.h"
#include "object.h"
#include "objsmoke.h"

#define SHIP_MAX_PARTS		50
#define BOT_MAX_PARTS		50
#define REACTOR_MAX_PARTS	50
#define DEBRIS_MAX_PARTS	50
#define MSL_MAX_PARTS		500

#define PLR_PART_LIFE	-4000
#define PLR_PART_SPEED	40
#define OBJ_PART_LIFE	-4000
#define OBJ_PART_SPEED	40
#define MSL_PART_LIFE	-3000
#define MSL_PART_SPEED	30
#define DEBRIS_PART_LIFE	-2000
#define DEBRIS_PART_SPEED	30

//------------------------------------------------------------------------------

void KillPlayerSmoke (int i)
{
KillObjectSmoke (gameData.multi.players [i].nObject);
}

//------------------------------------------------------------------------------

void ResetPlayerSmoke (void)
{
	int	i;

for (i = 0; i < MAX_PLAYERS; i++)
	KillPlayerSmoke (i);
}

//------------------------------------------------------------------------------

void InitObjectSmoke (void)
{
memset (gameData.smoke.objects, 0xff, sizeof (gameData.smoke.objects));
memset (gameData.smoke.objExplTime, 0xff, sizeof (gameData.smoke.objExplTime));
}

//------------------------------------------------------------------------------

void ResetObjectSmoke (void)
{
	int	i;

for (i = 0; i < MAX_OBJECTS; i++)
	KillObjectSmoke (i);
InitObjectSmoke ();
}

//------------------------------------------------------------------------------

static inline int RandN (int n)
{
if (!n)
	return 0;
return (int) ((double) rand () * (double) n / (double) RAND_MAX);
}

//------------------------------------------------------------------------------

void CreateDamageExplosion (int h, int i)
{
if (EGI_FLAG (bDamageExplosions, 1, 0, 0) && 
	 (gameStates.app.nSDLTicks - gameData.smoke.objExplTime [i] > 100)) {
	gameData.smoke.objExplTime [i] = gameStates.app.nSDLTicks;
	if (!RandN (11 - h))
		CreateSmallFireballOnObject (gameData.objs.objects + i, F1_0, 1);
	}
}

//------------------------------------------------------------------------------
#if 0
void CreateThrusterFlames (tObject *objP)
{
	static int nThrusters = -1;

	vmsVector	pos, dir = objP->position.mOrient.fVec;
	int			d, j;
	tCloud		*pCloud;

VmVecNegate (&dir);
if (nThrusters < 0) {
	nThrusters = 
		CreateSmoke (&objP->position.vPos, &dir, objP->nSegment, 2, -2000, 20000,
						 gameOpts->render.smoke.bSyncSizes ? -1 : gameOpts->render.smoke.nSize [1],
						 2, -2000, PLR_PART_SPEED * 50, 5, OBJ_IDX (objP));
	gameData.smoke.objects [OBJ_IDX (objP)] = nThrusters;
	}
else
	SetSmokeDir (nThrusters, &dir);
d = 8 * objP->size / 40;
for (j = 0; j < 2; j++)
	if (pCloud = GetCloud (nThrusters, j)) {
		VmVecScaleAdd (&pos, &objP->position.vPos, &objP->position.mOrient.fVec, -objP->size);
		VmVecScaleInc (&pos, &objP->position.mOrient.rVec, j ? d : -d);
		VmVecScaleInc (&pos, &objP->position.mOrient.uVec,  -objP->size / 25);
		SetCloudPos (pCloud, &pos);
		}
}
#endif
//------------------------------------------------------------------------------

extern tSmoke	smoke [];

void DoPlayerSmoke (tObject *objP, int i)
{
	int			h, j, d, nParts, nType;
	float			nScale;
	tCloud		*pCloud;
	vmsVector	vPos [2], fn, mn;
	static int	bForward = 1;

if (i < 0)
	i = objP->id;
if (gameData.multi.players [i].flags & PLAYER_FLAGS_CLOAKED) {
	KillObjectSmoke (i);
	return;
	}
j = OBJ_IDX (objP);
if (gameOpts->render.smoke.bDecreaseLag && (i == gameData.multi.nLocalPlayer)) {
	fn = objP->position.mOrient.fVec;
	VmVecSub (&mn, &objP->position.vPos, &objP->last_pos);
	VmVecNormalize (&fn);
	VmVecNormalize (&mn);
	d = VmVecDot (&fn, &mn);
	if (d >= -F1_0 / 2) 
		bForward = 1;
	else {
		if (bForward) {
			if ((h = gameData.smoke.objects [j]) >= 0) {
				KillObjectSmoke (i);
				DestroySmoke (h);
				}
			bForward = 0;
			nScale = 0;
			return;
			}
		}
	}
#if 0
if (EGI_FLAG (bThrusterFlames, 1, 1, 0)) {
	if ((a <= F1_0 / 4) && (a || !gameStates.input.bControlsSkipFrame))	//no thruster flames if moving backward
		DropAfterburnerBlobs (objP, 2, i2f (1), -1, gameData.objs.console, 1); //F1_0 / 4);
	}
#endif
if ((gameData.app.nGameMode & GM_NETWORK) && !gameData.multi.players [i].connected)
	nParts = 0;
else if (objP->flags & (OF_SHOULD_BE_DEAD | OF_DESTROYED))
	nParts = 0;
else if ((i == gameData.multi.nLocalPlayer) && (gameStates.app.bPlayerIsDead || (gameData.multi.players [i].shields < 0)))
	nParts = 0;
else {
	h = f2ir (gameData.multi.players [i].shields);
	nParts = 10 - h / 5;
	nScale = f2fl (objP->size);
	if (h <= 25)
		nScale /= 1.5;
	else if (h <= 50)
		nScale /= 2;
	else
		nScale /= 3;
	if (nParts <= 0) {
		nType = 2;
		//nParts = (gameStates.entropy.nTimeLastMoved < 0) ? 250 : 125;
		}
	else {
		CreateDamageExplosion (nParts, j);
		nType = (h > 25);
		nParts *= 25;
		nParts += 75;
		}
	nParts = (gameStates.entropy.nTimeLastMoved < 0) ? SHIP_MAX_PARTS * 2 : SHIP_MAX_PARTS;
	if (SHOW_SMOKE && nParts && gameOpts->render.smoke.bPlayers) {
		if (!gameOpts->render.smoke.bSyncSizes) {
			nParts = -MAX_PARTICLES (nParts, gameOpts->render.smoke.nDens [1]);
			nScale = PARTICLE_SIZE (gameOpts->render.smoke.nSize [1], nScale);
			}
		if (0 > (h = gameData.smoke.objects [j])) {
			//LogErr ("creating tPlayer smoke\n");
			h = gameData.smoke.objects [j] = 
				CreateSmoke (&objP->position.vPos, NULL, objP->nSegment, 2, nParts, nScale,
								 gameOpts->render.smoke.bSyncSizes ? -1 : gameOpts->render.smoke.nSize [1],
								 2, PLR_PART_LIFE / (nType + 1), PLR_PART_SPEED, nType, j);
			}
		else {
			SetSmokeType (h, nType);
			SetSmokePartScale (h, nScale);
			SetSmokeDensity (h, nParts, gameOpts->render.smoke.bSyncSizes ? -1 : gameOpts->render.smoke.nSize [1]);
			}
		CalcShipThrusterPos (objP, vPos);
		for (j = 0; j < 2; j++)
			if (pCloud = GetCloud (h, j))
				SetCloudPos (pCloud, vPos + j);
		return;
		}
	}
KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoRobotSmoke (tObject *objP)
{
	int			h = -1, i, nShields = 0, nParts;
	float			nScale;
	vmsVector	pos;

if (!(SHOW_SMOKE && gameOpts->render.smoke.bRobots))
	return;
i = (int) (objP - gameData.objs.objects);
if ((objP->shields < 0) || (objP->flags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else {
	nShields = f2ir (RobotDefaultShields (objP));
	h = f2ir (objP->shields) * 100 / nShields;
	}
if (h < 0)
	return;	
nParts = 10 - h / 5;
if (nParts > 0) {
	if (nShields > 4000)
		nShields = 4000;
	else if (nShields < 1000)
		nShields = 1000;
	CreateDamageExplosion (nParts, i);
	//nParts *= nShields / 10;
	nParts = BOT_MAX_PARTS;
	nScale = (float) sqrt (8.0 / f2fl (objP->size));
	nScale *= 1.0f + h / 25.0f;
	if (!gameOpts->render.smoke.bSyncSizes) {
		nParts = -MAX_PARTICLES (nParts, gameOpts->render.smoke.nDens [2]);
		nScale = PARTICLE_SIZE (gameOpts->render.smoke.nSize [2], nScale);
		}
	if (gameData.smoke.objects [i] < 0) {
		//LogErr ("creating robot %d smoke\n", i);
		gameData.smoke.objects [i] = CreateSmoke (&objP->position.vPos, NULL, objP->nSegment, 1, nParts, nScale,
																gameOpts->render.smoke.bSyncSizes ? -1 : gameOpts->render.smoke.nSize [2],
																1, OBJ_PART_LIFE, OBJ_PART_SPEED, 0, i);
		}
	else {
		SetSmokePartScale (gameData.smoke.objects [i], nScale);
		SetSmokeDensity (gameData.smoke.objects [i], nParts, gameOpts->render.smoke.bSyncSizes ? -1 : gameOpts->render.smoke.nSize [2]);
		}
	VmVecScaleAdd (&pos, &objP->position.vPos, &objP->position.mOrient.fVec, -objP->size / 2);
	SetSmokePos (gameData.smoke.objects [i], &pos);
	}
else 
	KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoReactorSmoke (tObject *objP)
{
	int			h = -1, i, nShields = 0, nParts;
	vmsVector	vDir, vPos;

if (!(SHOW_SMOKE && gameOpts->render.smoke.bRobots))
	return;
i = (int) (objP - gameData.objs.objects);
if ((objP->shields < 0) || (objP->flags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else {
	nShields = f2ir (gameData.bots.info [gameStates.app.bD1Mission][objP->id].strength);
	h = f2ir (objP->shields) * 100 / nShields;
	}
if (h < 0)
	h = 0;	
nParts = 10 - h / 10;
if (nParts > 0) {
	nParts = REACTOR_MAX_PARTS;
	if (gameData.smoke.objects [i] < 0) {
		//LogErr ("creating robot %d smoke\n", i);
		gameData.smoke.objects [i] = CreateSmoke (&objP->position.vPos, NULL, objP->nSegment, 1, nParts, 1.0,
																-1, 1, OBJ_PART_LIFE * 2, OBJ_PART_SPEED * 8, 0, i);
		}
	else {
		SetSmokePartScale (gameData.smoke.objects [i], 0.5);
		SetSmokeDensity (gameData.smoke.objects [i], nParts, -1);
		vDir.p.x = d_rand () - F1_0 / 4;
		vDir.p.y = d_rand () - F1_0 / 4;
		vDir.p.z = d_rand () - F1_0 / 4;
		VmVecNormalize (&vDir);
		VmVecScaleAdd (&vPos, &objP->position.vPos, &vDir, -objP->size / 2);
		SetSmokePos (gameData.smoke.objects [i], &vPos);
		}
	}
else 
	KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoMissileSmoke (tObject *objP)
{
	int			nParts, i;
	float			nScale = 1.5f;
	vmsVector	pos;

if (!(SHOW_SMOKE && gameOpts->render.smoke.bMissiles))
	return;
i = (int) (objP - gameData.objs.objects);
if ((objP->shields < 0) || (objP->flags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else 
#if 1
	nParts = MSL_MAX_PARTS;
#else
	nParts = (objP->id == EARTHSHAKER_ID) ? 1500 : 
				(objP->id == MEGAMSL_ID) ? 1400 : 
				(objP->id == SMARTMSL_ID) ? 1300 : 
				1200;
#endif
if (nParts) {
	if (gameData.smoke.objects [i] < 0) {
		//LogErr ("creating missile %d smoke\n", i);
	if (!gameOpts->render.smoke.bSyncSizes) {
		nParts = -MAX_PARTICLES (nParts, gameOpts->render.smoke.nDens [3]);
		nScale = PARTICLE_SIZE (gameOpts->render.smoke.nSize [3], nScale);
		}
	gameData.smoke.objects [i] = CreateSmoke (&objP->position.vPos, NULL, objP->nSegment, 1, nParts, nScale,
															gameOpts->render.smoke.bSyncSizes ? -1 : gameOpts->render.smoke.nSize [3],
															1, (gameOpts->render.smoke.nLife [3] + 1) * MSL_PART_LIFE, MSL_PART_SPEED, 1, i);
	}
	VmVecScaleAdd (&pos, &objP->position.vPos, &objP->position.mOrient.fVec, -objP->size);
	SetSmokePos (gameData.smoke.objects [i], &pos);
	}
else 
	KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoDebrisSmoke (tObject *objP)
{
	int			nParts, i;
	vmsVector	pos;

if (!(SHOW_SMOKE && gameOpts->render.smoke.bDebris))
	return;
i = (int) (objP - gameData.objs.objects);
if ((objP->shields < 0) || (objP->flags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else 
	nParts = -DEBRIS_MAX_PARTS;
if (nParts) {
	if (gameData.smoke.objects [i] < 0) {
		//LogErr ("creating missile %d smoke\n", i);
		gameData.smoke.objects [i] = CreateSmoke (&objP->position.vPos, NULL, objP->nSegment, 1, nParts / 2, 1.5,
																-1, 1, DEBRIS_PART_LIFE, DEBRIS_PART_SPEED, 2, i);
		}
	VmVecScaleAdd (&pos, &objP->position.vPos, &objP->position.mOrient.fVec, -objP->size);
	SetSmokePos (gameData.smoke.objects [i], &pos);
	}
else 
	KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoObjectSmoke (tObject *objP)
{
int t = objP->nType;
#if 0
if (extraGameInfo [0].bShadows && (gameStates.render.nShadowPass < 3))
	return;
#endif
if (t == OBJ_PLAYER)
	DoPlayerSmoke (objP, -1);
else if (t == OBJ_ROBOT)
	DoRobotSmoke (objP);
else if (t == OBJ_CNTRLCEN)
	DoReactorSmoke (objP);
else if (t == OBJ_WEAPON) {
	if (bIsMissile [objP->id])
		DoMissileSmoke (objP);
	}
else if (t == OBJ_DEBRIS)
	DoDebrisSmoke (objP);
}

//------------------------------------------------------------------------------

void PlayerSmokeFrame (void)
{
	int	i;

if (!gameOpts->render.smoke.bPlayers)
	return;
for (i = 0; i < gameData.multi.nPlayers; i++)
	DoPlayerSmoke (gameData.objs.objects + gameData.multi.players [i].nObject, i);
}

//------------------------------------------------------------------------------

void RobotSmokeFrame (void)
{
	int		i;
	tObject	*objP;

if (!SHOW_SMOKE)
	return;
if (!gameOpts->render.smoke.bRobots)
	return;
for (i = 0, objP = gameData.objs.objects; i <= gameData.objs.nLastObject; i++, objP++)
	if (objP->nType == OBJ_NONE)
		KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoSmokeFrame (void)
{
#if SHADOWS
if (gameStates.render.nShadowPass > 1)
	return;
#endif
#ifdef _DEBUG
if (!gameStates.render.bExternalView)
#else
if (!gameStates.render.bExternalView && (!IsMultiGame || IsCoopGame || EGI_FLAG (bEnableCheats, 0, 0, 0)))
#endif
	DoPlayerSmoke (gameData.objs.viewer, gameData.multi.nLocalPlayer);
RobotSmokeFrame ();
if (SHOW_SMOKE)
	MoveSmoke ();
}

//------------------------------------------------------------------------------
