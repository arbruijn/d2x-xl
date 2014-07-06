#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "descent.h"
#include "error.h"
#include "timer.h"
#include "u_mem.h"
#include "interp.h"
#include "lightning.h"
#include "network.h"
#include "rendermine.h"
#include "objeffects.h"
#include "objrender.h"
#include "objsmoke.h"
#include "shrapnel.h"
#include "automap.h"
#include "thrusterflames.h"
#include "renderthreads.h"

static CFloatVector smokeColors [] = {
	 {{{0.5f, 0.5f, 0.5f, 2.0f}}},	// alpha == 2.0 means that the particles are red in the beginning
	 {{{0.75f, 0.75f, 0.75f, 2.0f}}},
	 {{{1.0f, 1.0f, 1.0f, 2.0f}}},
	 {{{1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f, -0.25f}}},
	 {{{1.0f, 1.0f, 1.0f, -0.1f}}}
	};

#define SHIP_MAX_PARTS				50
#define PLR_PART_LIFE				-1400
#define PLR_PART_SPEED				50

#define BOT_MAX_PARTS				250
#define BOT_PART_LIFE				-6000
#define BOT_PART_SPEED				300

#define MSL_MAX_PARTS				750
#define MSL_PART_LIFE				-3000
#define MSL_PART_SPEED				100

#define LASER_MAX_PARTS				250
#define LASER_PART_LIFE				-500
#define LASER_PART_SPEED			0

#define BOMB_MAX_PARTS				250
#define BOMB_PART_LIFE				-16000
#define BOMB_PART_SPEED				200

#define DEBRIS_MAX_PARTS			250
#define DEBRIS_PART_LIFE			-2000
#define DEBRIS_PART_SPEED			50

#define FIRE_PART_LIFE				-800

#define REACTOR_MAX_PARTS			500

#define PLAYER_SMOKE					1
#define ROBOT_SMOKE					1
#define MISSILE_SMOKE				1
#define REACTOR_SMOKE				1
#define GATLING_SMOKE				1
#define DEBRIS_SMOKE					1
#define BOMB_SMOKE					1
#define STATIC_SMOKE					1
#define PARTICLE_TRAIL				1

//------------------------------------------------------------------------------

#if DBG

void KillObjectSmoke (int32_t nObject)
{
if ((nObject >= 0) && (particleManager.GetObjectSystem (nObject) >= 0)) {
	if (OBJECTS [nObject].Type () == OBJ_EFFECT)
		audio.DestroyObjectSound (nObject);
	particleManager.SetLife (particleManager.GetObjectSystem (nObject), 0);
	particleManager.SetObjectSystem (nObject, -1);
	shrapnelManager.Destroy (OBJECTS + nObject);
	}
}

#endif

//------------------------------------------------------------------------------

void KillPlayerSmoke (int32_t i)
{
KillObjectSmoke (gameData.multiplayer.players [i].nObject);
}

//------------------------------------------------------------------------------

void ResetPlayerSmoke (void)
{
	int32_t	i;

for (i = 0; i < MAX_PLAYERS; i++)
	KillPlayerSmoke (i);
}

//------------------------------------------------------------------------------

void InitObjectSmoke (void)
{
particleManager.InitObjects ();
}

//------------------------------------------------------------------------------

void ResetObjectSmoke (void)
{
	int32_t	i;

for (i = 0; i < LEVEL_OBJECTS; i++)
	KillObjectSmoke (i);
InitObjectSmoke ();
}

//------------------------------------------------------------------------------

static inline int32_t RandN (int32_t n)
{
if (!n)
	return 0;
return (int32_t) ((double) rand () * (double) n / (double) RAND_MAX);
}

//------------------------------------------------------------------------------

void CreateDamageExplosion (int32_t h, int32_t i)
{
if (EGI_FLAG (bDamageExplosions, 1, 0, 0) &&
	 (gameStates.app.nSDLTicks [0] - *particleManager.ObjExplTime (i) > 100)) {
	*particleManager.ObjExplTime (i) = gameStates.app.nSDLTicks [0];
	if (!RandN (11 - h))
		CreateSmallFireballOnObject (OBJECTS + i, I2X (1), 1);
	}
}

//------------------------------------------------------------------------------
#if 0
void CreateThrusterFlames (CObject *objP)
{
	static int32_t nThrusters = -1;

	CFixVector	coord, dir = objP->info.position.mOrient.mat.dir.f;
	int32_t			d, j;
	tParticleEmitter		*emitterP;

VmVecNegate (&dir);
if (nThrusters < 0) {
	nThrusters =
		particleManager.Create (&objP->info.position.vPos, &dir, objP->info.nSegment, 2, -2000, 20000,
										gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [1],
										2, -2000, PLR_PART_SPEED * 50, LIGHT_PARTICLES, objP->Index (), NULL, 1, -1);
	particleManager.SetObjectSystem (objP->Index ()) = nThrusters;
	}
else
	particleManager.SetDir (nThrusters, &dir);
d = 8 * objP->info.xSize / 40;
for (j = 0; j < 2; j++)
	if (emitterP = GetParticleEmitter (nThrusters, j)) {
		VmVecScaleAdd (&coord, &objP->info.position.vPos, &objP->info.position.mOrient.mat.dir.f, -objP->info.xSize);
		VmVecScaleInc (&coord, &objP->info.position.mOrient.mat.dir.r, j ? d : -d);
		VmVecScaleInc (&coord, &objP->info.position.mOrient.mat.dir.u,  -objP->info.xSize / 25);
		SetParticleEmitterPos (emitterP, &coord, NULL, objP->info.nSegment);
		}
}
#endif

