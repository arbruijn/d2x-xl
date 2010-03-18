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

static tRgbaColorf smokeColors [] = {
	 {0.5f, 0.5f, 0.5f, 1.0f},
	 {0.75f, 0.75f, 0.75f, 1.0f},
	 {1.0f, 1.0f, 1.0f, 1.0f},
	 {1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f, -0.25f},
	 {1.0f, 1.0f, 1.0f, -0.1f}
	};

#define SHIP_MAX_PARTS				50
#define PLR_PART_LIFE				-1400
#define PLR_PART_SPEED				50

#define BOT_MAX_PARTS				250
#define BOT_PART_LIFE				-6000
#define BOT_PART_SPEED				300

#define MSL_MAX_PARTS				500
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

//------------------------------------------------------------------------------

#if DBG

void KillObjectSmoke (int nObject)
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
particleManager.InitObjects ();
}

//------------------------------------------------------------------------------

void ResetObjectSmoke (void)
{
	int	i;

for (i = 0; i < LEVEL_OBJECTS; i++)
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
	 (gameStates.app.nSDLTicks - *particleManager.ObjExplTime (i) > 100)) {
	*particleManager.ObjExplTime (i) = gameStates.app.nSDLTicks;
	if (!RandN (11 - h))
		CreateSmallFireballOnObject (OBJECTS + i, I2X (1), 1);
	}
}

//------------------------------------------------------------------------------
#if 0
void CreateThrusterFlames (CObject *objP)
{
	static int nThrusters = -1;

	CFixVector	pos, dir = objP->info.position.mOrient.FVec ();
	int			d, j;
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
		VmVecScaleAdd (&pos, &objP->info.position.vPos, &objP->info.position.mOrient.FVec (), -objP->info.xSize);
		VmVecScaleInc (&pos, &objP->info.position.mOrient.RVec (), j ? d : -d);
		VmVecScaleInc (&pos, &objP->info.position.mOrient.UVec (),  -objP->info.xSize / 25);
		SetParticleEmitterPos (emitterP, &pos, NULL, objP->info.nSegment);
		}
}
#endif

//------------------------------------------------------------------------------

void KillPlayerBullets (CObject *objP)
{
	int	i = gameData.multiplayer.bulletEmitters [objP->info.nId];

if (i >= 0) {
	particleManager.SetLife (i, 0);
	gameData.multiplayer.bulletEmitters [objP->info.nId] = -1;
	}
}

//------------------------------------------------------------------------------

void KillGatlingSmoke (CObject *objP)
{
	int	i = gameData.multiplayer.gatlingSmoke [objP->info.nId];

if (i >= 0) {
	particleManager.SetLife (i, 0);
	gameData.multiplayer.gatlingSmoke [objP->info.nId] = -1;
	}
}

//------------------------------------------------------------------------------

#define BULLET_MAX_PARTS	50
#define BULLET_PART_LIFE	-2000
#define BULLET_PART_SPEED	50

