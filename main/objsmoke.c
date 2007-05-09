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
#include "automap.h"

#define SHIP_MAX_PARTS				50
#define PLR_PART_LIFE				-4000
#define PLR_PART_SPEED				40

#define BOT_MAX_PARTS				50
#define BOT_PART_LIFE				-6000
#define BOT_PART_SPEED				300

#define MSL_MAX_PARTS				500
#define MSL_PART_LIFE				-3000
#define MSL_PART_SPEED				30

#define BOMB_MAX_PARTS				60
#define BOMB_PART_LIFE				-16000
#define BOMB_PART_SPEED				200

#define DEBRIS_MAX_PARTS			25
#define DEBRIS_PART_LIFE			-2000
#define DEBRIS_PART_SPEED			30

#define STATIC_SMOKE_MAX_PARTS	100
#define STATIC_SMOKE_PART_LIFE	-3200
#define STATIC_SMOKE_PART_SPEED	1000

#define REACTOR_MAX_PARTS	50

//------------------------------------------------------------------------------

#ifdef _DEBUG

void KillObjectSmoke (int i)
{
if ((i >= 0) && (gameData.smoke.objects [i] >= 0)) {
	SetSmokeLife (gameData.smoke.objects [i], 0);
	gameData.smoke.objects [i] = -1;
	}
}

#endif

//------------------------------------------------------------------------------