//------------------------------------------------------------------------------

void KillPlayerBullets (CObject *objP)
{
if (objP) {
	int32_t	i = gameData.multiplayer.bulletEmitters [objP->info.nId];

	if ((i >= 0) && (i < MAX_PLAYERS)) {
		particleManager.SetLife (i, 0);
		gameData.multiplayer.bulletEmitters [objP->info.nId] = -1;
		}
	}
}

//------------------------------------------------------------------------------

void KillGatlingSmoke (CObject *objP)
{
if (objP) {
	int32_t	i = gameData.multiplayer.gatlingSmoke [objP->info.nId];

	if ((i >= 0) && (i < MAX_PLAYERS)) {
		particleManager.SetLife (i, 0);
		gameData.multiplayer.gatlingSmoke [objP->info.nId] = -1;
		}
	}
}

//------------------------------------------------------------------------------

#define BULLET_MAX_PARTS	50
#define BULLET_PART_LIFE	-2000
#define BULLET_PART_SPEED	50

void DoPlayerBullets (CObject *objP)
{
if (gameOpts->render.ship.bBullets) {
		int32_t	nModel = objP->ModelId ();
		int32_t	bHires = G3HaveModel (nModel) - 1;

	if (bHires >= 0) {
		RenderModel::CModel	*pm = gameData.models.renderModels [bHires] + nModel;

		if (pm->m_bBullets) {
				int32_t			nPlayer = objP->info.nId;
				int32_t			nGun = EquippedPlayerGun (objP);
				int32_t			bDoEffect = (bHires >= 0) && ((nGun == VULCAN_INDEX) || (nGun == GAUSS_INDEX)) &&
												(gameData.multiplayer.weaponStates [nPlayer].firing [0].nDuration >= GATLING_DELAY);
				int32_t			i = gameData.multiplayer.bulletEmitters [nPlayer];

			if (bDoEffect) {
					int32_t			bSpectate = SPECTATOR (objP);
					tObjTransformation	*posP = bSpectate ? &gameStates.app.playerPos : &objP->info.position;
					CFixVector	vEmitter, vDir;
					CFixMatrix	m, *viewP;

				if (bSpectate) {
					viewP = &m;
					m = posP->mOrient.Transpose();
				}
				else
					viewP = objP->View (0);
				vEmitter = *viewP * pm->m_vBullets;
				vEmitter += posP->vPos;
				vDir = posP->mOrient.m.dir.u;
				vDir.Neg ();
				if (i < 0) {
					gameData.multiplayer.bulletEmitters [nPlayer] =
						particleManager.Create (&vEmitter, &vDir, &posP->mOrient, objP->info.nSegment, 1, BULLET_MAX_PARTS, 15.0f, 
														/*1, 1,*/ 
														BULLET_PART_LIFE, BULLET_PART_SPEED, BULLET_PARTICLES, 0x7fffffff, NULL, 0, -1);
					}
				else {
					particleManager.SetPos (i, &vEmitter, &posP->mOrient, objP->info.nSegment);
					particleManager.SetFadeType (i, -1);
					}
				}
			else {
				if (i >= 0) {
					particleManager.SetLife (i, 0);
					gameData.multiplayer.bulletEmitters [nPlayer] = -1;
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

#define GATLING_MAX_PARTS	35
#define GATLING_PART_LIFE	-1000
#define GATLING_PART_SPEED	30

void DoGatlingSmoke (CObject *objP)
{
#if GATLING_SMOKE
	int32_t	nModel = objP->ModelId ();
	int32_t	bHires = G3HaveModel (nModel) - 1;

if (bHires >= 0) {
	RenderModel::CModel	*pm = gameData.models.renderModels [bHires] + nModel;

	if (pm->m_bBullets) {
			int32_t			nPlayer = objP->info.nId;
			int32_t			nGun = EquippedPlayerGun (objP);
			int32_t			bDoEffect = (bHires >= 0) && ((nGun == VULCAN_INDEX) || (nGun == GAUSS_INDEX)) &&
											(gameData.multiplayer.weaponStates [nPlayer].firing [0].nDuration >= GATLING_DELAY);
			int32_t			i = gameData.multiplayer.gatlingSmoke [nPlayer];

		if (bDoEffect) {
				int32_t			bSpectate = SPECTATOR (objP);
				tObjTransformation	*posP = bSpectate ? &gameStates.app.playerPos : &objP->info.position;
				CFixVector	*vGunPoints, vEmitter, vDir;
				CFixMatrix	m, *viewP;

			if (!(vGunPoints = GetGunPoints (objP, nGun)))
				return;
			if (bSpectate) {
				viewP = &m;
				*viewP = posP->mOrient.Transpose();
			}

			else
				viewP = objP->View (0);
			vEmitter = *viewP * vGunPoints[nGun];
			vEmitter += posP->vPos;
			//vDir = posP->mOrient.m.v.f;
			vDir = posP->mOrient.m.dir.f * (I2X (1) / 8);
			if (i < 0) {
				gameData.multiplayer.gatlingSmoke [nPlayer] =
					particleManager.Create (&vEmitter, &vDir, &posP->mOrient, objP->info.nSegment, 1, GATLING_MAX_PARTS, I2X (1) / 2, 
													/*1, 1,*/ 
													GATLING_PART_LIFE, GATLING_PART_SPEED, SIMPLE_SMOKE_PARTICLES, 0x7ffffffe, smokeColors + 3, 0, -1);
				}
			else {
				particleManager.SetPos (i, &vEmitter, &posP->mOrient, objP->info.nSegment);
				}
			}
		else {
			if (i >= 0) {
				particleManager.SetLife (i, 0);
				gameData.multiplayer.gatlingSmoke [nPlayer] = -1;
				}
			}
		}
	}
#endif
}

//------------------------------------------------------------------------------

void DoPlayerSmoke (CObject *objP, int32_t nPlayer)
{
#if PLAYER_SMOKE
	int32_t					nObject, nSmoke, d, nParts, nType;
	float					nScale;
	CParticleEmitter	*emitterP;
	CFixVector			fn, mn, vDir;
	tThrusterInfo		ti;

	static int32_t	bForward = 1;

if (nPlayer < 0)
	nPlayer = objP->info.nId;
if ((gameData.multiplayer.players [nPlayer].flags & PLAYER_FLAGS_CLOAKED) ||
	 (automap.Display () && IsMultiGame && !AM_SHOW_PLAYERS)) {
	KillObjectSmoke (nPlayer);
	return;
	}
nObject = objP->Index ();
if (gameOpts->render.particles.bDecreaseLag && (nPlayer == N_LOCALPLAYER)) {
	fn = objP->info.position.mOrient.m.dir.f;
	mn = objP->info.position.vPos - objP->info.vLastPos;
	CFixVector::Normalize (fn);
	CFixVector::Normalize (mn);
	d = CFixVector::Dot (fn, mn);
	if (d >= -I2X (1) / 2)
		bForward = 1;
	else {
		if (bForward) {
			if ((nSmoke = particleManager.GetObjectSystem (nObject)) >= 0) {
				KillObjectSmoke (nObject);
				particleManager.Destroy (nSmoke);
				}
			bForward = 0;
			nScale = 0;
			return;
			}
		}
	}
#if 0
if (EGI_FLAG (bThrusterFlames, 1, 1, 0)) {
	if ((vec <= I2X (1) / 4) && (vec || !gameStates.input.bControlsSkipFrame))	//no thruster flames if moving backward
		DropAfterburnerBlobs (objP, 2, I2X (1), -1, gameData.objs.consoleP, 1); //I2X (1) / 4);
	}
#endif
if (IsNetworkGame && !gameData.multiplayer.players [nPlayer].connected)
	nParts = 0;
else if (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED))
	nParts = 0;
else if ((nPlayer == N_LOCALPLAYER) && (gameStates.app.bPlayerIsDead || (gameData.multiplayer.players [nPlayer].Shield () < 0)))
	nParts = 0;
else if (SHOW_SMOKE && gameOpts->render.particles.bPlayers) {
	tObjTransformation* pos = OBJPOS (objP);
	int16_t nSegment = OBJSEG (objP);
	CFixVector* vDirP = (SPECTATOR (objP) || objP->mType.physInfo.thrust.IsZero ()) ? &vDir : NULL;

	nSmoke = X2IR (gameData.multiplayer.players [nPlayer].Shield ());
	nScale = X2F (objP->info.xSize) / 2.0f;
	nParts = 10 - nSmoke / 5;	// scale with damage, starting at 50% damage
	if (nParts <= 0) {
		nType = 2;	// no damage
		//nScale /= 2;
		nParts = SHIP_MAX_PARTS / 2; //vDirP ? SHIP_MAX_PARTS / 2 : SHIP_MAX_PARTS;
		}
	else {
		CreateDamageExplosion (nParts, nObject);
		nType = (nSmoke > 25);	// 0: severely damaged, 1: damaged
		nScale /= 2.0f - float (nSmoke) / 50.0f;
		nParts *= 25;
		nParts += 75;
		}

	if (nParts && ((nType < 2) || vDirP)) {
		if (gameOpts->render.particles.bSyncSizes) {
			nParts = -MAX_PARTICLES (nParts, gameOpts->render.particles.nDens [0]);
			nScale = PARTICLE_SIZE (gameOpts->render.particles.nSize [0], nScale, 1);
			}
		else {
			nParts = -MAX_PARTICLES (nParts, gameOpts->render.particles.nDens [1]);
			nScale = PARTICLE_SIZE (gameOpts->render.particles.nSize [1], nScale, 1);
			}
		int32_t nLife = ((nType != 2) || vDirP) ? PLR_PART_LIFE : PLR_PART_LIFE / 7;
		if (vDirP) {	// if the ship is standing still, let the thruster smoke move away from it
			nParts /= 2;
			if (nType != 2)
				nScale /= 2;
			else
				nType = 4;
			vDir = OBJPOS (objP)->mOrient.m.dir.f * -(I2X (1) / 8);
			}
		else if (nType == 2)
			nParts /= 2;
		if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
			nSmoke = particleManager.Create (&pos->vPos, vDirP, NULL, nSegment, 2, nParts, nScale,
														/*gameOpts->render.particles.nSize [1], 2,*/ 
														nLife, PLR_PART_SPEED, SMOKE_PARTICLES, nObject, smokeColors + nType, 1, -1);
			if (nSmoke < 0)
				return;
			particleManager.SetObjectSystem (nObject, nSmoke);
			}
		else {
			particleManager.SetDir (nSmoke, vDirP);
			particleManager.SetFadeType (nSmoke, vDirP != NULL);
			particleManager.SetLife (nSmoke, nLife);
			//particleManager.SetBlowUp (nSmoke, bBlowUp);
			particleManager.SetType (nSmoke, SMOKE_PARTICLES);
			particleManager.SetScale (nSmoke, -nScale);
			particleManager.SetDensity (nSmoke, nParts, 1/*, gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [1]*/);
			particleManager.SetSpeed (particleManager.GetObjectSystem (nObject),
											  objP->mType.physInfo.thrust.IsZero () ? PLR_PART_SPEED * 2 : PLR_PART_SPEED);
			}
		thrusterFlames.CalcPos (objP, &ti);
		for (int32_t i = 0; i < 2; i++)
			if ((emitterP = particleManager.GetEmitter (nSmoke, i)))
				emitterP->SetPos (ti.vPos + i, NULL, nSegment);
		}
	DoGatlingSmoke (objP);
	return;
	}
KillObjectSmoke (nObject);
KillGatlingSmoke (objP);
#endif
}

//------------------------------------------------------------------------------

void DoRobotSmoke (CObject *objP)
{
#if ROBOT_SMOKE
	int32_t			h = -1, nObject, nSmoke, nShield = 0, nParts;
	float			nScale;
	CFixVector	pos, vDir;

nObject = objP->Index ();
if (!(SHOW_SMOKE && gameOpts->render.particles.bRobots)) {
	if (particleManager.GetObjectSystem (nObject) >= 0)
		KillObjectSmoke (nObject);
	return;
	}
if ((objP->info.xShield < 0) || (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else {
	nShield = X2IR (RobotDefaultShield (objP));
	if (0 >= nShield)
		return;
	h = X2IR (objP->info.xShield) * 100 / nShield;
	}
if (h < 0)
	return;
nParts = 10 - h / 5;
if (nParts > 0) {
	CFixVector* vDirP = objP->mType.physInfo.velocity.IsZero () ? &vDir : NULL;

	if (vDirP) // if the robot is standing still, let the smoke move away from it
		vDir = OBJPOS (objP)->mOrient.m.dir.f * -(I2X (1) / 12);

	if (nShield > 4000)
		nShield = 4000;
	else if (nShield < 1000)
		nShield = 1000;
	CreateDamageExplosion (nParts, nObject);
	//nParts *= nShield / 10;
	nParts = BOT_MAX_PARTS;
	nScale = (float) sqrt (8.0 / X2F (objP->info.xSize));
	nScale *= 1.0f + h / 25.0f;
	if (!gameOpts->render.particles.bSyncSizes) {
		nParts = -MAX_PARTICLES (nParts, gameOpts->render.particles.nDens [2]);
		nScale = PARTICLE_SIZE (gameOpts->render.particles.nSize [2], nScale, 1);
		}
	if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
		nSmoke = particleManager.Create (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts, nScale,
													/*gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [2], 1,*/ 
													gameOpts->render.particles.nSize [3] * BOT_PART_LIFE / 2, BOT_PART_SPEED, SMOKE_PARTICLES, nObject, smokeColors, 1, -1);
		if (nSmoke < 0)
			return;
		particleManager.SetObjectSystem (nObject, nSmoke);
		}
	else {
		particleManager.SetDir (nSmoke, vDirP);
		particleManager.SetFadeType (nSmoke, vDirP != NULL);
		particleManager.SetScale (nSmoke, nScale);
		particleManager.SetDensity (nSmoke, nParts, 2/*, gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [2]*/);
		particleManager.SetSpeed (nSmoke, !objP->mType.physInfo.velocity.IsZero () ? BOT_PART_SPEED : BOT_PART_SPEED * 2 / 3);
		}
	pos = objP->info.position.vPos + objP->info.position.mOrient.m.dir.f * (-objP->info.xSize / 2);
	particleManager.SetPos (nSmoke, &pos, NULL, objP->info.nSegment);
	}
else
	KillObjectSmoke (nObject);
#endif
}

//------------------------------------------------------------------------------

void DoReactorSmoke (CObject *objP)
{
#if REACTOR_SMOKE
	int32_t			h = -1, nObject, nSmoke, nShield = 0, nParts;
	CFixVector	vDir, vPos;

nObject = objP->Index ();
if (!(SHOW_SMOKE && gameOpts->render.particles.bRobots)) {
	if (particleManager.GetObjectSystem (nObject) >= 0)
		KillObjectSmoke (nObject);
	return;
	}
if ((objP->info.xShield < 0) || (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else {
	nShield = X2IR (gameData.bots.info [gameStates.app.bD1Mission][objP->info.nId].strength);
	h = nShield ? X2IR (objP->info.xShield) * 100 / nShield : 0;
	}
if (h < 0)
	h = 0;
nParts = 10 - h / 10;
if (nParts > 0) {
	nParts = REACTOR_MAX_PARTS;
	if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
		nSmoke = particleManager.Create (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts, F2X (-4.0),
													/*-1, 1,*/ 
													gameOpts->render.particles.nSize [3] * BOT_PART_LIFE, BOT_PART_SPEED, SMOKE_PARTICLES, nObject, smokeColors, 1, -1);
		if (nSmoke < 0)
			return;
		particleManager.SetObjectSystem (nObject, nSmoke);
		}
	else {
		particleManager.SetScale (nSmoke, F2X (-4.0));
		particleManager.SetDensity (nSmoke, nParts, 2/*, -1*/);
		vDir.v.coord.x = SRandShort ();
		vDir.v.coord.y = SRandShort ();
		vDir.v.coord.z = SRandShort ();
		CFixVector::Normalize (vDir);
		vPos = objP->info.position.vPos + vDir * (-objP->info.xSize / 2);
		particleManager.SetPos (nSmoke, &vPos, NULL, objP->info.nSegment);
		}
	}
else
	KillObjectSmoke (nObject);
#endif
}

//------------------------------------------------------------------------------

void DoMissileSmoke (CObject *objP)
{
#if MISSILE_SMOKE
	int32_t				nParts, nSpeed, nObject, nSmoke;
	float				nScale = 1.5f + float (gameOpts->render.particles.nQuality) / 2.0f, 
						nLife;

nObject = objP->Index ();
if (!(SHOW_SMOKE && gameOpts->render.particles.bMissiles)) {
	if (particleManager.GetObjectSystem (nObject) >= 0)
		KillObjectSmoke (nObject);
	return;
	}
if ((objP->info.xShield < 0) || (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else {
	nSpeed = WI_speed (objP->info.nId, gameStates.app.nDifficultyLevel);
	nLife = float (gameOpts->render.particles.nLife [3]) * sqrt (float (nSpeed) / float (WI_speed (CONCUSSION_ID, gameStates.app.nDifficultyLevel)));
	nParts = int32_t (MSL_MAX_PARTS /** nLife*/); //int32_t (MSL_MAX_PARTS * X2F (nSpeed) / (15.0f * (4 - nLife)));
#if 0
	if ((objP->info.nId == EARTHSHAKER_MEGA_ID) || (objP->info.nId == ROBOT_SHAKER_MEGA_ID))
		nParts /= 2;
#endif
	}
if (nParts) {
	if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
		if (!gameOpts->render.particles.bSyncSizes) {
			nParts = -MAX_PARTICLES (nParts, gameOpts->render.particles.nDens [3]);
			nScale = PARTICLE_SIZE (gameOpts->render.particles.nSize [3], nScale, 1);
			}
		nSmoke = particleManager.Create (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, int32_t (nParts * nLife * nLife), nScale,
													/*gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [3], 1,*/ 
													int32_t (nLife * MSL_PART_LIFE * 0.5), MSL_PART_SPEED, SMOKE_PARTICLES, nObject, smokeColors + 1, 1, -1);
		if (nSmoke < 0)
			return;
		particleManager.SetObjectSystem (nObject, nSmoke);
		}
#if 1
	particleManager.SetPos (nSmoke, &objP->info.position.vPos, NULL, objP->info.nSegment);
#else
	tThrusterInfo	ti;
	thrusterFlames.CalcPos (objP, &ti);
	particleManager.SetPos (nSmoke, ti.vPos, NULL, objP->info.nSegment);
#endif
	}
else
	KillObjectSmoke (nObject);
#endif
}

//------------------------------------------------------------------------------

void DoDebrisSmoke (CObject *objP)
{
#if DEBRIS_SMOKE
	int32_t			nParts, nObject, nSmoke;
	float			nScale = 2;
	CFixVector	pos;

nObject = objP->Index ();
if (!(SHOW_SMOKE && gameOpts->render.particles.bDebris)) {
	if (particleManager.GetObjectSystem (nObject) >= 0)
		KillObjectSmoke (nObject);
	return;
	}
if ((objP->info.xShield < 0) || (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)) ||
	 (gameOpts->render.nDebrisLife && (I2X (nDebrisLife [gameOpts->render.nDebrisLife]) - objP->info.xLifeLeft > I2X (10))))
	nParts = 0;
else
	nParts = DEBRIS_MAX_PARTS;
if (nParts) {
	if (!gameOpts->render.particles.bSyncSizes) {
		nParts = -MAX_PARTICLES (nParts, gameOpts->render.particles.nDens [4]);
		nScale = PARTICLE_SIZE (gameOpts->render.particles.nSize [4], nScale, 1);
		}
	if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
		nSmoke = particleManager.Create (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts / 2, nScale, 
													/*-1, 1,*/ 
													gameOpts->render.particles.nSize [4] * DEBRIS_PART_LIFE / 2, DEBRIS_PART_SPEED, SMOKE_PARTICLES, nObject, smokeColors, 0, -1);
		if (nSmoke < 0)
			return;
		particleManager.SetObjectSystem (nObject, nSmoke);
		}
	pos = objP->info.position.vPos + objP->info.position.mOrient.m.dir.f * (-objP->info.xSize);
	particleManager.SetPos (nSmoke, &pos, NULL, objP->info.nSegment);
	}
else
	KillObjectSmoke (nObject);
#endif
}

//------------------------------------------------------------------------------

void DoStaticParticles (CObject *objP)
{
#if STATIC_SMOKE
	int32_t			i, j, nObject, nSmoke, 
					nType, nFadeType;
	CFixVector	pos, offs, dir;

	static CFloatVector defaultColors [7] = {{{{0.5f, 0.5f, 0.5f, 0.0f}}}, {{{0.8f, 0.9f, 1.0f, 1.0f}}}, {{{1.0f, 1.0f, 1.0f, 1.0f}}},
														 {{{1.0f, 1.0f, 1.0f, 1.0f}}}, {{{1.0f, 1.0f, 1.0f, 1.0f}}}, {{{1.0f, 1.0f, 1.0f, 1.0f}}},
														 {{{1.0f, 1.0f, 1.0f, 1.0f}}}};
	static int32_t particleTypes [7] = {SMOKE_PARTICLES, BUBBLE_PARTICLES, FIRE_PARTICLES, WATERFALL_PARTICLES, SIMPLE_SMOKE_PARTICLES, RAIN_PARTICLES, SNOW_PARTICLES};

nObject = (int32_t) objP->Index ();
if (objP->rType.particleInfo.nType == SMOKE_TYPE_WATERFALL) {
	nType = 3;
	nFadeType = 2;
	}
else if (objP->rType.particleInfo.nType == SMOKE_TYPE_FIRE) {
	nType = 2;
	nFadeType = 3;
	}
else if (objP->rType.particleInfo.nType == SMOKE_TYPE_BUBBLES) {
	nType = 1;
	nFadeType = -1;
	}
else if ((objP->rType.particleInfo.nType == SMOKE_TYPE_RAIN) || (objP->rType.particleInfo.nType == SMOKE_TYPE_SNOW)) {
	nType = objP->rType.particleInfo.nType;
	nFadeType = -1;
	}
else {
	nType = 0;
	nFadeType = 1;
	}
if (!(SHOW_SMOKE && objP->rType.particleInfo.bEnabled && ((nType == 1) ? gameOpts->render.particles.bBubbles : gameOpts->render.particles.bStatic))) {
	if (particleManager.GetObjectSystem (nObject) >= 0)
		KillObjectSmoke (nObject);
	return;
	}
if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
		CFloatVector color;
		int32_t bColor;

	color.Red () = (float) objP->rType.particleInfo.color.Red () / 255.0f;
	color.Green () = (float) objP->rType.particleInfo.color.Green () / 255.0f;
	color.Blue () = (float) objP->rType.particleInfo.color.Blue () / 255.0f;
	if ((bColor = (color.Red () + color.Green () + color.Blue () > 0)))
		color.Alpha () = (float) -objP->rType.particleInfo.color.Alpha () / 255.0f;
	dir = objP->info.position.mOrient.m.dir.f * (objP->rType.particleInfo.nSpeed * I2X (2) / 55);
	nSmoke = particleManager.Create (&objP->info.position.vPos, &dir, &objP->info.position.mOrient,
												objP->info.nSegment, 1,
												-objP->rType.particleInfo.nParts / ((nType == SMOKE_TYPE_RAIN) ? 10 : ((nType == SMOKE_TYPE_FIRE) || (nType == SMOKE_TYPE_SNOW)) ? 10 : 1),
												-PARTICLE_SIZE (objP->rType.particleInfo.nSize [gameOpts->render.particles.bDisperse], (nType == 1) ? 4.0f : 2.0f, 1),
												/*-1, 3,*/ 
												(nType == 2) ? FIRE_PART_LIFE * int32_t (sqrt (double (objP->rType.particleInfo.nLife))) : STATIC_SMOKE_PART_LIFE * objP->rType.particleInfo.nLife,
												(nType == SMOKE_TYPE_RAIN) ? objP->rType.particleInfo.nDrift * 64 : objP->rType.particleInfo.nDrift, 
												particleTypes [nType], 
												nObject, bColor ? &color : defaultColors + nType, nType != 2, objP->rType.particleInfo.nSide - 1);
	if (nSmoke < 0)
		return;
	particleManager.SetObjectSystem (nObject, nSmoke);
	if (nType == 1)
		audio.CreateObjectSound (-1, SOUNDCLASS_GENERIC, nObject, 1, I2X (1) / 2, I2X (256), -1, -1, AddonSoundName (SND_ADDON_AIRBUBBLES));
	else
		particleManager.SetBrightness (nSmoke, objP->rType.particleInfo.nBrightness);
	particleManager.SetFadeType (nSmoke, nFadeType);
	}
if (objP->rType.particleInfo.nSide <= 0) {	//don't vary emitter position for smoke emitting faces
	i = objP->rType.particleInfo.nDrift >> 4;
	i += objP->rType.particleInfo.nSize [0] >> 2;
	i /= 2;
	if (!(j = i - i / 2))
		j = 2;
	i /= 2;
	offs.v.coord.x = SRandShort () * (RandShort () % j + i);
	offs.v.coord.y = SRandShort () * (RandShort () % j + i);
	offs.v.coord.z = SRandShort () * (RandShort () % j + i);
	pos = objP->info.position.vPos + offs;
	particleManager.SetPos (nSmoke, &pos, NULL, objP->info.nSegment);
	}
#endif
}

//------------------------------------------------------------------------------

void DoBombSmoke (CObject *objP)
{
#if BOMB_SMOKE
	int32_t			nParts, nObject, nSmoke;
	CFixVector	pos, offs;

if (gameStates.app.bNostalgia || !gameStates.app.bHaveExtraGameInfo [IsMultiGame])
	return;
nObject = objP->Index ();
if ((objP->info.xShield < 0) || (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else
	nParts = -BOMB_MAX_PARTS;
if (nParts) {
	if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
		nSmoke = particleManager.Create (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts, -PARTICLE_SIZE (3, 0.5f, 1), 
													/*-1, 3,*/ 
													BOMB_PART_LIFE, BOMB_PART_SPEED, SMOKE_PARTICLES, nObject, NULL, 1, -1);
		if (nSmoke < 0)
			return;
		particleManager.SetObjectSystem (nObject, nSmoke);
		}
	offs.v.coord.x = SRandShort () * ((RandShort () & 15) + 16);
	offs.v.coord.y = SRandShort () * ((RandShort () & 15) + 16);
	offs.v.coord.z = SRandShort () * ((RandShort () & 15) + 16);
	pos = objP->info.position.vPos + offs;
	particleManager.SetPos (nSmoke, &pos, NULL, objP->info.nSegment);
	}
else
	KillObjectSmoke (nObject);
#endif
}

//------------------------------------------------------------------------------

void DoParticleTrail (CObject *objP)
{
#if PARTICLE_TRAIL
	int32_t			nParts, nObject, nSmoke, id = objP->info.nId, 
					bGatling = objP->IsGatlingRound (),
					bOmega = (id == OMEGA_ID);
	float			nScale;
	CFixVector	pos;
	CFloatVector	c;

if (!(SHOW_OBJ_FX && (bGatling ? EGI_FLAG (bUseParticles, 0, 0, 0) && EGI_FLAG (bGatlingTrails, 1, 1, 0) : EGI_FLAG (bLightTrails, 1, 1, 0))))
	return;
nObject = objP->Index ();
if (!(bGatling || gameOpts->render.particles.bPlasmaTrails)) {
	if (particleManager.GetObjectSystem (nObject) >= 0)
		KillObjectSmoke (nObject);
	return;
	}
#if 1
nParts = bGatling ? LASER_MAX_PARTS : 2 * LASER_MAX_PARTS / 3;
#else
nParts = gameData.weapons.info [objP->info.nId].speed [0] / I2X (1);
#endif
if (bGatling) {
	c.Red () = c.Green () = c.Blue () = 2.0f / 3.0f;
	c.Alpha () = 1.0f / 4.0f;
	}
else {
	c.Red () = (float) gameData.weapons.color [objP->info.nId].Red ();
	c.Green () = (float) gameData.weapons.color [objP->info.nId].Green ();
	c.Blue () = (float) gameData.weapons.color [objP->info.nId].Blue ();
	c.Alpha () = 0.5f;
	}
if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
	if (bGatling)
		nScale = 5.0f;
	else {
		if (((id >= LASER_ID) && (id <= LASER_ID + 3)) ||
			 (id == SUPERLASER_ID) || (id == SUPERLASER_ID + 1) ||
			 (id == ROBOT_BLUE_LASER_ID) || (id == ROBOT_GREEN_LASER_ID) || 
			 (id == ROBOT_RED_LASER_ID) || (id == ROBOT_WHITE_LASER_ID))
			nScale = 3;
		else if ((id == PHOENIX_ID) || (id == ROBOT_LIGHT_FIREBALL_ID) || (id == ROBOT_FAST_PHOENIX_ID))
			nScale = 1;
		else if ((id == PLASMA_ID) || (id == ROBOT_PLASMA_ID))
			nScale = 1.5;
		else if (id == FUSION_ID)
			nScale = 2;
		else if ((id == SPREADFIRE_ID) || (id == HELIX_ID) || (id == ROBOT_HELIX_ID))
			nScale = 2;
		else if (id == FLARE_ID)
			nScale = 2;
		else if ((id == ROBOT_BLUE_ENERGY_ID) || (id == ROBOT_WHITE_ENERGY_ID) || (id == ROBOT_PHASE_ENERGY_ID))
			nScale = 2;
		else if (bOmega)
			nScale = omegaLightning.Exist () ? 2.0f : 1.0f;
		else
			nScale = 1;
		c.Alpha () = 0.1f + nScale / 10;
		}
	nSmoke = particleManager.Create (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts << bGatling, -PARTICLE_SIZE (1, nScale, 1),
											   /*gameOpts->render.particles.nSize [3], 1,*/ 
												(gameOpts->render.particles.nLife [3] + 1) * (bGatling ? 3 * LASER_PART_LIFE / 2 : LASER_PART_LIFE >> bOmega) /*<< bGatling*/, LASER_PART_SPEED, 
											   bGatling ? GATLING_PARTICLES : LIGHT_PARTICLES, nObject, &c, 0, -1);
	if (nSmoke < 0)
		return;
	particleManager.SetObjectSystem (nObject, nSmoke);
	particleManager.SetFadeType (nSmoke, bGatling ? 0 : 4);
	}
pos = objP->RenderPos () + objP->info.position.mOrient.m.dir.f * (-objP->info.xSize / 2);
particleManager.SetPos (nSmoke, &pos, NULL, objP->info.nSegment);
#endif
}

//------------------------------------------------------------------------------

int32_t DoObjectSmoke (CObject *objP)
{
int32_t t = objP->info.nType;
	uint8_t nId = objP->info.nId;

if (t == OBJ_PLAYER)
	DoPlayerSmoke (objP, -1);
else if (t == OBJ_ROBOT)
	DoRobotSmoke (objP);
else if ((t == OBJ_EFFECT) && (nId == PARTICLE_ID))
	DoStaticParticles (objP);
else if (t == OBJ_REACTOR)
	DoReactorSmoke (objP);
else if (t == OBJ_WEAPON) {
	if (objP->IsMissile ())
		DoMissileSmoke (objP);
	else if (objP->IsGatlingRound ())
		DoParticleTrail (objP);
	else if (IsMultiGame && !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0) && (nId == PROXMINE_ID))
		DoBombSmoke (objP);
	else if (gameOpts->render.particles.bPlasmaTrails && gameStates.app.bHaveExtraGameInfo [IsMultiGame] && EGI_FLAG (bLightTrails, 0, 0, 0) &&
				objP->HasLightTrail (nId))
		DoParticleTrail (objP);
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

void PlayerBulletFrame (void)
{
if (!gameOpts->render.ship.bBullets)
	return;
for (int32_t i = 0; i < gameData.multiplayer.nPlayers; i++)
	DoPlayerBullets (OBJECTS + gameData.multiplayer.players [i].nObject);
}

//------------------------------------------------------------------------------

void PlayerParticleFrame (void)
{
if (!gameOpts->render.particles.bPlayers)
	return;
for (int32_t i = 0; i < gameData.multiplayer.nPlayers; i++)
	DoPlayerSmoke (OBJECTS + gameData.multiplayer.players [i].nObject, i);
}

//------------------------------------------------------------------------------

void ObjectParticleFrame (void)
{
#if 0
if (!SHOW_SMOKE)
	return;
#endif
CObject	*objP = OBJECTS.Buffer ();

for (int32_t i = 0; i <= gameData.objs.nLastObject [1]; i++, objP++) {
	if (gameData.objs.bWantEffect [i] & DESTROY_SMOKE) {
		gameData.objs.bWantEffect [i] &= ~DESTROY_SMOKE;
		KillObjectSmoke (i);
		}
	else if (objP->info.nType == OBJ_NONE)
		KillObjectSmoke (i);
	else
		DoObjectSmoke (objP);
	}
}

//------------------------------------------------------------------------------

void StaticParticlesFrame (void)
{
	CObject* objP;

if (!SHOW_SMOKE)
	return;
FORALL_EFFECT_OBJS (objP) {
	if (objP->info.nId == PARTICLE_ID)
		DoStaticParticles (objP);
	}
}

//------------------------------------------------------------------------------

void DoParticleFrame (void)
{
//if (gameStates.render.nShadowPass > 1)
//	return;
PlayerBulletFrame ();
ObjectParticleFrame ();
StaticParticlesFrame ();
shrapnelManager.DoFrame ();
particleManager.Update ();
}

//------------------------------------------------------------------------------