void DoPlayerBullets (CObject *objP)
{
if (gameOpts->render.ship.bBullets) {
		int	nModel = objP->rType.polyObjInfo.nModel;
		int	bHires = G3HaveModel (nModel) - 1;

	if (bHires >= 0) {
		RenderModel::CModel	*pm = gameData.models.renderModels [bHires] + nModel;

		if (pm->m_bBullets) {
				int			nPlayer = objP->info.nId;
				int			nGun = EquippedPlayerGun (objP);
				int			bDoEffect = (bHires >= 0) && ((nGun == VULCAN_INDEX) || (nGun == GAUSS_INDEX)) &&
												(gameData.multiplayer.weaponStates [nPlayer].firing [0].nDuration >= GATLING_DELAY);
				int			i = gameData.multiplayer.bulletEmitters [nPlayer];

			if (bDoEffect) {
					int			bSpectate = SPECTATOR (objP);
					tObjTransformation	*posP = bSpectate ? &gameStates.app.playerPos : &objP->info.position;
					CFixVector	vEmitter, vDir;
					CFixMatrix	m, *viewP;

				if (bSpectate) {
					viewP = &m;
					m = posP->mOrient.Transpose();
				}
				else
					viewP = objP->View ();
				vEmitter = *viewP * pm->m_vBullets;
				vEmitter += posP->vPos;
				vDir = posP->mOrient.UVec ();
				vDir.Neg ();
				if (i < 0) {
					gameData.multiplayer.bulletEmitters [nPlayer] =
						particleManager.Create (&vEmitter, &vDir, &posP->mOrient, objP->info.nSegment, 1, BULLET_MAX_PARTS, 15.0f, 1,
													   1, BULLET_PART_LIFE, BULLET_PART_SPEED, BULLET_PARTICLES, 0x7fffffff, NULL, 0, -1);
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
	int	nModel = objP->rType.polyObjInfo.nModel;
	int	bHires = G3HaveModel (nModel) - 1;

if (bHires >= 0) {
	RenderModel::CModel	*pm = gameData.models.renderModels [bHires] + nModel;

	if (pm->m_bBullets) {
			int			nPlayer = objP->info.nId;
			int			nGun = EquippedPlayerGun (objP);
			int			bDoEffect = (bHires >= 0) && ((nGun == VULCAN_INDEX) || (nGun == GAUSS_INDEX)) &&
											(gameData.multiplayer.weaponStates [nPlayer].firing [0].nDuration >= GATLING_DELAY);
			int			i = gameData.multiplayer.gatlingSmoke [nPlayer];

		if (bDoEffect) {
				int			bSpectate = SPECTATOR (objP);
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
				viewP = objP->View ();
			vEmitter = *viewP * vGunPoints[nGun];
			vEmitter += posP->vPos;
			//vDir = posP->mOrient.FVec ();
			vDir = posP->mOrient.FVec () * (I2X (1) / 8);
			if (i < 0) {
				gameData.multiplayer.gatlingSmoke [nPlayer] =
					particleManager.Create (&vEmitter, &vDir, &posP->mOrient, objP->info.nSegment, 1, GATLING_MAX_PARTS, I2X (1) / 2, 1,
													1, GATLING_PART_LIFE, GATLING_PART_SPEED, SMOKE_PARTICLES, 0x7ffffffe, smokeColors + 3, 0, -1);
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
}

//------------------------------------------------------------------------------

void DoPlayerSmoke (CObject *objP, int nPlayer)
{
#if 1
	int					nObject, nSmoke, d, nParts, nType;
	float					nScale;
	CParticleEmitter	*emitterP;
	CFixVector			fn, mn, vDir;
	tThrusterInfo		ti;

	static int	bForward = 1;

if (nPlayer < 0)
	nPlayer = objP->info.nId;
if ((gameData.multiplayer.players [nPlayer].flags & PLAYER_FLAGS_CLOAKED) ||
	 (automap.Display () && IsMultiGame && !AM_SHOW_PLAYERS)) {
	KillObjectSmoke (nPlayer);
	return;
	}
nObject = objP->Index ();
if (gameOpts->render.particles.bDecreaseLag && (nPlayer == gameData.multiplayer.nLocalPlayer)) {
	fn = objP->info.position.mOrient.FVec ();
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
	if ((a <= I2X (1) / 4) && (a || !gameStates.input.bControlsSkipFrame))	//no thruster flames if moving backward
		DropAfterburnerBlobs (objP, 2, I2X (1), -1, gameData.objs.consoleP, 1); //I2X (1) / 4);
	}
#endif
if ((gameData.app.nGameMode & GM_NETWORK) && !gameData.multiplayer.players [nPlayer].connected)
	nParts = 0;
else if (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED))
	nParts = 0;
else if ((nPlayer == gameData.multiplayer.nLocalPlayer) && (gameStates.app.bPlayerIsDead || (gameData.multiplayer.players [nPlayer].shields < 0)))
	nParts = 0;
else if (SHOW_SMOKE && gameOpts->render.particles.bPlayers) {
	CFixVector* vDirP = (SPECTATOR (objP) || objP->mType.physInfo.thrust.IsZero ()) ? &vDir : NULL;

	nSmoke = X2IR (gameData.multiplayer.players [nPlayer].shields);
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
		int nLife = ((nType != 2) || vDirP) ? PLR_PART_LIFE : PLR_PART_LIFE / 7;
		if (vDirP) {	// if the ship is standing still, let the thruster smoke move away from it
			nParts /= 2;
			if (nType != 2)
				nScale /= 2;
			else
				nType = 4;
			vDir = OBJPOS (objP)->mOrient.FVec () * -(I2X (1) / 8);
			}
		else if (nType == 2)
			nParts /= 2;
		if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
			//PrintLog ("creating CPlayerData smoke\n");
			nSmoke = particleManager.Create (&objP->info.position.vPos, vDirP, NULL, objP->info.nSegment, 2, nParts, nScale,
														gameOpts->render.particles.nSize [1],
														2, nLife, PLR_PART_SPEED, SMOKE_PARTICLES, nObject, smokeColors + nType, 1, -1);
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
			particleManager.SetDensity (nSmoke, nParts, gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [1]);
			particleManager.SetSpeed (particleManager.GetObjectSystem (nObject),
											  objP->mType.physInfo.thrust.IsZero () ? PLR_PART_SPEED * 2 : PLR_PART_SPEED);
			}
		thrusterFlames.CalcPos (objP, &ti);
		for (int i = 0; i < 2; i++)
			if ((emitterP = particleManager.GetEmitter (nSmoke, i)))
				emitterP->SetPos (ti.vPos + i, NULL, objP->info.nSegment);
		DoGatlingSmoke (objP);
		return;
		}
	}
KillObjectSmoke (nObject);
KillGatlingSmoke (objP);
#endif
}

//------------------------------------------------------------------------------

void DoRobotSmoke (CObject *objP)
{
	int			h = -1, nObject, nSmoke, nShields = 0, nParts;
	float			nScale;
	CFixVector	pos, vDir;

nObject = objP->Index ();
if (!(SHOW_SMOKE && gameOpts->render.particles.bRobots)) {
	if (particleManager.GetObjectSystem (nObject) >= 0)
		KillObjectSmoke (nObject);
	return;
	}
if ((objP->info.xShields < 0) || (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else {
	nShields = X2IR (RobotDefaultShields (objP));
	h = X2IR (objP->info.xShields) * 100 / nShields;
	}
if (h < 0)
	return;
nParts = 10 - h / 5;
if (nParts > 0) {
	CFixVector* vDirP = objP->mType.physInfo.velocity.IsZero () ? &vDir : NULL;

	if (vDirP) // if the robot is standing still, let the smoke move away from it
		vDir = OBJPOS (objP)->mOrient.FVec () * -(I2X (1) / 12);

	if (nShields > 4000)
		nShields = 4000;
	else if (nShields < 1000)
		nShields = 1000;
	CreateDamageExplosion (nParts, nObject);
	//nParts *= nShields / 10;
	nParts = BOT_MAX_PARTS;
	nScale = (float) sqrt (8.0 / X2F (objP->info.xSize));
	nScale *= 1.0f + h / 25.0f;
	if (!gameOpts->render.particles.bSyncSizes) {
		nParts = -MAX_PARTICLES (nParts, gameOpts->render.particles.nDens [2]);
		nScale = PARTICLE_SIZE (gameOpts->render.particles.nSize [2], nScale, 1);
		}
	if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
		//PrintLog ("creating robot %d smoke\n", nObject);
		nSmoke = particleManager.Create (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts, nScale,
													gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [2],
													1, BOT_PART_LIFE, BOT_PART_SPEED, SMOKE_PARTICLES, nObject, smokeColors, 1, -1);
		if (nSmoke < 0)
			return;
		particleManager.SetObjectSystem (nObject, nSmoke);
		}
	else {
		particleManager.SetDir (nSmoke, vDirP);
		particleManager.SetFadeType (nSmoke, vDirP != NULL);
		particleManager.SetScale (nSmoke, nScale);
		particleManager.SetDensity (nSmoke, nParts, gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [2]);
		particleManager.SetSpeed (nSmoke, !objP->mType.physInfo.velocity.IsZero () ? BOT_PART_SPEED : BOT_PART_SPEED * 2 / 3);
		}
	pos = objP->info.position.vPos + objP->info.position.mOrient.FVec () * (-objP->info.xSize / 2);
	particleManager.SetPos (nSmoke, &pos, NULL, objP->info.nSegment);
	}
else
	KillObjectSmoke (nObject);
}

//------------------------------------------------------------------------------

void DoReactorSmoke (CObject *objP)
{
	int			h = -1, nObject, nSmoke, nShields = 0, nParts;
	CFixVector	vDir, vPos;

nObject = objP->Index ();
if (!(SHOW_SMOKE && gameOpts->render.particles.bRobots)) {
	if (particleManager.GetObjectSystem (nObject) >= 0)
		KillObjectSmoke (nObject);
	return;
	}
if ((objP->info.xShields < 0) || (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else {
	nShields = X2IR (gameData.bots.info [gameStates.app.bD1Mission][objP->info.nId].strength);
	h = nShields ? X2IR (objP->info.xShields) * 100 / nShields : 0;
	}
if (h < 0)
	h = 0;
nParts = 10 - h / 10;
if (nParts > 0) {
	nParts = REACTOR_MAX_PARTS;
	if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
		//PrintLog ("creating robot %d smoke\n", nObject);
		nSmoke = particleManager.Create (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts, F2X (-4.0),
													-1, 1, BOT_PART_LIFE * 2, BOT_PART_SPEED, SMOKE_PARTICLES, nObject, smokeColors, 1, -1);
		if (nSmoke < 0)
			return;
		particleManager.SetObjectSystem (nObject, nSmoke);
		}
	else {
		particleManager.SetScale (nSmoke, F2X (-4.0));
		particleManager.SetDensity (nSmoke, nParts, -1);
		vDir[X] = d_rand () - I2X (1) / 4;
		vDir[Y] = d_rand () - I2X (1) / 4;
		vDir[Z] = d_rand () - I2X (1) / 4;
		CFixVector::Normalize (vDir);
		vPos = objP->info.position.vPos + vDir * (-objP->info.xSize / 2);
		particleManager.SetPos (nSmoke, &vPos, NULL, objP->info.nSegment);
		}
	}
else
	KillObjectSmoke (nObject);
}

//------------------------------------------------------------------------------

void DoMissileSmoke (CObject *objP)
{
	int				nParts, nSpeed, nLife, nObject, nSmoke;
	float				nScale = 1.5f + float (gameOpts->render.particles.nQuality) / 2.0f;

nObject = objP->Index ();
if (!(SHOW_SMOKE && gameOpts->render.particles.bMissiles)) {
	if (particleManager.GetObjectSystem (nObject) >= 0)
		KillObjectSmoke (nObject);
	return;
	}
if ((objP->info.xShields < 0) || (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else {
	nSpeed = WI_speed (objP->info.nId, gameStates.app.nDifficultyLevel);
	nLife = gameOpts->render.particles.nLife [3] + 1;
#if 1
	nParts = int (MSL_MAX_PARTS * X2F (nSpeed) / (15.0f * (4 - nLife)));
	if ((objP->info.nId == EARTHSHAKER_MEGA_ID) || (objP->info.nId == ROBOT_SHAKER_MEGA_ID))
		nParts /= 2;

#else
	nParts = (objP->info.nId == EARTHSHAKER_ID) ? 1500 :
				(objP->info.nId == MEGAMSL_ID) ? 1400 :
				(objP->info.nId == SMARTMSL_ID) ? 1300 :
				1200;
#endif
	}
if (nParts) {
	if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
		if (!gameOpts->render.particles.bSyncSizes) {
			nParts = -MAX_PARTICLES (nParts, gameOpts->render.particles.nDens [3]);
			nScale = PARTICLE_SIZE (gameOpts->render.particles.nSize [3], nScale, 1);
			}
		nSmoke = particleManager.Create (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts, nScale,
													gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [3],
													1, nLife * MSL_PART_LIFE, MSL_PART_SPEED, SIMPLE_SMOKE_PARTICLES, nObject, smokeColors + 1, 1, -1);
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
}

//------------------------------------------------------------------------------

void DoDebrisSmoke (CObject *objP)
{
	int			nParts, nObject, nSmoke;
	float			nScale = 2;
	CFixVector	pos;

nObject = objP->Index ();
if (!(SHOW_SMOKE && gameOpts->render.particles.bDebris)) {
	if (particleManager.GetObjectSystem (nObject) >= 0)
		KillObjectSmoke (nObject);
	return;
	}
if ((objP->info.xShields < 0) || (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)) ||
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
		nSmoke = particleManager.Create (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts / 2,
												   nScale, -1, 1, DEBRIS_PART_LIFE, DEBRIS_PART_SPEED, SMOKE_PARTICLES, nObject, smokeColors, 0, -1);
		if (nSmoke < 0)
			return;
		particleManager.SetObjectSystem (nObject, nSmoke);
		}
	pos = objP->info.position.vPos + objP->info.position.mOrient.FVec () * (-objP->info.xSize);
	particleManager.SetPos (nSmoke, &pos, NULL, objP->info.nSegment);
	}
else
	KillObjectSmoke (nObject);
}

//------------------------------------------------------------------------------

void DoStaticParticles (CObject *objP)
{
	int			i, j, nObject, nSmoke, 
					nType, nFadeType;
	CFixVector	pos, offs, dir;

	static tRgbaColorf defaultColors [3] = {{0.5f, 0.5f, 0.5f, 0.0f}, {0.8f, 0.9f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}};
	static int particleTypes [4] = {SMOKE_PARTICLES, BUBBLE_PARTICLES, FIRE_PARTICLES, WATERFALL_PARTICLES};

nObject = (int) objP->Index ();
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
		tRgbaColorf color;
		int bColor;

	color.red = (float) objP->rType.particleInfo.color.red / 255.0f;
	color.green = (float) objP->rType.particleInfo.color.green / 255.0f;
	color.blue = (float) objP->rType.particleInfo.color.blue / 255.0f;
	if ((bColor = (color.red + color.green + color.blue > 0)))
		color.alpha = (float) -objP->rType.particleInfo.color.alpha / 255.0f;
	dir = objP->info.position.mOrient.FVec () * (objP->rType.particleInfo.nSpeed * I2X (2) / 55);
	nSmoke = particleManager.Create (&objP->info.position.vPos, &dir, &objP->info.position.mOrient,
												objP->info.nSegment, 1, -objP->rType.particleInfo.nParts / ((nType == 2) ? 10 : 1),
												-PARTICLE_SIZE (objP->rType.particleInfo.nSize [gameOpts->render.particles.bDisperse], (nType == 1) ? 4.0f : 2.0f, 1),
												-1, 3, (nType == 2) ? FIRE_PART_LIFE * int (sqrt (double (objP->rType.particleInfo.nLife))) : STATIC_SMOKE_PART_LIFE * objP->rType.particleInfo.nLife,
												objP->rType.particleInfo.nDrift, particleTypes [nType], 
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
	offs [X] = (I2X (1) / 4 - d_rand ()) * (d_rand () % j + i);
	offs [Y] = (I2X (1) / 4 - d_rand ()) * (d_rand () % j + i);
	offs [Z] = (I2X (1) / 4 - d_rand ()) * (d_rand () % j + i);
	pos = objP->info.position.vPos + offs;
	particleManager.SetPos (nSmoke, &pos, NULL, objP->info.nSegment);
	}
}

//------------------------------------------------------------------------------

void DoBombSmoke (CObject *objP)
{
	int			nParts, nObject, nSmoke;
	CFixVector	pos, offs;

if (gameStates.app.bNostalgia || !gameStates.app.bHaveExtraGameInfo [IsMultiGame])
	return;
nObject = objP->Index ();
if ((objP->info.xShields < 0) || (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else
	nParts = -BOMB_MAX_PARTS;
if (nParts) {
	if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
		nSmoke = particleManager.Create (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts,
												   -PARTICLE_SIZE (3, 0.5f, 1), -1, 3, BOMB_PART_LIFE, BOMB_PART_SPEED, SMOKE_PARTICLES, nObject, NULL, 1, -1);
		if (nSmoke < 0)
			return;
		particleManager.SetObjectSystem (nObject, nSmoke);
		}
	offs [X] = (I2X (1) / 4 - d_rand ()) * ((d_rand () & 15) + 16);
	offs [Y] = (I2X (1) / 4 - d_rand ()) * ((d_rand () & 15) + 16);
	offs [Z] = (I2X (1) / 4 - d_rand ()) * ((d_rand () & 15) + 16);
	pos = objP->info.position.vPos + offs;
	particleManager.SetPos (nSmoke, &pos, NULL, objP->info.nSegment);
	}
else
	KillObjectSmoke (nObject);
}

//------------------------------------------------------------------------------

void DoParticleTrail (CObject *objP)
{
	int			nParts, nObject, nSmoke, id = objP->info.nId, 
					bGatling = (id == VULCAN_ID) || (id == GAUSS_ID),
					bOmega = (id == OMEGA_ID);
	float			nScale;
	CFixVector	pos;
	tRgbaColorf	c;

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
	c.red = c.green = c.blue = 2.0f / 3.0f;
	c.alpha = 1.0f / 4.0f;
	}
else {
	c.red = (float) gameData.weapons.color [objP->info.nId].red;
	c.green = (float) gameData.weapons.color [objP->info.nId].green;
	c.blue = (float) gameData.weapons.color [objP->info.nId].blue;
	c.alpha = 0.5f;
	}
if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
	if (bGatling)
		nScale = 5.0f;
	else {
		if (((id >= LASER_ID) && (id < LASER_ID + 4)) ||
			 (id == SUPERLASER_ID) || (id == SUPERLASER_ID + 1) ||
			 (id == ROBOT_BLUE_LASER_ID) || (id == ROBOT_GREEN_LASER_ID) || (id == ROBOT_RED_LASER_ID) || (id == ROBOT_WHITE_LASER_ID))
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
			nScale = omegaLightnings.Exist () ? 2.0f : 1.0f;
		else
			nScale = 1;
		c.alpha = 0.1f + nScale / 10;
		}
	nSmoke = particleManager.Create (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts << bGatling, -PARTICLE_SIZE (1, nScale, 1),
											   gameOpts->render.particles.nSize [3],
												1, (gameOpts->render.particles.nLife [3] + 1) * (bGatling ? 3 * LASER_PART_LIFE / 2 : LASER_PART_LIFE >> bOmega) /*<< bGatling*/, LASER_PART_SPEED, 
											   bGatling ? GATLING_PARTICLES : LIGHT_PARTICLES, nObject, &c, 0, -1);
	if (nSmoke < 0)
		return;
	particleManager.SetObjectSystem (nObject, nSmoke);
	particleManager.SetFadeType (nSmoke, bGatling ? 0 : 4);
	}
pos = objP->RenderPos () + objP->info.position.mOrient.FVec () * (-objP->info.xSize / 2);
particleManager.SetPos (nSmoke, &pos, NULL, objP->info.nSegment);
}

//------------------------------------------------------------------------------

int DoObjectSmoke (CObject *objP)
{
int t = objP->info.nType;
if (gameStates.render.bQueryCoronas)
	return 0;
if (t == OBJ_PLAYER)
	DoPlayerSmoke (objP, -1);
else if (t == OBJ_ROBOT)
	DoRobotSmoke (objP);
else if ((t == OBJ_EFFECT) && (objP->info.nId == SMOKE_ID))
	DoStaticParticles (objP);
else if (t == OBJ_REACTOR)
	DoReactorSmoke (objP);
else if (t == OBJ_WEAPON) {
	if (gameData.objs.bIsMissile [objP->info.nId])
		DoMissileSmoke (objP);
	else if ((objP->info.nId == VULCAN_ID) || (objP->info.nId == GAUSS_ID))
		DoParticleTrail (objP);
	else if (IsMultiGame && !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0) && (objP->info.nId == PROXMINE_ID))
		DoBombSmoke (objP);
	else if (gameOpts->render.particles.bPlasmaTrails && gameStates.app.bHaveExtraGameInfo [IsMultiGame] && EGI_FLAG (bLightTrails, 0, 0, 0) &&
				gameData.objs.bIsWeapon [objP->info.nId] && !gameData.objs.bIsSlowWeapon [objP->info.nId])
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
for (int i = 0; i < gameData.multiplayer.nPlayers; i++)
	DoPlayerBullets (OBJECTS + gameData.multiplayer.players [i].nObject);
}

//------------------------------------------------------------------------------

void PlayerParticleFrame (void)
{
if (!gameOpts->render.particles.bPlayers)
	return;
for (int i = 0; i < gameData.multiplayer.nPlayers; i++)
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

for (int i = 0; i <= gameData.objs.nLastObject [1]; i++, objP++) {
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
	CObject	*objP;

if (!SHOW_SMOKE)
	return;
FORALL_EFFECT_OBJS (objP, i) {
	if (objP->info.nId == SMOKE_ID)
		DoStaticParticles (objP);
	}
}

//------------------------------------------------------------------------------

void DoParticleFrame (void)
{
if (gameStates.render.nShadowPass > 1)
	return;
SEM_ENTER (SEM_SMOKE)
PlayerBulletFrame ();
ObjectParticleFrame ();
//StaticParticlesFrame ();
shrapnelManager.DoFrame ();
particleManager.Update ();
SEM_LEAVE (SEM_SMOKE)
}

//------------------------------------------------------------------------------