void KillPlayerSmoke (int i)
{
KillObjectSmoke (gameData.multiplayer.players [i].nObject);
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
memset (gameData.smoke.objects, 0xff, sizeof (*gameData.smoke.objects) * MAX_OBJECTS);
memset (gameData.smoke.objExplTime, 0xff, sizeof (*gameData.smoke.objExplTime) * MAX_OBJECTS);
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
if ((gameData.multiplayer.players [i].flags & PLAYER_FLAGS_CLOAKED) ||
	 (gameStates.render.automap.bDisplay && IsMultiGame && !AM_SHOW_PLAYERS)) {
	KillObjectSmoke (i);
	return;
	}
j = OBJ_IDX (objP);
if (gameOpts->render.smoke.bDecreaseLag && (i == gameData.multiplayer.nLocalPlayer)) {
	fn = objP->position.mOrient.fVec;
	VmVecSub (&mn, &objP->position.vPos, &objP->vLastPos);
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
if ((gameData.app.nGameMode & GM_NETWORK) && !gameData.multiplayer.players [i].connected)
	nParts = 0;
else if (objP->flags & (OF_SHOULD_BE_DEAD | OF_DESTROYED))
	nParts = 0;
else if ((i == gameData.multiplayer.nLocalPlayer) && (gameStates.app.bPlayerIsDead || (gameData.multiplayer.players [i].shields < 0)))
	nParts = 0;
else {
	h = f2ir (gameData.multiplayer.players [i].shields);
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
			SetSmokeSpeed (gameData.smoke.objects [i], 
								(objP->mType.physInfo.velocity.p.x || objP->mType.physInfo.velocity.p.y || objP->mType.physInfo.velocity.p.z) ?
								PLR_PART_SPEED : PLR_PART_SPEED * 3 / 4);
			}
		CalcShipThrusterPos (objP, vPos);
		for (j = 0; j < 2; j++)
			if ((pCloud = GetCloud (h, j)))
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
i = OBJ_IDX (objP);
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
																1, BOT_PART_LIFE, BOT_PART_SPEED, 0, i);
		}
	else {
		SetSmokePartScale (gameData.smoke.objects [i], nScale);
		SetSmokeDensity (gameData.smoke.objects [i], nParts, gameOpts->render.smoke.bSyncSizes ? -1 : gameOpts->render.smoke.nSize [2]);
		SetSmokeSpeed (gameData.smoke.objects [i], 
							(objP->mType.physInfo.velocity.p.x || objP->mType.physInfo.velocity.p.y || objP->mType.physInfo.velocity.p.z) ?
							BOT_PART_SPEED : BOT_PART_SPEED * 2 / 3);
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
i = OBJ_IDX (objP);
if ((objP->shields < 0) || (objP->flags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else {
	nShields = f2ir (gameData.bots.info [gameStates.app.bD1Mission][objP->id].strength);
	h = nShields ? f2ir (objP->shields) * 100 / nShields : 0;
	}
if (h < 0)
	h = 0;	
nParts = 10 - h / 10;
if (nParts > 0) {
	nParts = REACTOR_MAX_PARTS;
	if (gameData.smoke.objects [i] < 0) {
		//LogErr ("creating robot %d smoke\n", i);
		gameData.smoke.objects [i] = CreateSmoke (&objP->position.vPos, NULL, objP->nSegment, 1, nParts, 3.0,
																-1, 1, BOT_PART_LIFE * 2, BOT_PART_SPEED, 0, i);
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
i = OBJ_IDX (objP);
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
i = OBJ_IDX (objP);
if ((objP->shields < 0) || (objP->flags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)) ||
	 (gameOpts->render.nDebrisLife && (nDebrisLife [gameOpts->render.nDebrisLife] * F1_0 - objP->lifeleft > 10 * F1_0)))
	nParts = 0;
else 
	nParts = -DEBRIS_MAX_PARTS;
if (nParts) {
	if (gameData.smoke.objects [i] < 0) {
		gameData.smoke.objects [i] = CreateSmoke (&objP->position.vPos, NULL, objP->nSegment, 1, nParts / 2,
																-PARTICLE_SIZE (3, 2), -1, 1, DEBRIS_PART_LIFE, DEBRIS_PART_SPEED, 2, i);
		}
	VmVecScaleAdd (&pos, &objP->position.vPos, &objP->position.mOrient.fVec, -objP->size);
	SetSmokePos (gameData.smoke.objects [i], &pos);
	}
else 
	KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoBombSmoke (tObject *objP)
{
	int			nParts, i;
	vmsVector	pos, offs;

if (gameStates.app.bNostalgia || !gameStates.app.bHaveExtraGameInfo [IsMultiGame])
	return;
i = OBJ_IDX (objP);
if ((objP->shields < 0) || (objP->flags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else 
	nParts = -BOMB_MAX_PARTS;
if (nParts) {
	if (gameData.smoke.objects [i] < 0) {
		gameData.smoke.objects [i] = CreateSmoke (&objP->position.vPos, NULL, objP->nSegment, 1, nParts,
																-PARTICLE_SIZE (3, 0.5f), -1, 3, BOMB_PART_LIFE, BOMB_PART_SPEED, 0, i);
		}
	offs.p.x = (F1_0 / 4 - d_rand ()) * ((d_rand () & 15) + 16);
	offs.p.y = (F1_0 / 4 - d_rand ()) * ((d_rand () & 15) + 16);
	offs.p.z = (F1_0 / 4 - d_rand ()) * ((d_rand () & 15) + 16);
	VmVecAdd (&pos, &objP->position.vPos, &offs);
	SetSmokePos (gameData.smoke.objects [i], &pos);
	}
else 
	KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoStaticSmoke (tObject *objP)
{
	int			nLife, nParts, nSpeed, nSize, nDrift, i, j;
	vmsVector	pos, offs, dir;
	tTrigger		*trigP;

if (!(SHOW_SMOKE && gameOpts->render.smoke.bStatic))
	return;
objP->renderType = RT_NONE;
i = (int) OBJ_IDX (objP);
if (gameData.smoke.objects [i] < 0) {
	trigP = FindObjTrigger (OBJ_IDX (objP), TT_SMOKE_LIFE, -1);
	nLife = (trigP && trigP->value) ? trigP->value : 5;
	trigP = FindObjTrigger (OBJ_IDX (objP), TT_SMOKE_SPEED, -1);
	j = (trigP && trigP->value) ? trigP->value : 5;
	for (nSpeed = 0; j; j--)
		nSpeed += j;
	trigP = FindObjTrigger (OBJ_IDX (objP), TT_SMOKE_DENS, -1);
	nParts = (trigP && trigP->value) ? trigP->value * 25 : STATIC_SMOKE_MAX_PARTS;
	trigP = FindObjTrigger (OBJ_IDX (objP), TT_SMOKE_SIZE, -1);
	j = (trigP && trigP->value) ? trigP->value : 5;
	if (gameOpts->render.smoke.bDisperse)
		for (nSize = 0; j; j--)
			nSize += j;
	else
		nSize = j + 1;
	trigP = FindObjTrigger (OBJ_IDX (objP), TT_SMOKE_DRIFT, -1);
	nDrift = (trigP && trigP->value) ? trigP->value * 200 : nSpeed * 50;
	VmVecCopyScale (&dir, &objP->position.mOrient.fVec, F1_0 * nSpeed);
	gameData.smoke.objects [i] = CreateSmoke (&objP->position.vPos, &dir, 
															objP->nSegment, 1, -nParts, -PARTICLE_SIZE (nSize, 2.0f), 
															-1, 3, STATIC_SMOKE_PART_LIFE * nLife, nDrift, 2, i);
	}
trigP = FindObjTrigger (OBJ_IDX (objP), TT_SMOKE_DRIFT, -1);
i = (trigP && trigP->value) ? trigP->value * 3 : 15;
trigP = FindObjTrigger (OBJ_IDX (objP), TT_SMOKE_SIZE, -1);
i += ((trigP && trigP->value) ? trigP->value * 3 : 15) / 2;
i /= 2;
j = i - i / 2;
i /= 2;
offs.p.x = (F1_0 / 4 - d_rand ()) * (d_rand () % j + i);
offs.p.y = (F1_0 / 4 - d_rand ()) * (d_rand () % j + i);
offs.p.z = (F1_0 / 4 - d_rand ()) * (d_rand () % j + i);
VmVecAdd (&pos, &objP->position.vPos, &offs);
SetSmokePos (gameData.smoke.objects [i], &pos);
}

//------------------------------------------------------------------------------

int DoObjectSmoke (tObject *objP)
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
else if (t == OBJ_SMOKE)
	DoStaticSmoke (objP);
else if (t == OBJ_CNTRLCEN)
	DoReactorSmoke (objP);
else if (t == OBJ_WEAPON) {
	if (bIsMissile [objP->id])
		DoMissileSmoke (objP);
	else if (!COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0) && (objP->id == PROXMINE_ID))
		DoBombSmoke (objP);
	else
		return 0;
	}
else if (t == OBJ_DEBRIS)
	DoDebrisSmoke (objP);
else
	return 0;
return 1;
}

//------------------------------------------------------------------------------

void PlayerSmokeFrame (void)
{
	int	i;

if (!gameOpts->render.smoke.bPlayers)
	return;
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	DoPlayerSmoke (gameData.objs.objects + gameData.multiplayer.players [i].nObject, i);
}

//------------------------------------------------------------------------------

void ObjectSmokeFrame (void)
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

void StaticSmokeFrame (void)
{
	tObject	*objP = gameData.objs.objects;
	int		i;

if (!SHOW_SMOKE)
	return;
if (!gameOpts->render.smoke.bStatic)
	return;
for (i = gameData.objs.nLastObject + 1; i; i--, objP++)
	if (objP->nType == OBJ_SMOKE)
		DoStaticSmoke (objP);
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
	DoPlayerSmoke (gameData.objs.viewer, gameData.multiplayer.nLocalPlayer);
ObjectSmokeFrame ();
StaticSmokeFrame ();
//if (SHOW_SMOKE)
	MoveSmoke ();
}

//------------------------------------------------------------------------------
